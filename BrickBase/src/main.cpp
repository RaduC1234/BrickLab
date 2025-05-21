#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "brick_i2c_host.hpp"
#include "brick_lua_vm.hpp"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#define GATTS_TAG "BLE_SERVER"
#define SERVICE_UUID 0x00FF
#define CHAR_UUID_GET  0xFF01
#define CHAR_UUID_POST 0xFF02
#define DESCR_UUID_CCCD 0x2902

static uint8_t char_value_get[] = "Hello from ESP32!";
static uint16_t notify_conn_id = 0;
static esp_gatt_if_t notify_gatt_if = 0;
static uint16_t get_char_handle = 0;
static QueueHandle_t cmd_queue = NULL;

#define CMD_DEVICE_LIST_REQUEST 0xFF
#define CMD_DEVICE_LIST_RESPONSE 0x01
#define CMD_RUN_LUA_SCRIPT 0x02
#define CMD_SET_DEVICE_STATE 0x03
#define CMD_ERROR_RESPONSE 0xFE

typedef struct {
    uint8_t cmd_type;
    uint16_t length;
    uint8_t data[256];
} ble_cmd_t;

static uint8_t lua_script_buffer[4096];
static uint16_t lua_script_length = 0;

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, 
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param);
static void gap_event_handler(esp_gap_ble_cb_event_t event, 
                             esp_ble_gap_cb_param_t *param);
static void serialized_device_list(uint8_t* buffer, size_t* length);
static void handle_client_write(uint8_t* value, uint16_t length);
static void process_lua_script(uint8_t* data, uint16_t length, bool is_last_chunk);

static void process_lua_script(uint8_t* data, uint16_t length, bool is_last_chunk) {
    if (lua_script_length + length > sizeof(lua_script_buffer)) {
        ESP_LOGE(GATTS_TAG, "Lua script too large");
        return;
    }

    memcpy(lua_script_buffer + lua_script_length, data, length);
    lua_script_length += length;

    if (is_last_chunk) {
        ESP_LOGI(GATTS_TAG, "Executing Lua script, length: %d", lua_script_length);

        lua_script_buffer[lua_script_length] = '\0';

        const char* error = brick_lua_vm_run((const char*)lua_script_buffer);
        if (error) {
            ESP_LOGE(GATTS_TAG, "Lua script error: %s", error);

            uint8_t response[256];
            response[0] = CMD_ERROR_RESPONSE;
            size_t error_len = strlen(error);
            if (error_len > 254) error_len = 254;
            memcpy(response + 1, error, error_len);
            response[error_len + 1] = '\0';

            if (notify_gatt_if != 0 && notify_conn_id != 0) {
                esp_ble_gatts_send_indicate(notify_gatt_if, notify_conn_id, 
                                           get_char_handle, error_len + 2, response, false);
            }
        }

        lua_script_length = 0;
    }
}

static void handle_client_write(uint8_t* value, uint16_t length) {
    if (length == 0) return;
    
    uint8_t cmd_type = value[0];
    ESP_LOGI(GATTS_TAG, "Received command: %d, length: %d", cmd_type, length);

    ble_cmd_t cmd;
    cmd.cmd_type = cmd_type;
    cmd.length = length - 1;

    if (cmd.length > 0 && cmd.length <= sizeof(cmd.data)) {
        memcpy(cmd.data, value + 1, cmd.length);
    }

    if (cmd_type == CMD_DEVICE_LIST_REQUEST) {
        cmd.length = 0;
    }

    if (cmd_type == CMD_RUN_LUA_SCRIPT && length >= 2) {
        cmd.cmd_type = CMD_RUN_LUA_SCRIPT;
        cmd.length = length - 1;
        memcpy(cmd.data, value + 1, cmd.length);
    }

    if (xQueueSend(cmd_queue, &cmd, 0) != pdTRUE) {
        ESP_LOGE(GATTS_TAG, "Command queue full");
    }
}

