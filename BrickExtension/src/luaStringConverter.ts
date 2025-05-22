// luaStringConverter.ts - Simplified without chunking

/**
 * BLE command constants matching ESP32 firmware
 */
export const BLE_COMMANDS = {
  DEVICE_LIST_REQUEST: 0xFF,
  DEVICE_LIST_RESPONSE: 0x01,
  RUN_LUA_SCRIPT: 0x02,
  SET_DEVICE_STATE: 0x03,
  ERROR_RESPONSE: 0xFE
} as const;

/**
 * Simple Lua code validation
 */
export function validateLuaCode(luaCode: string): { valid: boolean; error?: string } {
  if (!luaCode || luaCode.trim().length === 0) {
    return { valid: false, error: 'Lua code is empty' };
  }
  
  if (luaCode.length > 8192) { // 8KB limit for BLE transmission
    return { valid: false, error: 'Lua code too large (max 8KB)' };
  }
  
  return { valid: true };
}

/**
 * Get Lua code size information
 */
export function getLuaCodeInfo(luaCode: string): {
  characterCount: number;
  byteSize: number;
  lines: number;
} {
  const bytes = Buffer.from(luaCode, 'utf8');
  
  return {
    characterCount: luaCode.length,
    byteSize: bytes.length,
    lines: luaCode.split('\n').length
  };
}