// main.cpp - Simple task kill implementation

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>

#include "brick_i2c_host.hpp"
#include "brick_lua_vm.hpp"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <vector>

#define GATTS_TAG "BLE_SERVER"
#define LUA_TAG "LUA_EXECUTOR"
#define GATTS_SERVICE_UUID    "0000FF00-0000-1000-8000-00805F9B34FB"
#define GATTS_CHAR_UUID_GET   "0000FF01-0000-1000-8000-00805F9B34FB"
#define GATTS_CHAR_UUID_POST  "0000FF02-0000-1000-8000-00805F9B34FB"

// BLE Protocol Commands
#define CMD_DEVICE_LIST_REQUEST 0xFF
#define CMD_DEVICE_LIST_RESPONSE 0x01
#define CMD_RUN_LUA_SCRIPT 0x02
#define CMD_ERROR_RESPONSE 0xFE

// Lua execution task configuration
#define LUA_TASK_STACK_SIZE 8192
#define LUA_TASK_PRIORITY 3

// Global BLE characteristics
BLECharacteristic *pCharacteristicGet = nullptr;
BLECharacteristic *pCharacteristicPost = nullptr;

// Lua execution system
TaskHandle_t luaTaskHandle = nullptr;
std::vector<char> currentLuaScript; // Protected by task recreation

// Simple packet structure
struct BlePacket {
    uint8_t command;
    std::vector<uint8_t> data;

    BlePacket(uint8_t cmd) : command(cmd) {
    }

    BlePacket(uint8_t cmd, const uint8_t *payload, size_t len)
        : command(cmd), data(payload, payload + len) {
    }
};

/**
 * Send response back to BLE client
 */
void sendBleResponse(uint8_t responseType, const std::vector<uint8_t> &data = {}) {
    if (!pCharacteristicGet) {
        ESP_LOGE(GATTS_TAG, "GET characteristic not available for response");
        return;
    }

    std::vector<uint8_t> response;
    response.reserve(1 + data.size());
    response.push_back(responseType);
    response.insert(response.end(), data.begin(), data.end());

    pCharacteristicGet->setValue(response.data(), response.size());
    pCharacteristicGet->notify();

    ESP_LOGI(GATTS_TAG, "Sent response 0x%02X (%zu bytes)", responseType, response.size());
}

/**
 * Send error response
 */
void sendErrorResponse(const char *errorMessage) {
    std::vector<uint8_t> errorData(errorMessage, errorMessage + strlen(errorMessage));
    sendBleResponse(CMD_ERROR_RESPONSE, errorData);
    ESP_LOGE(GATTS_TAG, "Sent error: %s", errorMessage);
}

/**
 * Send device list response
 */
void sendDeviceList() {
    std::lock_guard<std::mutex> lock(device_map_mutex);

    // Calculate response size: 18 bytes per device
    size_t deviceCount = device_map.size();
    std::vector<uint8_t> deviceData;
    deviceData.reserve(deviceCount * 18);

    for (const auto &[uuid, device]: device_map) {
        // Add 16-byte UUID
        deviceData.insert(deviceData.end(), device.uuid.bytes, device.uuid.bytes + 16);

        // Add I2C address
        deviceData.push_back(device.i2c_address);

        // Add online status
        deviceData.push_back(device.online ? 1 : 0);
    }

    sendBleResponse(CMD_DEVICE_LIST_RESPONSE, deviceData);
    ESP_LOGI(GATTS_TAG, "Sent device list: %zu devices", deviceCount);
}

/**
 * Simple Lua execution task - just runs the script and exits
 */
void luaExecutionTask(void *parameter) {
    ESP_LOGI(LUA_TAG, "Lua execution task started (%zu bytes)", currentLuaScript.size() - 1);
    ESP_LOGI(LUA_TAG, "Script preview: %.100s%s",
             currentLuaScript.data(),
             currentLuaScript.size() > 101 ? "..." : "");

    // Execute Lua code safely
    const char *error = brick_lua_vm_run(currentLuaScript.data());

    if (error) {
        ESP_LOGE(LUA_TAG, "Lua execution error: %s", error);
        sendErrorResponse(error);
    } else {
        ESP_LOGI(LUA_TAG, "Lua script executed successfully");
    }

    ESP_LOGI(LUA_TAG, "Lua execution task finished");

    // Task self-destructs
    luaTaskHandle = nullptr;
    vTaskDelete(nullptr);
}

/**
 * Execute new Lua script - kills any running task and starts fresh
 */
