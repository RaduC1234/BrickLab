#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "brick_i2c_host.hpp"
#include "brick_lua_vm.hpp"


static void brick_task_rgb_cycle(void *pvParams) {
    const TickType_t delay = pdMS_TO_TICKS(500);

    while (true) {
        brick_device_t *dev = brick_i2c_get_device_uuid("424C1010-0000-0000-87CB-CF832BF0EFAD");

        if (dev && dev->device_type == LED_RGB) {
            brick_command_t cmd = {
                .command = CMD_LED_RGB,
                .device = dev
            };

            dev->impl.led_rgb.red = 255;
            dev->impl.led_rgb.blue = 255;
            dev->impl.led_rgb.green = 255;
            brick_i2c_send_device_command(&cmd);
            vTaskDelay(delay);

            dev->impl.led_rgb.red = 255;
            dev->impl.led_rgb.blue = 255;
            dev->impl.led_rgb.green = 0;
            brick_i2c_send_device_command(&cmd);
            vTaskDelay(delay);

            dev->impl.led_rgb.red = 0;
            dev->impl.led_rgb.blue = 255;
            dev->impl.led_rgb.green = 0;
            brick_i2c_send_device_command(&cmd);
            vTaskDelay(delay);

            dev->impl.led_rgb.red = 0;
            dev->impl.led_rgb.blue = 0;
            dev->impl.led_rgb.green = 255;
            brick_i2c_send_device_command(&cmd);
            vTaskDelay(delay);

            dev->impl.led_rgb.red = 255;
            dev->impl.led_rgb.blue = 0;
            dev->impl.led_rgb.green = 255;
            brick_i2c_send_device_command(&cmd);
            vTaskDelay(delay);

            dev->impl.led_rgb.red = 0;
            dev->impl.led_rgb.blue = 0;
            dev->impl.led_rgb.green = 0;
            brick_i2c_send_device_command(&cmd);
            vTaskDelay(delay);
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}


extern "C" void app_main(void) {
    brick_i2c_init();

    xTaskCreatePinnedToCore(brick_task_i2c_scan_devices,
                            "scan_devices",
                            4096,
                            nullptr,
                            5,
                            nullptr,
                            tskNO_AFFINITY
    );

    xTaskCreatePinnedToCore(brick_task_rgb_cycle,
                            "rgb_cycle",
                            4096,
                            nullptr,
                            5,
                            nullptr,
                            tskNO_AFFINITY
    );
}
