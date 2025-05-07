local env = require("brick_lab")

local rgbDevice = DeviceRgb.new("424C1010-0000-0000-87CB-CF832BF0EFAD");

while true do
    rgbDevice:set_rgb({ 0, 0, 255 })
    delay(100);

    rgbDevice:set_rgb({ 0, 255, 0 })
    delay(100);

    rgbDevice:set_rgb({ 0, 255, 0 })
    delay(100);
end
