#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "brick_i2c_host.hpp"
#include "brick_lua_vm.hpp"

extern "C" void app_main(void) {
    xTaskCreatePinnedToCore(brick_task_i2c_scan_devices,
                            "scan_devices",
                            4096,
                            nullptr,
                            5,
                            nullptr,
                            tskNO_AFFINITY
    );


    printf(brick_lua_vm_run(R"lua(
local brick_labs = require("brick_lab")
local DeviceRgb = brick_labs.DeviceRgb

local uuid = "424C1010-0000-0000-87CB-CF832BF0EFAD"
local delay_ms = 500

-- Define the RGB color sequence
local colors = {
  { red = 255, green = 255, blue = 255 },
  { red = 255, green = 0,   blue = 255 },
  { red = 0,   green = 0,   blue = 255 },
  { red = 0,   green = 255, blue = 0   },
  { red = 255, green = 255, blue = 0   },
  { red = 0,   green = 0,   blue = 0   }
}

-- Initialize the RGB device
local device = DeviceRgb.new(uuid)

-- Manually set colors in a loop
while true do
  for _, color in ipairs(colors) do
    device:set_rgb(color)      -- OOP call to send RGB values
    delay(delay_ms)         -- global delay function (exposed from C)
  end
end
)lua"));
}
