#include "ble_server.hpp"

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gatts_api.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

#define TAG "BLE_SERVER"
#define GATTS_APP_ID 0

inline void createSimpleService(esp_gatt_if_t gatts_if, uint16_t app_id) {
    // 128-bit Service UUID (little-endian)
    static const uint8_t SERVICE_UUID[16] = {
        0xde, 0xc0, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a,
        0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12
    };

    // 128-bit Characteristic UUID (little-endian)
    static const uint8_t CHAR_UUID[16] = {
        0x01, 0x00, 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45,
        0x23, 0x01, 0xf6, 0xe5, 0xd4, 0xc3, 0xb2, 0xa1
    };

    static uint8_t char_value = 42; // Default value

    static uint16_t esp_gatt_uuid_pri_service = ESP_GATT_UUID_PRI_SERVICE;
    static uint16_t esp_gatt_uuid_char_declare = ESP_GATT_UUID_CHAR_DECLARE;
    static uint16_t esp_gatt_uuid_char_client_config = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
    static uint8_t char_props = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
    static uint8_t cccd_val[2] = {0x00, 0x00};

    esp_gatts_attr_db_t gatt_db[4] = {
        // [0] Primary Service
        {
            {ESP_GATT_AUTO_RSP},
            {
                ESP_UUID_LEN_16,
                reinterpret_cast<uint8_t *>(&esp_gatt_uuid_pri_service),
                ESP_GATT_PERM_READ,
                sizeof(SERVICE_UUID),
                sizeof(SERVICE_UUID),
                const_cast<uint8_t *>(SERVICE_UUID)
            }
        },
        // [1] Characteristic Declaration
        {
            {ESP_GATT_AUTO_RSP},
            {
                ESP_UUID_LEN_16,
                reinterpret_cast<uint8_t *>(&esp_gatt_uuid_char_declare),
                ESP_GATT_PERM_READ,
                sizeof(uint8_t),
                sizeof(uint8_t),
                &char_props
            }
        },
        // [2] Characteristic Value
        {
            {ESP_GATT_AUTO_RSP},
            {
                ESP_UUID_LEN_128,
                const_cast<uint8_t *>(CHAR_UUID),
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                512,
                sizeof(char_value),
                &char_value
            }
        },
        // [3] CCCD (optional for notify support, safe to include)
        {
            {ESP_GATT_AUTO_RSP},
            {
                ESP_UUID_LEN_16,
                reinterpret_cast<uint8_t *>(&esp_gatt_uuid_char_client_config),
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                2,
                2,
                cccd_val
            }
        }
    };

    esp_err_t err = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, 4, app_id);
    ESP_LOGI(TAG, "create_attr_tab returned: %s", esp_err_to_name(err));
}


BleServer *BleServer::instance = nullptr;

BleServer::BleServer(std::string serverName) : serverName(std::move(serverName)) {
}

void BleServer::start() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    BleServer::instance = this;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(BleServer::gapHandler));
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(BleServer::gattsHandler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(GATTS_APP_ID));
}

void BleServer::gapHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT) {
        esp_ble_adv_params_t adv_params = {
            .adv_int_min = 0x20,
            .adv_int_max = 0x40,
            .adv_type = ADV_TYPE_IND,
            .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
            .channel_map = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
        };
        esp_ble_gap_start_advertising(&adv_params);
    }
}

void BleServer::gattsHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (!BleServer::instance) return;

    switch (event) {
        case ESP_GATTS_REG_EVT: {
            BleServer::instance->gattIf = gatts_if;
            esp_ble_gap_set_device_name(instance->serverName.c_str());

            esp_ble_adv_data_t adv_data = {
                .set_scan_rsp = false,
                .include_name = true,
                .include_txpower = false,
                .min_interval = 0x20,
                .max_interval = 0x40,
                .appearance = 0x00,
                .manufacturer_len = 0,
                .p_manufacturer_data = nullptr,
                .service_data_len = 0,
                .p_service_data = nullptr,
                .service_uuid_len = 0,
                .p_service_uuid = nullptr,
                .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
            };

            esp_ble_gap_config_adv_data(&adv_data);

            createSimpleService(gatts_if, GATTS_APP_ID);
            break;
        }
        case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
            if (param->add_attr_tab.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "Failed to create attr table: 0x%x", param->add_attr_tab.status);
                return;
            }

            esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
            break;
        }


        default:
            break;
    }
}
