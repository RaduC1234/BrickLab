#ifndef BRICKLABS_API_H
#define BRICKLABS_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//====================================================================================
// Command Types
//====================================================================================
/**
 * @brief Brick command types sent via I²C.
 */
typedef enum {
    // Universal command
    CMD_IDENTIFY = 0x00, /**< Returns UUID (16 bytes) */

    // LED commands
    CMD_LED = 0x01, /**< Set single LED intensity */
    CMD_LED_DOUBLE = 0x02, /**< Set dual LEDs */
    CMD_LED_RGB = 0x03, /**< Set RGB LED */

    // Servo commands
    CMD_SERVO_SET_ANGLE = 0x10, /**< Set servo angle (-365 to 365 degrees) */
    CMD_STEPPER_MOVE = 0x11, /**< Move stepper motor: direction + steps + microstepping */

    // Sensor commands
    CMD_SENSOR_GET_CM = 0x30 /**< Request distance sensor measurement (2 bytes, cm) */
} brick_command_type_t;

//====================================================================================
// Device Types
//====================================================================================
/**
 * @brief Base groups for device types.
 */
typedef enum {
    BRICK_TYPE_LED_BASE = 0x1000,
    BRICK_TYPE_MOTOR_BASE = 0x2000,
    BRICK_TYPE_SENSOR_BASE = 0x3000
} brick_device_type_group_t;

/**
 * @brief Specific device types.
 */
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

//====================================================================================
// UUID Types
//====================================================================================
/**
 * @brief UUID structure layout (raw bytes).
 *
 * Format:
 * ┌────────┬────────────┬────────────────────────────────┐
 * │ Byte   │ Field      │ Description                    │
 * ├────────┼────────────┼────────────────────────────────┤
 * │ 0–1    │ prefix     │ Must be 'B', 'L' (0x42, 0x4C)  │
 * │ 2–3    │ type       │ Device type (big-endian)       │
 * │ 4–7    │ reserved   │ Reserved for future use        │
 * │ 8–15   │ unique_id  │ 8-byte unique device identifier│
 * └────────┴────────────┴────────────────────────────────┘
 */
typedef struct __attribute__((packed)) {
    uint8_t prefix[2]; /**< 'B', 'L' */
    uint8_t device_type[2]; /**< Device type (big-endian) */
    uint8_t reserved[4]; /**< Reserved bytes */
    uint8_t unique_id[8]; /**< Unique ID */
} brick_uuid_raw_t;

/**
 * @brief UUID representation as raw bytes or structured format.
 */
typedef union {
    brick_uuid_raw_t raw; /**< Structured format */
    uint8_t bytes[16]; /**< Raw bytes */
} brick_uuid_t;

typedef struct {
    uint8_t is_on;
} brick_device_led_single_impl_t;

typedef struct {
    uint8_t is_on_1;
    uint8_t is_on_2;
} brick_device_led_double_impl_t;

typedef struct {
    uint8_t is_on_r;
    uint8_t is_on_g;
    uint8_t is_on_b;
} brick_device_led_rgb_impl_t;

// register here all the devices
typedef union {
    brick_device_led_single_impl_t led_single;
    brick_device_led_double_impl_t led_double;
    brick_device_led_rgb_impl_t led_rgb;
} brick_device_impl_t;

//====================================================================================
// Device APIs
//====================================================================================
/**
 * @brief Describes a Brick device on the I²C bus.
 */
typedef struct {
    brick_uuid_t uuid; /**< Device UUID */
    brick_device_type_t device_type; /**< Type of the device */
    uint8_t i2c_address; /**< I²C address */
    brick_device_impl_t impl;
    uint8_t online; /**< 1 = online, 0 = offline (tracked by master) */
} brick_device_t;

typedef brick_device_t *brick_device;

/**
 * @brief Brick command structure.
 */
typedef struct {
    brick_command_type_t command; /**< Command type */
    brick_device_t *device;
} brick_command_t;

//====================================================================================
// Public API Functions
//====================================================================================
/**
 * @brief Parses a 16-byte UUID and returns the corresponding device specification.
 *
 * @param uuid Pointer to the 16-byte UUID.
 * @return Device structure with type and other information.
 */
brick_device_t brick_get_device_specs_from_uuid(const uint8_t *uuid);

/**
 * @brief Derives an I²C address from a UUID's first unique_id byte.
 *
 * Ensures the result is inside a safe I²C address range (0x08 - 0x77).
 *
 * @param uuid Pointer to UUID structure.
 * @return I²C address derived from UUID.
 */
uint8_t brick_uuid_get_i2c_address(const brick_uuid_t *uuid);

/**
 * @brief Validates the UUID prefix.
 *
 * @param uuid Pointer to UUID (16 bytes).
 * @return true if UUID is valid, false otherwise.
 */
bool brick_uuid_valid(const uint8_t *uuid);

/**
 * @brief Prints a UUID to standard output.
 *
 * @param uuid Pointer to UUID structure.
 */
void brick_print_uuid(const brick_uuid_t *uuid);

/**
 * @brief Returns a human-readable C-string for a device type.
 *
 * @param type Device type enum.
 * @return Constant C-string name of device type.
 */
const char *brick_device_type_str(brick_device_type_t type);

//====================================================================================
// Bluetooth API
//====================================================================================
/**
 * @brief Loads and runs a program from a source string.
 *
 * @param program_source Pointer to program source code.
 */
void brick_load_and_run(const char *program_source);

/**
 * @brief Returns a pointer to the host module list.
 *
 * @return Pointer to array of brick_device_t structures.
 */
void brick_get_host_modules(void);

#ifdef __cplusplus
}
#endif

#endif // BRICKLABS_API_H
