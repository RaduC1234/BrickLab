
/**
 * UUID Format (brick_uuid_t) from brickapi.h:
 * ```C
 * ┌────────┬────────────┬────────────────────────────────┐
 * │ Byte   │ Field      │ Description                    │
 * ├────────┼────────────┼────────────────────────────────┤
 * │ 0–1    │ prefix     │ Must be 'B', 'L' (0x42, 0x4C)  │
 * │ 2–3    │ type       │ Device type (big-endian)       │
 * │ 4–7    │ reserved   │ Reserved bytes (future use)    │
 * │ 8–15   │ unique_id  │ 8-byte device-specific ID      │
 * └────────┴────────────┴────────────────────────────────┘
 *
 * typedef struct __attribute__((packed)) {
 *    uint8_t prefix[2]; // 'B', 'L'
 *    uint8_t device_type[2]; // Big-endian: [hi, lo]
 *    uint8_t reserved[4]; // Reserved
 * uint8_t unique_id[8]; // Unique ID (e.g., serial, random)
 * } brick_uuid_raw_t;
 * ```
 */
export interface BrickModule {
    uuid: string; // Full UUID
    deviceType: number; // [2,3]
    i2cAddress: number;
    online: boolean;
}

/**
 * Parses a binary BLE payload into a list of BrickDevice entries.
 * Expects 18 bytes per device.
 */
export function parseBrickDeviceList(buffer: ArrayBuffer): BrickModule[] {
    const devices: BrickModule[] = [];
    const view = new DataView(buffer);
    const entrySize = 18;

    for (let offset = 0; offset + entrySize <= buffer.byteLength; offset += entrySize) {
        // UUID as hex string
        const uuidBytes = new Uint8Array(buffer.slice(offset, offset + 16));
        // @ts-ignore
        const uuidHex = Array.from(uuidBytes)
            .map(b => b.toString(16).padStart(2, '0'))
            .join('')
            .toUpperCase();

        const deviceType = (uuidBytes[2] << 8) | uuidBytes[3];
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