#include "brickapi.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

brick_device_t brick_get_device_specs_from_uuid(const uint8_t *uuid_bytes) {
    brick_device_t device = {0};

    if (!uuid_bytes) return device;

    memcpy(device.uuid.bytes, uuid_bytes, 16);

    uint16_t type = (uuid_bytes[2] << 8) | uuid_bytes[3];
    device.device_type = (brick_device_type_t) type;

    device.i2c_address = brick_uuid_get_i2c_address(&device.uuid);
    device.online = 0;

    return device;
}

uint8_t brick_uuid_get_i2c_address(const brick_uuid_t *uuid) {
    if (!uuid)
        return 0x00;

    const uint8_t raw = uuid->raw.unique_id[0];
    return 0x08 + raw % (0x78 - 0x08);
}

bool brick_uuid_valid(const uint8_t *uuid) {
    return uuid && uuid[0] == 'B' && uuid[1] == 'L';
}

void brick_print_uuid(const brick_uuid_t *uuid) {
    printf("UUID: ");
    for (int i = 0; i < 16; ++i)
        printf("%02X ", uuid->bytes[i]);
    printf("\n");
}

const char *brick_device_type_str(brick_device_type_t type) {
    switch (type) {
        case LED_SINGLE: return "LED_SINGLE";
        case LED_DOUBLE: return "LED_DOUBLE";
        case LED_RGB: return "LED_RGB";
        case MOTOR_SERVO_180: return "MOTOR_SERVO_180";
        case MOTOR_SERVO_360: return "MOTOR_SERVO_360";
        case MOTOR_STEPPER: return "MOTOR_STEPPER";
        case SENSOR_COLOR: return "SENSOR_COLOR";
        case SENSOR_DISTANCE: return "SENSOR_DISTANCE";
        default: return "UNKNOWN";
    }
}

void brick_load_and_run(const char *program_source) {
}

void brick_get_host_modules(void) {
}

#ifdef __cplusplus
}
#endif
