#include "ble_server.hpp"
#include <esp_log_level.h>

extern "C" void app_main() {
    esp_log_level_set("*", ESP_LOG_INFO);

    BleServer server("Ble_Test_Server");
    server.start();
}
