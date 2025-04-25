#ifndef BRICKLABS_API_HPP
#define BRICKLABS_API_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// I2C api
//====================================================================================

// Universal command
#define CMD_IDENTIFY             0x00  // → returns UUID (16 bytes)

// LED commands
#define CMD_LED_ON               0x01
#define CMD_LED_OFF              0x02
#define CMD_LED_TOGGLE           0x03

// Servo commands
#define CMD_SERVO_SET_ANGLE      0x10  // +1 byte (0–180)

// Sensor commands
#define CMD_SENSOR_GET_CM        0x20  // ← returns 2 bytes (cm)

/**
 * UUID Format (brick_uuid_t):
 * ┌────────┬────────────┬────────────────────────────────┐
 * │ Byte   │ Field      │ Description                    │
 * ├────────┼────────────┼────────────────────────────────┤
 * │ 0–1    │ prefix     │ Must be 'B', 'L' (0x42, 0x4C)  │
 * │ 2–3    │ type       │ Device type (big-endian)       │
 * │ 4–7    │ reserved   │ Reserved bytes (future use)    │
 * │ 8–15   │ unique_id  │ 8-byte device-specific ID      │
 * └────────┴────────────┴────────────────────────────────┘
 */
typedef struct __attribute__((packed)) {
    uint8_t prefix[2]; // 'B', 'L'
    uint8_t device_type[2]; // Big-endian: [hi, lo]
    uint8_t reserved[4]; // Reserved
    uint8_t unique_id[8]; // Unique ID (e.g., serial, random)
} brick_uuid_raw_t;

typedef union {
    brick_uuid_raw_t raw;
    uint8_t bytes[16];
} brick_uuid_t;

typedef enum {
    BRICK_TYPE_LED_BASE = 0x1000,
    BRICK_TYPE_MOTOR_BASE = 0x2000,
    BRICK_TYPE_SENSOR_BASE = 0x3000
} brick_device_type_group_t;

typedef enum {
    // LEDs
    LED_SINGLE = BRICK_TYPE_LED_BASE + 0x00,
    LED_DOUBLE = BRICK_TYPE_LED_BASE + 0x01,
    LED_RGB = BRICK_TYPE_LED_BASE + 0x10,

    // Motors
    MOTOR_SERVO_180 = BRICK_TYPE_MOTOR_BASE + 0x00,
    MOTOR_SERVO_360 = BRICK_TYPE_MOTOR_BASE + 0x01,
    MOTOR_STEPPER = BRICK_TYPE_MOTOR_BASE + 0x02,

    // Sensors
    SENSOR_COLOR = BRICK_TYPE_SENSOR_BASE + 0x00,
    SENSOR_DISTANCE = BRICK_TYPE_SENSOR_BASE + 0x01
} brick_device_type_t;

// led section
typedef struct {
    int is_on;
} brick_device_led_single_impl_t;

typedef struct {
    int is_on_1;
    int is_on_2;
} brick_device_led_double_impl_t;

typedef struct {
    int is_on_r;
    int is_on_g;
    int is_on_b;
} brick_device_led_rgb_impl_t;

// register here all the devices
typedef union {
    brick_device_led_single_impl_t led_single;
    brick_device_led_double_impl_t led_double;
    brick_device_led_rgb_impl_t led_rgb;
} brick_device_impl_t;

typedef struct {
    brick_uuid_t uuid;
    brick_device_type_t device_type;
    uint8_t i2c_address;
    brick_device_impl_t impl;
    uint8_t online; // 1 = online, 0 = offline (maintained by master)
} brick_device_t;


/**
 * Parses a 16-byte UUID and returns the corresponding device struct.
 * Does not populate i2c_address.
 */
brick_device_t get_device_specs_from_uuid(const uint8_t *uuid);

/**
* Derives I²C address from the first byte of unique_id.
* Ensures address is in safe I²C range: 0x08 - 0x77.
* @note the i2c uuid is always the first byte from the unique serial number
*/
uint8_t brick_uuid_get_i2c_address(const brick_uuid_t *uuid);

int brick_uuid_valid(const uint8_t *uuid);

void brick_print_uuid(const brick_uuid_t *uuid);

const char *brick_device_type_str(brick_device_type_t type);

// bluetooth api
//====================================================================================

void load_and_run(const char *programSource);

brick_device_t *get_host_modules();

#ifdef __cplusplus
}
#endif

#endif  // BRICKLABS_API_HPP
