// std
#include <cstdio>
#include <cstring>

// i2c & lua
#include "brick_i2c_host.hpp"
#include "brick_lua_vm.hpp"

// ble
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// sys
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#define GATTS_TAG "BLE_SERVER"
#define GATTS_SERVICE_UUID_A    0x00FF
#define GATTS_CHAR_UUID_GET     0xFF01
#define GATTS_CHAR_UUID_POST    0xFF02
#define GATTS_DESCR_UUID_CCCD   0x2902

// BLE Protocol Commands
#define CMD_DEVICE_LIST_REQUEST 0xFF
#define CMD_DEVICE_LIST_RESPONSE 0x01
#define CMD_RUN_LUA_SCRIPT 0x02
#define CMD_SET_DEVICE_STATE 0x03
#define CMD_ERROR_RESPONSE 0xFE


typedef enum {
} brick_bluetooth_device_cmd_t;

extern "C" void app_main() {
    // Initialize Lua VM first
    brick_lua_vm_init();

    // Start I2C scanning task
    xTaskCreatePinnedToCore(brick_task_i2c_scan_devices,
                            "scan_devices",
                            4096,
                            nullptr,
                            5,
                            nullptr,
                            tskNO_AFFINITY);

    BLEDevice::init("Bricklab Base");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService("0000FF00-0000-1000-8000-00805F9B34FB");
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
        "0000FF01-0000-1000-8000-00805F9B34FB",
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE
    );

    pCharacteristic->setValue("Hello World");
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("0000FF00-0000-1000-8000-00805F9B34FB");
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    printf("Characteristic defined! Now you can read it in your phone!\n");
}
