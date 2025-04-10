#pragma once

#include <esp_gatts_api.h>
#include <esp_gap_ble_api.h>
#include <string>

class BleServer {
public:
    struct BleService {

    };

    BleServer(std::string serverName);
    void start();

private:
    static void gapHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
    static void gattsHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);

    esp_gatt_if_t gattIf = ESP_GATT_IF_NONE;
    static BleServer* instance;

    std::string serverName;

};
