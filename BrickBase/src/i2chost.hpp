// i2chost.hpp
#ifndef I2CHOST_HPP
#define I2CHOST_HPP

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <driver/i2c.h>
#include <esp_log.h>
#include <string.h>

#include "brickapi.h"

#define I2C_MASTER_NUM         I2C_NUM_0
#define I2C_MASTER_SDA_IO      GPIO_NUM_16
#define I2C_MASTER_SCL_IO      GPIO_NUM_17
#define I2C_MASTER_FREQ_HZ     100000
#define I2C_TIMEOUT_MS         100

#define MAX_DEVICES 16

extern brick_device_t i2c_devices[MAX_DEVICES];
extern SemaphoreHandle_t i2c_devices_mutex;

#define I2C_DEVICES_MUTEX_START  do { if (xSemaphoreTake(i2c_devices_mutex, portMAX_DELAY)) {
#define I2C_DEVICES_MUTEX_END    xSemaphoreGive(i2c_devices_mutex); } } while (0);

void brick_i2c_init();
void brick_i2c_scan_devices();
void brick_task_i2c_scan_devices(void *pvParams);

#endif // I2CHOST_HPP
