#include <esp_log.h>

#include "ble_server.hpp"
#include <esp_log_level.h>

extern "C" void app_main() {
    esp_log_level_set("*", ESP_LOG_INFO);

    BleServer server("Ble_Test_Server");

    GattService exampleService = GattServiceBuilder()
            .uuid({0xde, 0xc0, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12})
            .addCharacteristic(GattCharacteristicBuilder()
                .uuid({ 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x9a, 0xbc, 0xde, 0xf0, 0x12, 0x34, 0xc0, 0xde }) // <-- Must be called!
                .properties(ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY)
                .permissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE)
                .value({0x42})
                .onRead([] {
                    ESP_LOGI("CHAR", "onRead called");
                    return std::vector<uint8_t>{0x10};
                })
                .onWrite([](const std::vector<uint8_t> &data) {
                    ESP_LOGI("CHAR", "Message");
                    if (!data.empty())
                        ESP_LOGI("CHAR", "onWrite: value = %02x", data[0]);
                })
                .build())
            .build();

    // here the uuid is invalid before the service creation the builder is fauilty
    ESP_LOG_BUFFER_HEX("UUID", exampleService.characteristics[0].uuid.data(), 16);

    server.addService(exampleService);
    server.start();
}
