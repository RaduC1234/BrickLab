-- brick_labs.lua

-- Base Device class: handles common device logic
local Device = {}
Device.__index = Device

-- Create a new Device instance by UUID
function Device.new(uuid)
  local handle = brick.get_device_from_uuid(uuid)              -- Get device from C API
  assert(handle, "Device not found: " .. uuid)                 -- Ensure device exists
  return setmetatable({ uuid = uuid, handle = handle }, Device)
end

-- Get the device type (e.g., LED_RGB, SERVO, etc.)
function Device:get_type()
  return self.handle.device_type
end

-- Check if this device is an RGB LED device
function Device:is_rgb()
  return self:get_type() == brick.DEVICE_LED_RGB               -- Requires DEVICE_LED_RGB enum exposed to Lua
end

-- DeviceRgb subclass: extends Device with RGB-specific methods
local DeviceRgb = {}
DeviceRgb.__index = DeviceRgb
setmetatable(DeviceRgb, { __index = Device })                  -- Inherit from Device

-- Create a new RGB device, ensuring the device type matches
function DeviceRgb.new(uuid)
  local base = Device.new(uuid)
  assert(base:is_rgb(), "Not an RGB LED device")               -- Type check at runtime
  return setmetatable(base, DeviceRgb)
end

-- Set the RGB LED to a specific color table: { red = X, green = Y, blue = Z }
function DeviceRgb:set_rgb(color)
  brick.send_command(self.uuid, brick.CMD_LED_RGB, color)      -- Send RGB command via C API
end

-- Return the module: exposes both classes to users of require("brick_labs")
return {
  Device = Device,
  DeviceRgb = DeviceRgb
}