void executeLuaScript(const std::vector<uint8_t> &data) {
    if (data.empty()) {
        ESP_LOGE(GATTS_TAG, "Empty Lua script received");
        sendErrorResponse("Empty Lua script");
        return;
    }

    // Check script size limit
    if (data.size() > 8192) {
        ESP_LOGE(GATTS_TAG, "Lua script too large: %zu bytes", data.size());
        sendErrorResponse("Script too large (max 8KB)");
        return;
    }

    // Kill any running Lua task immediately
    if (luaTaskHandle != nullptr) {
        ESP_LOGW(LUA_TAG, "Killing existing Lua task for new script");
        vTaskDelete(luaTaskHandle);
        luaTaskHandle = nullptr;

        // Small delay to ensure task cleanup
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Reset Lua VM to clean state
    brick_lua_vm_reset();

    // Prepare script data (null-terminated)
    currentLuaScript.resize(data.size() + 1);
    memcpy(currentLuaScript.data(), data.data(), data.size());
    currentLuaScript[data.size()] = '\0';

    // Create new task to execute script
    BaseType_t result = xTaskCreatePinnedToCore(
        luaExecutionTask,
        "lua_executor",
        LUA_TASK_STACK_SIZE,
        nullptr,
        LUA_TASK_PRIORITY,
        &luaTaskHandle,
        tskNO_AFFINITY
    );

    if (result != pdPASS) {
        ESP_LOGE(LUA_TAG, "Failed to create Lua execution task");
        sendErrorResponse("Failed to create Lua task");
        luaTaskHandle = nullptr;
        return;
    }

    ESP_LOGI(GATTS_TAG, "New Lua script started (%zu bytes)", data.size());
}

/**
 * Process incoming BLE commands
 */
void processCommand(const BlePacket &packet) {
    switch (packet.command) {
        case CMD_DEVICE_LIST_REQUEST:
            ESP_LOGI(GATTS_TAG, "Device list requested");
            sendDeviceList();
            break;

        case CMD_RUN_LUA_SCRIPT:
            ESP_LOGI(GATTS_TAG, "Lua script command received (%zu bytes)", packet.data.size());
            executeLuaScript(packet.data); // Kill old task and start new one
            break;

        default:
            ESP_LOGW(GATTS_TAG, "Unknown command: 0x%02X", packet.command);
            sendErrorResponse("Unknown command");
            break;
    }
}

class BleServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) override {
        ESP_LOGI(GATTS_TAG, "Client connected");
    }

    void onDisconnect(BLEServer *pServer) override {
        ESP_LOGI(GATTS_TAG, "Client disconnected, restarting advertising");
        BLEDevice::startAdvertising();
    }
};

class BleCharacteristicCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        std::string value = pCharacteristic->getValue();

        if (value.empty()) {
            ESP_LOGW(GATTS_TAG, "Empty write received");
            return;
        }

        ESP_LOGI(GATTS_TAG, "Received %zu bytes on BLE thread", value.length());

        // Parse packet quickly on BLE thread
        const uint8_t *data = reinterpret_cast<const uint8_t *>(value.data());
        uint8_t command = data[0];

        BlePacket packet(command);
        if (value.length() > 1) {
            packet.data.assign(data + 1, data + value.length());
        }

        // Process command (Lua execution will kill/recreate task)
        processCommand(packet);
    }
};

/**
 * Initialize BLE server
 */
void initializeBLE() {
    ESP_LOGI(GATTS_TAG, "Initializing BLE server");

    BLEDevice::init("BrickLab Base");

    // Create server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new BleServerCallbacks());

    // Create service
    BLEService *pService = pServer->createService(GATTS_SERVICE_UUID);

    // Create GET characteristic (read/notify)
    pCharacteristicGet = pService->createCharacteristic(
        GATTS_CHAR_UUID_GET,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristicGet->addDescriptor(new BLE2902());

    // Create POST characteristic (write)
    pCharacteristicPost = pService->createCharacteristic(
        GATTS_CHAR_UUID_POST,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pCharacteristicPost->setCallbacks(new BleCharacteristicCallbacks());

    // Start service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(GATTS_SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();

    ESP_LOGI(GATTS_TAG, "BLE server started, advertising as 'BrickLab Base'");
}

extern "C" void app_main() {
    ESP_LOGI("MAIN", "Starting BrickLab system");

    // Initialize Lua VM
    brick_lua_vm_init();
    ESP_LOGI("MAIN", "Lua VM initialized");

    // Start I2C scanning task
    xTaskCreatePinnedToCore(
        brick_task_i2c_scan_devices,
        "scan_devices",
        4096,
        nullptr,
        5,
        nullptr,
        tskNO_AFFINITY
    );
    ESP_LOGI("MAIN", "I2C scanner started");

    /*while (1) {
        brick_device_t *device = brick_i2c_get_device_uuid("424c2002-0000-0000-9374-675a4712a023");

        if (device) {
            brick_uuid_t uuid = device->uuid; // or device->uuid, depending on implementation
            if (device_map.contains(uuid)) {
                brick_command_t command = {
                    .command = CMD_STEPPER_MOVE,
                    .device = device
                };

                command.device->impl.stepper_motor.motor = 0b00001111;

                brick_i2c_send_device_command(&command);
                printf("Move\n");
            }
        }


    }*/
    // Initialize BLE
    initializeBLE();

    ESP_LOGI("MAIN", "BrickLab system ready");

    // Main loop - system monitoring
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));

        // Log system status
        const char *luaStatus = luaTaskHandle ? "RUNNING" : "IDLE";
        ESP_LOGI("MAIN", "System running - %zu devices, Lua: %s",
                 device_map.size(), luaStatus);
    }
}
