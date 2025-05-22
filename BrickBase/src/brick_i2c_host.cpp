#include "brick_i2c_host.hpp"

std::map<brick_uuid_t, brick_device_t, uuid_less> device_map;
std::mutex device_map_mutex;

bool uuid_less::operator()(const brick_uuid_t &a, const brick_uuid_t &b) const {
    return std::memcmp(a.bytes, b.bytes, 16) < 0;
}

void brick_i2c_init() {
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
            std::lock_guard<std::mutex> lock(device_map_mutex);
            for (auto &[uuid, device]: device_map) {
                if (device.online && device.i2c_address == addr) {
                    device.online = 0;
                    ESP_LOGW("brick_i2c_scan_devices", "Device at 0x%02X removed", addr);
                    break;
                }
            }
            continue;
        }

        uint8_t cmd_ident = CMD_IDENTIFY;
        uint8_t uuid_buf[16] = {0};
        esp_err_t res = i2c_master_write_read_device(
            I2C_MASTER_NUM, addr, &cmd_ident, 1, uuid_buf, 16,
            pdMS_TO_TICKS(I2C_TIMEOUT_MS)
        );

        if (res == ESP_OK && brick_uuid_valid(uuid_buf)) {
            brick_uuid_t uuid;
            std::memcpy(uuid.bytes, uuid_buf, 16);
            std::lock_guard lock(device_map_mutex);
            auto it = device_map.find(uuid);

            if (it != device_map.end()) {
                it->second.online = 1;
            } else {
                brick_device_t new_dev = brick_get_device_specs_from_uuid(uuid_buf);
                new_dev.i2c_address = addr;
                new_dev.online = 1;
                device_map[uuid] = new_dev;

                ESP_LOGI("brick_i2c_scan_devices", "Device found at 0x%02X (%s)", addr, brick_device_type_str(new_dev.device_type));
                brick_print_uuid(&new_dev.uuid);
            }
        } else {
            ESP_LOGW("brick_i2c_scan_devices", "Failed to read UUID from 0x%02X", addr);
        }
    }
}

void brick_task_i2c_scan_devices(void *pvParams) {
    ESP_LOGI("brick_task_i2c_scan_devices", "I\302\262C initialized on SDA=GPIO%d, SCL=GPIO%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);\
    brick_i2c_init();

    while (true) {
        brick_i2c_scan_devices();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

brick_device_t *brick_i2c_get_device_uuid(const char *uuid_str) {
    if (!uuid_str || std::strlen(uuid_str) != 36) return nullptr;

    brick_uuid_t uuid;
    int byte_index = 0;

    for (int i = 0; i < 36 && byte_index < 16;) {
        if (uuid_str[i] == '-') {
            ++i; // skip dash
            continue;
        }

        auto hex_char_to_int = [](char c) -> int {
            if ('0' <= c && c <= '9') return c - '0';
            if ('a' <= c && c <= 'f') return c - 'a' + 10;
            if ('A' <= c && c <= 'F') return c - 'A' + 10;
            return -1;
        };

        int high = hex_char_to_int(uuid_str[i]);
        int low = hex_char_to_int(uuid_str[i + 1]);
        if (high < 0 || low < 0) return nullptr;

        uuid.bytes[byte_index++] = static_cast<uint8_t>((high << 4) | low);
        i += 2;
    }

    if (byte_index != 16) return nullptr;

    std::lock_guard<std::mutex> lock(device_map_mutex);
    for (auto &x: device_map) {
        if (std::memcmp(x.second.uuid.bytes, uuid.bytes, sizeof(uuid.bytes)) == 0) {
            return &x.second;
        }
    }

    return nullptr;
}

brick_device_t *brick_i2c_get_device_uuid(brick_uuid_t uuid) {
    std::lock_guard<std::mutex> lock(device_map_mutex);

    for (auto &x: device_map) {
        if (std::memcmp(x.second.uuid.bytes, uuid.bytes, sizeof(uuid.bytes)) == 0) {
            return &x.second;
        }
    }

    return nullptr;
}


bool brick_i2c_send_device_command(const brick_command_t *cmd) {
    if (!cmd || !cmd->device) {
        ESP_LOGE("brick_i2c_send_device_command", "Null device or command pointer");
        return false;
    }

    brick_device_t *device = cmd->device;
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (device->i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, static_cast<uint8_t>(cmd->command), true);

    // Optional payload based on command type
    switch (cmd->command) {
        case CMD_LED:
            i2c_master_write(cmd_handle,
                             reinterpret_cast<const uint8_t *>(&device->impl.led_single),
                             sizeof(device->impl.led_single),
                             true
            );
            break;

        case CMD_LED_DOUBLE:
            i2c_master_write(cmd_handle,
                             reinterpret_cast<const uint8_t *>(&device->impl.led_double),
                             sizeof(device->impl.led_double),
                             true
            );
            break;

        case CMD_LED_RGB:
            i2c_master_write(cmd_handle,
                             reinterpret_cast<const uint8_t *>(&device->impl.led_rgb),
                             sizeof(device->impl.led_rgb),
                             true
            );
            break;

        case CMD_SERVO_SET_ANGLE:
            i2c_master_write(cmd_handle,
                             reinterpret_cast<const uint8_t *>(&device->impl.servo_180),
                             sizeof(device->impl.servo_180),
                             true
            );
            break;

        case CMD_STEPPER_MOVE:
            // Add support if there's a struct for stepper move
            break;

        case CMD_SENSOR_GET_CM:
            // Usually read command, not write — might not need payload
            break;

        default:
            ESP_LOGW("brick_i2c_send_device_command", "Unhandled command type: 0x%02X", cmd->command);
            break;
    }

    i2c_master_stop(cmd_handle);
    esp_err_t res = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd_handle);

    if (res != ESP_OK) {
        ESP_LOGE("brick_i2c_send_device_command", "Failed to send command 0x%02X to device at 0x%02X", cmd->command, device->i2c_address);
    }

    return res == ESP_OK;
}
