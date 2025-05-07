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
local uuid = "424C1010-0000-0000-87CB-CF832BF0EFAD"
local delay_ms = 500

local colors = {
  { red = 255, green = 255, blue = 255 },
  { red = 255, green = 0,   blue = 255 },
  { red = 0,   green = 0,   blue = 255 },
  { red = 0,   green = 255, blue = 0   },
  { red = 255, green = 255, blue = 0   },
  { red = 0,   green = 0,   blue = 0   }
}

while true do
  local dev = brick.get_device_from_uuid(uuid)
  if dev then
    for _, color in ipairs(colors) do
      brick.send_command(uuid, brick.CMD_LED_RGB, color)
      delay(delay_ms)
    end
  else
    delay(1000)
  end
end
)lua"));
}