static void serialized_device_list(uint8_t* buffer, size_t* length) {
    size_t offset = 0;
    std::lock_guard<std::mutex> lock(device_map_mutex);
    
    for (const auto& entry : device_map) {
        const brick_device_t& device = entry.second;

        if (offset + 18 > 255) break;

        memcpy(buffer + offset, device.uuid.bytes, 16);
        offset += 16;

        buffer[offset++] = device.i2c_address;

        buffer[offset++] = device.online;
    }
    
    *length = offset;
}

void ble_server_task(void* param) {
    ble_cmd_t cmd;
    
    for (;;) {
        if (xQueueReceive(cmd_queue, &cmd, portMAX_DELAY)) {
            ESP_LOGI(GATTS_TAG, "Processing command: %d", cmd.cmd_type);
            
            switch (cmd.cmd_type) {
                case CMD_DEVICE_LIST_REQUEST: {
                    uint8_t response[256];
                    size_t length = 0;
                    response[0] = CMD_DEVICE_LIST_RESPONSE;
                    length = 1;

                    serialized_device_list(response + 1, &length);
                    length += 1;

                    if (notify_gatt_if != 0 && notify_conn_id != 0) {
                        esp_ble_gatts_send_indicate(notify_gatt_if, notify_conn_id, 
                                                   get_char_handle, length, response, false);
                    }
                    break;
                }
                
                case CMD_RUN_LUA_SCRIPT: {
                    bool is_last_chunk = (cmd.data[0] & 0x80) != 0;
                    uint8_t chunk_index = cmd.data[0] & 0x7F;
                    
                    ESP_LOGI(GATTS_TAG, "Lua script chunk %d, last: %d", chunk_index, is_last_chunk);

                    process_lua_script(cmd.data + 1, cmd.length - 1, is_last_chunk);
                    break;
                }
                
                case CMD_SET_DEVICE_STATE: {
                    ESP_LOGI(GATTS_TAG, "Set device state command received");
                    break;
                }
                
                default:
                    ESP_LOGW(GATTS_TAG, "Unknown command: %d", cmd.cmd_type);
                    break;
            }
        }
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                      esp_gatt_if_t gatts_if,
                                      esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            ESP_LOGI(GATTS_TAG, "GATT application registered, status: %d", param->reg.status);
            if (param->reg.status == ESP_GATT_OK) {
                esp_ble_gap_set_device_name("BrickLab ESP32");

                esp_ble_adv_data_t adv_data = {};
                adv_data.set_scan_rsp = false;
                adv_data.include_name = true;
                adv_data.include_txpower = true;
                adv_data.min_interval = 0x0006;
                adv_data.max_interval = 0x0010;
                adv_data.appearance = 0x00;
                adv_data.manufacturer_len = 0;
                adv_data.p_manufacturer_data = NULL;
                adv_data.service_data_len = 0;
                adv_data.p_service_data = NULL;
                adv_data.service_uuid_len = 0;
                adv_data.p_service_uuid = NULL;
                adv_data.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
                
                ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&adv_data));

                esp_gatt_srvc_id_t service_id;
                service_id.is_primary = true;
                service_id.id.inst_id = 0;
                service_id.id.uuid.len = ESP_UUID_LEN_16;
                service_id.id.uuid.uuid.uuid16 = SERVICE_UUID;
                
                ESP_ERROR_CHECK(esp_ble_gatts_create_service(gatts_if, &service_id, 8));
            }
            break;
        }
        
        case ESP_GATTS_CREATE_EVT: {
            ESP_LOGI(GATTS_TAG, "Service created, status: %d", param->create.status);
            if (param->create.status == ESP_GATT_OK) {
                uint16_t service_handle = param->create.service_handle;
                ESP_ERROR_CHECK(esp_ble_gatts_start_service(service_handle));

                esp_gatt_char_prop_t prop_get = 
                    ESP_GATT_CHAR_PROP_BIT_READ | 
                    ESP_GATT_CHAR_PROP_BIT_NOTIFY;
                
                uint8_t initial_value[] = "BrickLab Ready";
                
                esp_attr_value_t char_val_get = {
                    .attr_max_len = 512,
                    .attr_len = sizeof(initial_value) - 1,
                    .attr_value = initial_value,
                };
                
                esp_bt_uuid_t char_uuid_get = {
                    .len = ESP_UUID_LEN_16,
                    .uuid = {.uuid16 = CHAR_UUID_GET},
                };
                
                ESP_ERROR_CHECK(esp_ble_gatts_add_char(
                    service_handle, 
                    &char_uuid_get,
                    ESP_GATT_PERM_READ,
                    prop_get,
                    &char_val_get,
                    NULL));

                get_char_handle = service_handle + 1;

                esp_bt_uuid_t descr_uuid = {
                    .len = ESP_UUID_LEN_16,
                    .uuid = {.uuid16 = DESCR_UUID_CCCD},
                };
                
                esp_attr_value_t descr_val = {
                    .attr_max_len = 2,
                    .attr_len = 2,
                    .attr_value = (uint8_t[]){0x00, 0x00},
                };
                
                ESP_ERROR_CHECK(esp_ble_gatts_add_char_descr(
                    service_handle,
                    &descr_uuid,
                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                    &descr_val,
                    NULL));

                esp_gatt_char_prop_t prop_post = 
                    ESP_GATT_CHAR_PROP_BIT_WRITE | 
                    ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
                
                esp_attr_value_t char_val_post = {
                    .attr_max_len = 512,
                    .attr_len = 0,
                    .attr_value = NULL,
                };
                
                esp_bt_uuid_t char_uuid_post = {
                    .len = ESP_UUID_LEN_16,
                    .uuid = {.uuid16 = CHAR_UUID_POST},
                };
                
                ESP_ERROR_CHECK(esp_ble_gatts_add_char(
                    service_handle,
                    &char_uuid_post,
                    ESP_GATT_PERM_WRITE,
                    prop_post,
                    &char_val_post,
                    NULL));
            }
            break;
        }
        
        case ESP_GATTS_ADD_CHAR_EVT: {
            ESP_LOGI(GATTS_TAG, "Characteristic added, status: %d", param->add_char.status);
            break;
        }
        
        case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
            ESP_LOGI(GATTS_TAG, "Descriptor added, status: %d", param->add_char_descr.status);
            break;
        }
        
        case ESP_GATTS_CONNECT_EVT: {
            ESP_LOGI(GATTS_TAG, "Client connected, conn_id: %d", param->connect.conn_id);

            notify_conn_id = param->connect.conn_id;
            notify_gatt_if = gatts_if;

            esp_ble_conn_update_params_t conn_params;
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            conn_params.latency = 0;
            conn_params.max_int = 0x20;
            conn_params.min_int = 0x10;
            conn_params.timeout = 400;

            ESP_ERROR_CHECK(esp_ble_gap_update_conn_params(&conn_params));

            lua_script_length = 0;
            break;
        }
        
        case ESP_GATTS_DISCONNECT_EVT: {
            ESP_LOGI(GATTS_TAG, "Client disconnected, reason: %d", param->disconnect.reason);

            notify_conn_id = 0;

            esp_ble_adv_params_t adv_params = {
                .adv_int_min = 0x20,
                .adv_int_max = 0x40,
                .adv_type = ADV_TYPE_IND,
                .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
                .channel_map = ADV_CHNL_ALL,
                .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
            };
            
            ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&adv_params));
            break;
        }
        
        case ESP_GATTS_READ_EVT: {
            ESP_LOGI(GATTS_TAG, "GATT read request, handle: %d", param->read.handle);

            if (param->read.handle == get_char_handle) {
                uint8_t response[256];
                response[0] = CMD_DEVICE_LIST_RESPONSE;
                size_t length = 0;

                serialized_device_list(response + 1, &length);
                length += 1;

                esp_gatt_rsp_t rsp;
                memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
                rsp.attr_value.handle = param->read.handle;
                rsp.attr_value.len = length;
                memcpy(rsp.attr_value.value, response, length);
                
                esp_ble_gatts_send_response(gatts_if, param->read.conn_id,
                                           param->read.trans_id, ESP_GATT_OK, &rsp);
            }
            break;
        }
        
        case ESP_GATTS_WRITE_EVT: {
            ESP_LOGI(GATTS_TAG, "GATT write request, handle: %d, len: %d", 
                    param->write.handle, param->write.len);

            if (!param->write.is_prep && param->write.need_rsp) {
                esp_gatt_rsp_t rsp;
                memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
                rsp.attr_value.handle = param->write.handle;
                rsp.attr_value.len = param->write.len;
                memcpy(rsp.attr_value.value, param->write.value, param->write.len);
                
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                           param->write.trans_id, ESP_GATT_OK, &rsp);
            }

            if (param->write.len > 0) {
                handle_client_write(param->write.value, param->write.len);
            }

            if (param->write.handle == get_char_handle + 1) {
                if (param->write.len == 2) {
                    uint16_t descr_value = param->write.value[0] | (param->write.value[1] << 8);
                    ESP_LOGI(GATTS_TAG, "CCCD value: %d", descr_value);
                }
            }
            break;
        }
        
        default:
            break;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT: {
            esp_ble_adv_params_t adv_params = {
                .adv_int_min = 0x20,
                .adv_int_max = 0x40,
                .adv_type = ADV_TYPE_IND,
                .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
                .channel_map = ADV_CHNL_ALL,
                .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
            };
            ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&adv_params));
            break;
        }
        default:
            break;
    }
}

