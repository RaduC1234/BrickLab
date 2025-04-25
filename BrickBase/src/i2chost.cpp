#include "i2chost.hpp"

brick_device_t i2c_devices[MAX_DEVICES];
SemaphoreHandle_t i2c_devices_mutex;

void brick_i2c_init() {
    i2c_devices_mutex = xSemaphoreCreateMutex();
    assert(i2c_devices_mutex != nullptr);

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
}

void brick_i2c_scan_devices() {
    for (int addr = 0x08; addr <= 0x77; ++addr) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ack = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
        i2c_cmd_link_delete(cmd);

        if (ack != ESP_OK) {
            I2C_DEVICES_MUTEX_START
            for (auto & i2c_device : i2c_devices) {
                if (i2c_device.online && i2c_device.i2c_address == addr) {
                    i2c_device.online = 0;
                    ESP_LOGW("brick_i2c_scan_devices", "Device at 0x%02X removed", addr);
                    break;
                }
            }
            I2C_DEVICES_MUTEX_END
            continue;
        }

        uint8_t cmd_ident = CMD_IDENTIFY;
        uint8_t uuid_buf[16] = {0};
        esp_err_t res = i2c_master_write_read_device(
            I2C_MASTER_NUM, addr, &cmd_ident, 1, uuid_buf, 16,
            pdMS_TO_TICKS(I2C_TIMEOUT_MS)
        );

        if (res == ESP_OK && brick_uuid_valid(uuid_buf)) {
            bool already_known = false;
            I2C_DEVICES_MUTEX_START
            for (auto & i2c_device : i2c_devices) {
                if (i2c_device.online && memcmp(i2c_device.uuid.bytes, uuid_buf, 16) == 0) {
                    i2c_device.online = 1;
                    already_known = true;
                    break;
                }
            }

            if (!already_known) {
                for (auto & i2c_device : i2c_devices) {
                    if (!i2c_device.online) {
                        brick_device_t new_dev = get_device_specs_from_uuid(uuid_buf);
                        new_dev.i2c_address = addr;
                        new_dev.online = 1;
                        i2c_device = new_dev;
                        ESP_LOGI("brick_i2c_scan_devices", "Device found at 0x%02X (%s)", addr, brick_device_type_str(new_dev.device_type));
                        brick_print_uuid(&new_dev.uuid);
                        break;
                    }
                }
            }
            I2C_DEVICES_MUTEX_END
        } else {
            ESP_LOGW("brick_i2c_scan_devices", "Failed to read UUID from 0x%02X", addr);
        }
    }
}

void brick_task_i2c_scan_devices(void *pvParams) {
    ESP_LOGI("brick_task_i2c_scan_devices", "I²C initialized on SDA=GPIO%d, SCL=GPIO%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    while (true) {
        brick_i2c_scan_devices();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
