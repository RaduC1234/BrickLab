// ble_server.hpp
#pragma once

#include <esp_gatts_api.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ble_gatt_api.hpp"
#include "esp_gap_ble_api.h"

class BleServer {
public:
    explicit BleServer(std::string serverName);
    void start();
    void addService(const GattService &service);

private:
    std::string serverName;
    esp_gatt_if_t gattIf{};

    struct PendingService {
        std::vector<esp_gatts_attr_db_t> attrTable;
        std::vector<std::unique_ptr<uint8_t>> dynamicProps;
        std::vector<std::unique_ptr<uint8_t[]>> dynamicValues;
        std::map<uint16_t, std::function<std::vector<uint8_t>()>> onRead;
        std::map<uint16_t, std::function<void(const std::vector<uint8_t>&)>> onWrite;
        std::array<uint8_t, 16> uuid;
    };

    static BleServer *instance;
    static uint8_t serviceInstanceId;

    std::vector<PendingService> pendingServices;

    static void gapHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    static void gattsHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
    static void registerService(const GattService &service, esp_gatt_if_t gatts_if);
};
