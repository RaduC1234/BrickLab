#include "ble_server.hpp"

#include <cstring>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gatts_api.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

#define TAG "BLE_SERVER"
#define GATTS_APP_ID 0

BleServer *BleServer::instance = nullptr;
uint8_t BleServer::serviceInstanceId = 0;

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

void BleServer::addService(const GattService &service) {
    services.push_back(service);
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

            for (auto &sev: instance->services) {
                registerService(sev, gatts_if);
            }
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
        case ESP_GATTS_READ_EVT: {
            auto it = instance->readCallbacks.find(param->read.handle);
            if (it != instance->readCallbacks.end()) {
                auto value = it->second();
                esp_gatt_rsp_t rsp{};
                rsp.attr_value.handle = param->read.handle;
                rsp.attr_value.len = value.size();
                memcpy(rsp.attr_value.value, value.data(), value.size());

                esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            }
            break;
        }
        case ESP_GATTS_WRITE_EVT: {
            auto it = instance->writeCallbacks.find(param->write.handle);
            if (it != instance->writeCallbacks.end()) {
                std::vector data(param->write.value, param->write.value + param->write.len);
                it->second(data);
            }

            if (!param->write.is_prep && param->write.need_rsp) {
                esp_ble_gatts_send_response(
                    gatts_if,
                    param->write.conn_id,
                    param->write.trans_id,
                    ESP_GATT_OK,
                    nullptr);
            }
            break;
        }
        case ESP_GATTS_DISCONNECT_EVT: { // start advertising after an client disconnect
            ESP_LOGI(TAG, "Client disconnected, restarting advertising");

            esp_ble_adv_params_t adv_params = {
                .adv_int_min = 0x20,
                .adv_int_max = 0x40,
                .adv_type = ADV_TYPE_IND,
                .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
                .channel_map = ADV_CHNL_ALL,
                .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
            };

            esp_ble_gap_start_advertising(&adv_params);
            break;
        }

        default:
            break;
    }
}

void BleServer::registerService(const GattService &service, esp_gatt_if_t gatts_if) {
    const auto &svc_uuid = service.uuid;
    const auto &chars = service.characteristics;

    std::vector<esp_gatts_attr_db_t> table;

    // Service Declaration
    static uint16_t primary_uuid = ESP_GATT_UUID_PRI_SERVICE;
    table.push_back({
        {ESP_GATT_AUTO_RSP},
        {
            ESP_UUID_LEN_16, reinterpret_cast<uint8_t*>(&primary_uuid),
            ESP_GATT_PERM_READ,
            16, 16, const_cast<uint8_t*>(svc_uuid.data())
        }
    });

    for (const auto& ch : chars) {
        static uint16_t char_decl_uuid = ESP_GATT_UUID_CHAR_DECLARE;
        static uint8_t props = ch.properties;

        ESP_LOGI("test", "%d", ch.properties);
        ESP_LOGI("test", "%p", reinterpret_cast<uint8_t*>(&props));

        // Characteristic Declaration
        table.push_back({
            {ESP_GATT_AUTO_RSP},
            {
                ESP_UUID_LEN_16,
                reinterpret_cast<uint8_t*>(&char_decl_uuid), // here is should have been ptr to the uuid bvalue not ESP_GATT_UUID_CHAR DECLARE?
                ESP_GATT_PERM_READ,
                sizeof(uint8_t), sizeof(uint8_t),
                &props
            }
        });

        // Characteristic Value
        table.push_back({
            {ESP_GATT_AUTO_RSP},
            {
                ESP_UUID_LEN_128,
                const_cast<uint8_t*>(ch.uuid.data()),
                ch.permissions,
                512,
                static_cast<uint16_t>(ch.value.size()),
                const_cast<uint8_t*>(ch.value.data())
            }
        });

        // Optional: Descriptors (not shown here for brevity)
    }

    esp_err_t err = esp_ble_gatts_create_attr_tab(table.data(), gatts_if, table.size(), serviceInstanceId);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create attribute table: %s", esp_err_to_name(err));
        return;
    }

    serviceInstanceId++;
}

