// BrickExtension/src/brickBleApi.ts
// Self-contained BLE API - no imports from BrickBase

/**
 * Device type constants (matching your C++ firmware)
 */
export const DEVICE_TYPES = {
    LED_SINGLE: 0x1000,
    LED_DOUBLE: 0x1001, 
    LED_RGB: 0x1010,
    MOTOR_SERVO_180: 0x2000,
    MOTOR_SERVO_360: 0x2001,
    MOTOR_STEPPER: 0x2002,
    SENSOR_COLOR: 0x3000,
    SENSOR_DISTANCE: 0x3001
} as const;

/**
 * Command type constants (matching your C++ firmware)
 */
export const COMMANDS = {
    CMD_IDENTIFY: 0x00,
    CMD_LED: 0x01,
    CMD_LED_DOUBLE: 0x02,
    CMD_LED_RGB: 0x03,
    CMD_SERVO_SET_ANGLE: 0x10,
    CMD_STEPPER_MOVE: 0x11,
    CMD_SENSOR_GET_CM: 0x30
} as const;

/**
 * BLE protocol commands
 */
export const BLE_PROTOCOL = {
    DEVICE_LIST_REQUEST: 0xFF,
    DEVICE_LIST_RESPONSE: 0x01,
    RUN_LUA_SCRIPT: 0x02,
    SET_DEVICE_STATE: 0x03,
    ERROR_RESPONSE: 0xFE
} as const;

/**
 * BrickModule interface representing a connected device
 */
export interface BrickModule {
    uuid: string;        // 32-char hex string (no dashes)
    deviceType: number;  // Device type from DEVICE_TYPES
    i2cAddress: number;  // I2C address (0x08-0x77)
    online: boolean;     // Connection status
}

/**
 * Parses BLE device list payload from ESP32
 * Format: 18 bytes per device (16 UUID + 1 I2C + 1 online)
 */
export function parseBrickDeviceList(buffer: ArrayBuffer): BrickModule[] {
    const devices: BrickModule[] = [];
    const view = new DataView(buffer);
    const entrySize = 18;

    for (let offset = 0; offset + entrySize <= buffer.byteLength; offset += entrySize) {
        // Extract 16-byte UUID
        const uuidBytes = new Uint8Array(buffer.slice(offset, offset + 16));
        
        // Convert to hex string
        const uuidHex = Array.from(uuidBytes)
            .map(b => b.toString(16).padStart(2, '0'))
            .join('')
            .toUpperCase();

        // Extract device type from UUID bytes [2,3] (big-endian)
        const deviceType = (uuidBytes[2] << 8) | uuidBytes[3];
        
        // Extract I2C address and online status
        const i2cAddress = view.getUint8(offset + 16);
        const online = view.getUint8(offset + 17) === 1;

        devices.push({
            uuid: uuidHex,
            deviceType,
            i2cAddress,
            online
        });
    }

    return devices;
}

/**
 * Formats UUID for display (adds dashes)
 */
export function formatUuidForDisplay(hexUuid: string): string {
    if (hexUuid.length !== 32) return hexUuid;
    
    return [
        hexUuid.slice(0, 8),
        hexUuid.slice(8, 12), 
        hexUuid.slice(12, 16),
        hexUuid.slice(16, 20),
        hexUuid.slice(20, 32)
    ].join('-');
}

/**
 * Gets device type name from numeric type
 */
export function getDeviceTypeName(deviceType: number): string {
    const typeMap: Record<number, string> = {
        [DEVICE_TYPES.LED_SINGLE]: 'LED Single',
        [DEVICE_TYPES.LED_DOUBLE]: 'LED Double',
        [DEVICE_TYPES.LED_RGB]: 'LED RGB',
        [DEVICE_TYPES.MOTOR_SERVO_180]: 'Servo 180°',
        [DEVICE_TYPES.MOTOR_SERVO_360]: 'Servo 360°', 
        [DEVICE_TYPES.MOTOR_STEPPER]: 'Stepper Motor',
        [DEVICE_TYPES.SENSOR_COLOR]: 'Color Sensor',
        [DEVICE_TYPES.SENSOR_DISTANCE]: 'Distance Sensor'
    };
    
    return typeMap[deviceType] || `Unknown (0x${deviceType.toString(16)})`;
}

/**
 * Validates if UUID has correct BrickLab prefix
 */
export function isValidBrickUuid(uuid: string): boolean {
    if (uuid.length !== 32) return false;
    
    // Check for 'BL' prefix (0x424C)
    const prefix = uuid.slice(0, 4).toUpperCase();
    return prefix === '424C';
}

/**
 * Extracts device type from UUID string
 */
export function getDeviceTypeFromUuid(uuid: string): number {
    if (uuid.length !== 32) return 0;
    
    // Device type is in bytes [2,3] (chars 4-7)
    const typeHex = uuid.slice(4, 8);
    return parseInt(typeHex, 16);
}

/**
 * Creates a device filter for specific types
 */
export function createDeviceFilter(deviceTypes: number[]) {
    return (device: BrickModule) => deviceTypes.includes(device.deviceType);
}

/**
 * Example device creation (for testing/simulation)
 */
export function createMockDevice(deviceType: number, uniqueId: string = '0123456789ABCDEF'): BrickModule {
    // Create UUID: 'BL' + deviceType + reserved + uniqueId
    const prefix = '424C'; // 'BL'
    const typeHex = deviceType.toString(16).padStart(4, '0').toUpperCase();
    const reserved = '00000000';
    const uuid = prefix + typeHex + reserved + uniqueId;
    
    return {
        uuid,
        deviceType,
        i2cAddress: 0x10 + (deviceType & 0x0F), // Mock I2C address
        online: true
    };
}

// Export commonly used device type groups
export const LED_DEVICES = [DEVICE_TYPES.LED_SINGLE, DEVICE_TYPES.LED_DOUBLE, DEVICE_TYPES.LED_RGB];
export const MOTOR_DEVICES = [DEVICE_TYPES.MOTOR_SERVO_180, DEVICE_TYPES.MOTOR_SERVO_360, DEVICE_TYPES.MOTOR_STEPPER];
export const SENSOR_DEVICES = [DEVICE_TYPES.SENSOR_COLOR, DEVICE_TYPES.SENSOR_DISTANCE];
