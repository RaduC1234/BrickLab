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
