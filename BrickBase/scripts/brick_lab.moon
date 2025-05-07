class Device
  new: (@uuid) =>
    @handle = brick.get_device_from_uuid uuid
    assert @handle, "Device not found: #{@uuid}"

  get_type: => @handle.device_type
  is_rgb: => @get_type! == brick.DEVICE_LED_RGB

class DeviceRgb extends Device
  new: (uuid) =>
    super uuid
    assert @is_rgb!, "Not an RGB device"

  set_rgb: (color) =>
    brick.send_command @uuid, brick.CMD_LED_RGB, color

{ Device, DeviceRgb }
