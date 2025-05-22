/**
 * @file brick_i2c_api.h
 * @brief Public API for BrickLab I²C device definitions and access.
 *
 * This file declares the types, enums, and utility functions used to
 * interact with BrickLab hardware devices over the I²C bus.
 */

#ifndef BRICK_I2C_API_H
#define BRICK_I2C_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//====================================================================================
// Command Types
//====================================================================================

/**
 * @enum brick_command_type_t
 * @brief Supported commands that can be sent to devices via I²C.
 *
 * These correspond to the types of control or data request operations available.
 * @note Must be mapped to the Lua environment manually if needed.
 */
typedef enum {
    CMD_IDENTIFY = 0x00, /**< Request UUID (16 bytes) */
    CMD_LED = 0x01, /**< Set single LED intensity */
    CMD_LED_DOUBLE = 0x02, /**< Set dual LED intensities */
    CMD_LED_RGB = 0x03, /**< Set RGB LED values */
    CMD_SERVO_SET_ANGLE = 0x10, /**< Set angle for servo motor (-365 to 365 degrees) */
    CMD_STEPPER_MOVE = 0x11, /**< Move stepper motor: direction + steps + microstepping */
    CMD_SENSOR_GET_CM = 0x30 /**< Request distance sensor measurement in centimeters */
} brick_command_type_t;

//====================================================================================
// Device Types
//====================================================================================

/**
 * @enum brick_device_type_group_t
 * @brief Top-level categories for organizing devices.
 */
typedef enum {
    BRICK_TYPE_LED_BASE = 0x1000, /**< LED device category */
    BRICK_TYPE_MOTOR_BASE = 0x2000, /**< Motor device category */
    BRICK_TYPE_SENSOR_BASE = 0x3000 /**< Sensor device category */
} brick_device_type_group_t;

/**
 * @enum brick_device_type_t
 * @brief Specific types of devices recognized by the Brick system.
 */
typedef enum {
    LED_SINGLE = BRICK_TYPE_LED_BASE + 0x00,
    LED_DOUBLE = BRICK_TYPE_LED_BASE + 0x01,
    LED_RGB = BRICK_TYPE_LED_BASE + 0x10,

    MOTOR_SERVO_180 = BRICK_TYPE_MOTOR_BASE + 0x00,
    MOTOR_SERVO_360 = BRICK_TYPE_MOTOR_BASE + 0x01,
    MOTOR_STEPPER = BRICK_TYPE_MOTOR_BASE + 0x02,

    SENSOR_COLOR = BRICK_TYPE_SENSOR_BASE + 0x00,
    SENSOR_DISTANCE = BRICK_TYPE_SENSOR_BASE + 0x01
} brick_device_type_t;

//====================================================================================
// UUID Types
//====================================================================================

/**
 * @struct brick_uuid_raw_t
 * @brief Human-readable UUID structure containing device type, reserved bytes, and unique ID.
 */
typedef struct __attribute__((packed)) {
    uint8_t prefix[2]; /**< Prefix bytes: must be 'B', 'L' */
    uint8_t device_type[2]; /**< Big-endian encoded device type */
    uint8_t reserved[4]; /**< Reserved for future expansion */
    uint8_t unique_id[8]; /**< Unique 64-bit identifier */
} brick_uuid_raw_t;

/**
 * @union brick_uuid_t
 * @brief Wrapper for UUIDs allowing access as raw bytes or structured fields.
 */
typedef union {
    brick_uuid_raw_t raw; /**< Structured access to UUID fields */
    uint8_t bytes[16]; /**< Raw 16-byte UUID */
} brick_uuid_t;

//====================================================================================
// Device Implementation Structures
//====================================================================================

/**
 * @brief State for a single LED device.
 */
typedef struct {
    uint8_t is_on;
} brick_device_led_single_impl_t;

/**
 * @brief State for a dual LED device.
 */
typedef struct {
    uint8_t is_on_1;
    uint8_t is_on_2;
} brick_device_led_double_impl_t;

/**
 * @brief State for an RGB LED device.
 */
typedef struct {
    uint8_t red;
    uint8_t blue;
    uint8_t green;
} brick_device_led_rgb_impl_t;

typedef struct {
    uint8_t angle;
} brick_device_servo_180_impl_t;

/**
 * @union brick_device_impl_t
 * @brief Contains device-specific implementation data.
 */
typedef union {
    brick_device_led_single_impl_t led_single;
    brick_device_led_double_impl_t led_double;
    brick_device_led_rgb_impl_t led_rgb;
    brick_device_servo_180_impl_t servo_180;
} brick_device_impl_t;

//====================================================================================
// Device Structure and Command Descriptor
//====================================================================================

/**
 * @struct brick_device_t
 * @brief Representation of a discovered device on the I²C bus.
 */
typedef struct {
    brick_uuid_t uuid; /**< Unique device UUID */
    brick_device_type_t device_type; /**< Device type */
    uint8_t i2c_address; /**< 7-bit I²C address */
    brick_device_impl_t impl; /**< Device-specific data/state */
    uint8_t online; /**< 1 = online, 0 = offline */
} brick_device_t;

/**
 * @struct brick_command_t
 * @brief Structure used to send a command to a device.
 */
typedef struct {
    brick_command_type_t command; /**< Command enum */
    brick_device_t *device; /**< Target device */
} brick_command_t;

//====================================================================================
// Public API Functions
//====================================================================================

/**
 * @brief Retrieves a device's properties from its UUID.
 * @param uuid Pointer to 16-byte UUID.
 * @return A populated brick_device_t.
 */
brick_device_t brick_get_device_specs_from_uuid(const uint8_t *uuid);

/**
 * @brief Calculates a valid I²C address from the UUID.
 * @param uuid Pointer to the UUID.
 * @return 7-bit I²C address.
 */
uint8_t brick_uuid_get_i2c_address(const brick_uuid_t *uuid);

/**
 * @brief Verifies if the UUID prefix is valid.
 * @param uuid Pointer to UUID (16 bytes).
 * @return true if valid, false if not.
 */
bool brick_uuid_valid(const uint8_t *uuid);

/**
 * @brief Outputs a formatted UUID string to stdout.
 * @param uuid Pointer to the UUID.
 */
void brick_print_uuid(const brick_uuid_t *uuid);

/**
 * @brief Returns a string name for a given device type.
 * @param type Device type enum.
 * @return C-string name.
 */
const char *brick_device_type_str(brick_device_type_t type);

#ifdef __cplusplus
}
#endif

#endif // BRICK_I2C_API_H