void ble_server_init() {
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_profile_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));

    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));

    cmd_queue = xQueueCreate(10, sizeof(ble_cmd_t));
    if (cmd_queue == NULL) {
        ESP_LOGE(GATTS_TAG, "Failed to create command queue");
    }
    
    ESP_LOGI(GATTS_TAG, "BLE server initialized");
}

extern "C" void app_main() {
    xTaskCreatePinnedToCore(brick_task_i2c_scan_devices,
                            "scan_devices",
                            4096,
                            nullptr,
                            5,
                            nullptr,
                            tskNO_AFFINITY
    );

    ble_server_init();

    xTaskCreatePinnedToCore(ble_server_task,
                            "ble_server",
                            4096,
                            nullptr,
                            5,
                            nullptr,
                            tskNO_AFFINITY
    );
        printf(brick_lua_vm_run(R"lua(
    local brick_labs = require("brick_lab")
    local DeviceRgb = brick_labs.DeviceRgb

    local uuid = "424C1010-0000-0000-87CB-CF832BF0EFAD"
    local delay_ms = 500

    -- Define the RGB color sequence
    local colors = {
      { red = 255, green = 255, blue = 255 },
      { red = 255, green = 0,   blue = 255 },
      { red = 0,   green = 0,   blue = 255 },
      { red = 0,   green = 255, blue = 0   },
      { red = 255, green = 255, blue = 0   },
      { red = 0,   green = 0,   blue = 0   }
    }

    -- Initialize the RGB device
    local device = DeviceRgb.new(uuid)

    -- Manually set colors in a loop
    while true do
      for _, color in ipairs(colors) do
        device:set_rgb(color)      -- OOP call to send RGB values
        delay(delay_ms)         -- global delay function (exposed from C)
      end
    end
    )lua"));

    brick_lua_vm_init();

    ESP_LOGI(GATTS_TAG, "Running test Lua script");
    const char* error = brick_lua_vm_run(R"lua(
        local brick_labs = require("brick_lab")
        print("Brick Lab Lua VM initialized!")
    )lua");
    
    if (error) {
        ESP_LOGE(GATTS_TAG, "Test Lua script error: %s", error);
    } else {
        ESP_LOGI(GATTS_TAG, "Test Lua script ran successfully");
    }
}