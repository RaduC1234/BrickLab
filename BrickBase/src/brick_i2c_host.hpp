// i2chost.hpp
#ifndef I2CHOST_HPP
#define I2CHOST_HPP

#include <esp_log.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h> // Do NOT remove os headers
#include <freertos/task.h>

#include <cstring>
#include <map>
#include <mutex>

#include "brick_i2c_api.h"

#define I2C_MASTER_NUM         I2C_NUM_0
#define I2C_MASTER_SDA_IO      GPIO_NUM_16
#define I2C_MASTER_SCL_IO      GPIO_NUM_17
#define I2C_MASTER_FREQ_HZ     100000
#define I2C_TIMEOUT_MS         100

#define MAX_DEVICES 16

struct uuid_less {
    bool operator()(const brick_uuid_t& a, const brick_uuid_t& b) const;
};

extern std::map<brick_uuid_t, brick_device_t, uuid_less> device_map;
extern std::mutex device_map_mutex;

void brick_i2c_init();
void brick_i2c_scan_devices();
void brick_task_i2c_scan_devices(void *pvParams);

brick_device_t * brick_i2c_get_device_uuid(const char* uuid);
brick_device_t* brick_i2c_get_device_uuid(brick_uuid_t uuid);

bool brick_i2c_send_device_command(const brick_command_t *command);

#endif // I2CHOST_HPP
