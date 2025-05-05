// std
#include <cstdio>
#include <cstring>

// i2c
#include "i2chost.hpp"

// ble
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"

// sys
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

BLEServer *server = nullptr;
BLECharacteristic *characteristic = nullptr;
bool device_connected = false;

#define SERVICE_UUID "323e12af-6bb4-48cf-813d-3f7f8aa9f72f"
#define CHARACTERISTIC_UUID "5f44ed80-420a-4684-914c-af625af2b856"

class BrickBleServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) override {
        device_connected = true;
    }

    void onDisconnect(BLEServer *pServer) override {
        device_connected = false;
    }
};

extern "C" void app_main() {

    // create i2c scan runtime
    brick_i2c_init();
    xTaskCreatePinnedToCore(
        brick_task_i2c_scan_devices,
        "brick_task_i2c_scan_devices",
        2048,
        nullptr,
        1,
        nullptr,
        tskNO_AFFINITY
    );

    //create bluetooth runtime
    /*BLEDevice::init("BrickBase");

    server = BLEDevice::createServer();
    server->setCallbacks(new BrickBleServerCallbacks());

    BLEService *service = server->createService(SERVICE_UUID);

    characteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );

    characteristic->addDescriptor(new BLE2902());

    service->start();*/


    //create lua runtime
}
