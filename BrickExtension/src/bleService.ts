// BrickExtension/src/bleService.ts - Simplified without chunking

import { BLE_COMMANDS } from './luaStringConverter';

// Import Noble
const noble = require('@abandonware/noble');

// BLE UUIDs for BrickLab ESP32 - CORRECTED TO MATCH ESP32
const BRICKLAB_SERVICE_UUID = 'ff00';        // ESP32 advertises ff00
const BRICKLAB_CHAR_GET_UUID = 'ff01';       // Read/Notify characteristic  
const BRICKLAB_CHAR_POST_UUID = 'ff02';      // Write characteristic

/**
 * BrickModule interface representing a connected hardware device
 */
export interface BrickModule {
    uuid: string;        // Device UUID as hex string (32 chars)
    deviceType: number;  // Device type extracted from UUID
    i2cAddress: number;  // I2C bus address
    online: boolean;     // Connection status
}

/**
 * Parse BLE payload containing device list from ESP32
 */
function parseBrickDeviceList(buffer: ArrayBuffer): BrickModule[] {
    const devices: BrickModule[] = [];
    const view = new DataView(buffer);
    const deviceSize = 18; // 16 bytes UUID + 1 byte I2C + 1 byte online

    for (let offset = 0; offset + deviceSize <= buffer.byteLength; offset += deviceSize) {
        // Extract 16-byte UUID
        const uuidBytes = new Uint8Array(buffer.slice(offset, offset + 16));
        const uuidHex = Array.from(uuidBytes)
            .map(b => b.toString(16).padStart(2, '0'))
            .join('')
            .toUpperCase();

        // Extract device type from UUID bytes 2-3 (big-endian)
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
 * Format UUID with dashes for display
 */
export function formatUuid(hexUuid: string): string {
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
 * BrickLab BLE Service using Noble for cross-platform Bluetooth LE support
 */
export class BrickBleService {
    // Connection state
    private isConnected: boolean = false;
    private connectedPeripheral: any = null;
    private connectedAddress: string = '';
    
    // GATT characteristics
    private readCharacteristic: any = null;
    private writeCharacteristic: any = null;
    
    // Device management
    private discoveredDevices: Map<string, any> = new Map();
    private cachedBrickDevices: BrickModule[] = [];
    
    // Event handling
    private notificationHandlers: Map<number, (data: Buffer) => void> = new Map();
    private isScanning: boolean = false;

    constructor() {
        this.initializeNoble();
    }

    /**
     * Initialize Noble event handlers
     */
    private initializeNoble(): void {
        noble.on('stateChange', (state: string) => {
            console.log(`Bluetooth state: ${state}`);
            
            if (state !== 'poweredOn' && this.isScanning) {
                this.isScanning = false;
            }
        });

        noble.on('discover', (peripheral: any) => {
            this.handleDeviceDiscovered(peripheral);
        });

        noble.on('scanStart', () => {
            console.log('📡 BLE scan started');
            this.isScanning = true;
        });

        noble.on('scanStop', () => {
            console.log('📡 BLE scan stopped');
            this.isScanning = false;
        });
    }

    /**
     * Handle discovered BLE peripheral
     */
    private handleDeviceDiscovered(peripheral: any): void {
        const address = peripheral.address || peripheral.id;
        const name = peripheral.advertisement?.localName || 'Unknown';
        const rssi = peripheral.rssi || -999;
        
        console.log(`📡 Discovered device: "${name}" at ${address} (RSSI: ${rssi})`);
        
        // Cache all discovered devices
        this.discoveredDevices.set(address, peripheral);
        
        // Check if this is a BrickLab device
        if (this.isBrickLabDevice(name)) {
            console.log(`🎯 FOUND BRICKLAB DEVICE: "${name}" at ${address}`);
        }
        
        // Set up disconnect handler for all devices
        peripheral.once('disconnect', () => {
            console.log(`📱 Device ${address} disconnected`);
            if (this.connectedPeripheral === peripheral) {
                console.log('📱 Connected device disconnected, cleaning up...');
                this.handleDisconnection();
            }
        });
    }

    /**
     * Check if device name indicates a BrickLab device
     */
    public isBrickLabDevice(name: string): boolean {
        if (!name) return false;
        
        const lowerName = name.toLowerCase();
        console.log(`🔍 Checking device name: "${name}" (lowercase: "${lowerName}")`);
        
        const patterns = [
            'bricklab',
            'brick lab', 
            'brick_lab',
            'brick-lab',
            'brickbase',
            'brick base',
            'brick_base',
            'brick-base',
            'esp32',
            'esp-32',
            'arduino'
        ];
        
        for (const pattern of patterns) {
            if (lowerName.includes(pattern)) {
                console.log(`✅ Device "${name}" matches pattern "${pattern}"`);
                return true;
            }
        }
        
        console.log(`❌ Device "${name}" doesn't match any BrickLab patterns`);
        return false;
    }

    /**
     * Wait for Bluetooth to be ready
     */
    private async waitForBluetoothReady(): Promise<void> {
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => {
                reject(new Error('Bluetooth initialization timeout. Check if Bluetooth is enabled.'));
            }, 15000);

            const checkState = () => {
                clearTimeout(timeout);
                
                switch (noble.state) {
                    case 'poweredOn':
                        resolve();
                        break;
                    case 'poweredOff':
                        reject(new Error('Bluetooth is turned off. Please enable Bluetooth.'));
                        break;
                    case 'unsupported':
                        reject(new Error('Bluetooth LE is not supported on this system.'));
                        break;
                    case 'unauthorized':
                        reject(new Error('Bluetooth access denied. Check permissions or run as administrator.'));
                        break;
                    case 'resetting':
                        // Wait for reset to complete
                        setTimeout(checkState, 1000);
                        break;
                    default:
                        reject(new Error(`Bluetooth not ready: ${noble.state}`));
                }
            };

            if (noble.state === 'unknown') {
                noble.once('stateChange', checkState);
            } else {
                checkState();
            }
        });
    }

    /**
     * Scan for BrickLab devices
     */
    async scanForDevices(timeoutMs: number = 10000): Promise<string[]> {
        try {
            console.log('🔍 Initializing Bluetooth for device scan...');
            await this.waitForBluetoothReady();
            
            console.log(`🔍 Scanning for BrickLab devices (${timeoutMs/1000}s)...`);
            
            // Clear previous discoveries
            this.discoveredDevices.clear();
            
            // Stop any existing scan
            if (this.isScanning) {
                noble.stopScanning();
                await new Promise(resolve => setTimeout(resolve, 500));
            }
            
            // Start scanning for all devices
            noble.startScanning([], false); // Empty array = all services, false = no duplicates
            
            // Wait for scan duration
            await new Promise(resolve => setTimeout(resolve, timeoutMs));
            
            // Stop scanning
            noble.stopScanning();
            
            // Filter for BrickLab devices
            const brickLabAddresses: string[] = [];
            
            for (const [address, peripheral] of this.discoveredDevices) {
                const name = peripheral.advertisement?.localName || '';
                if (this.isBrickLabDevice(name)) {
                    brickLabAddresses.push(address);
                }
            }
            
            console.log(`✅ Scan complete: ${brickLabAddresses.length} BrickLab devices found (${this.discoveredDevices.size} total)`);
            return brickLabAddresses;
            
        } catch (error) {
            console.error('❌ Device scan failed:', error);
            throw error;
        }
    }

    /**
     * Connect to a BrickLab device with retry logic
     */
    async connect(deviceAddress?: string, retries: number = 2): Promise<boolean> {
        for (let attempt = 1; attempt <= retries + 1; attempt++) {
            try {
                console.log(`🔗 Connection attempt ${attempt}/${retries + 1}`);
                
                const success = await this.attemptConnection(deviceAddress);
                if (success) {
                    return true;
                }
                
            } catch (error) {
                console.error(`❌ Connection attempt ${attempt} failed:`, error);
                
                if (attempt <= retries) {
                    console.log(`⏳ Retrying in 2 seconds...`);
                    await this.cleanup();
                    await new Promise(resolve => setTimeout(resolve, 2000));
                } else {
                    console.error('❌ All connection attempts failed');
                    await this.cleanup();
                    return false;
                }
            }
        }
        
        return false;
    }

    /**
     * Single connection attempt
     */
    private async attemptConnection(deviceAddress?: string): Promise<boolean> {
        await this.waitForBluetoothReady();
        
        // Auto-scan if no address provided
        if (!deviceAddress) {
            console.log('🔍 No device address specified, auto-scanning...');
            const devices = await this.scanForDevices(8000);
            
            if (devices.length === 0) {
                throw new Error('No BrickLab devices found. Ensure your ESP32 is powered on and advertising.');
            }
            
            deviceAddress = devices[0];
            console.log(`🎯 Auto-selected device: ${deviceAddress}`);
        }
        
        // Find the peripheral
        let peripheral = this.discoveredDevices.get(deviceAddress);
        
        if (!peripheral) {
            console.log('🔍 Device not in cache, scanning...');
            await this.scanForDevices(5000);
            peripheral = this.discoveredDevices.get(deviceAddress);
            
            if (!peripheral) {
                throw new Error(`Device ${deviceAddress} not found during scan`);
            }
        }
        
        console.log(`🔗 Connecting to ${deviceAddress}...`);
        
        // Set the peripheral first, before connecting
        this.connectedPeripheral = peripheral;
        this.connectedAddress = deviceAddress;
        
        // Connect to peripheral
        await this.connectToPeripheral(peripheral);
        
        // Longer delay to ensure connection is stable and GATT server is ready
        console.log('⏳ Waiting for GATT server to be ready...');
        await new Promise(resolve => setTimeout(resolve, 2000)); // Increased to 2 seconds
        
        // Verify peripheral is still connected
        if (!this.connectedPeripheral) {
            throw new Error('Peripheral was disconnected after connection');
        }
        
        // Discover and setup GATT services
        await this.setupGattServices();
        
        // Update connection state
        this.isConnected = true;
        
        console.log('✅ Successfully connected to BrickLab ESP32!');
        
        // Request initial device list
        setTimeout(() => {
            this.refreshDeviceList().catch(error => {
                console.warn('Failed to get initial device list:', error.message);
            });
        }, 1000);
        
        return true;
    }

    /**
     * Connect to a specific peripheral
     */
    private async connectToPeripheral(peripheral: any): Promise<void> {
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => {
                reject(new Error('Connection timeout (15s)'));
            }, 15000);
            
            // Set up connection event handlers
            peripheral.once('connect', () => {
                clearTimeout(timeout);
                console.log('✅ Connected to peripheral');
                resolve();
            });
            
            peripheral.once('disconnect', () => {
                clearTimeout(timeout);
                console.log('📱 Peripheral disconnected during connection');
                reject(new Error('Peripheral disconnected during connection'));
            });
            
            // Initiate connection
            peripheral.connect((error: any) => {
                if (error) {
                    clearTimeout(timeout);
                    reject(new Error(`Connection failed: ${error.message}`));
                }
                // Note: Don't resolve here, wait for 'connect' event
            });
        });
    }

    /**
     * Discover and setup GATT services and characteristics
     */
    private async setupGattServices(): Promise<void> {
        return new Promise((resolve, reject) => {
            // Check if peripheral is still connected
            if (!this.connectedPeripheral) {
                reject(new Error('Peripheral is null - connection was lost'));
                return;
            }
            
            const timeout = setTimeout(() => {
                reject(new Error('GATT service discovery timeout (15s)'));
            }, 15000);
            
            console.log('🔍 Discovering GATT services...');
            
            // Try to discover BrickLab service specifically - CORRECTED UUID
            this.connectedPeripheral.discoverServices([BRICKLAB_SERVICE_UUID], (error: any, services: any[]) => {
                if (error) {
                    clearTimeout(timeout);
                    console.error('❌ Specific service discovery error:', error);
                    reject(new Error(`Service discovery failed: ${error.message}`));
                    return;
                }
                
                console.log(`Found ${services.length} specific services`);
                
                if (services.length === 0) {
                    // If specific service not found, try discovering all services
                    console.log('🔍 BrickLab service not found, discovering all services...');
                    
                    if (!this.connectedPeripheral) {
                        clearTimeout(timeout);
                        reject(new Error('Peripheral disconnected during service discovery'));
                        return;
                    }
                    
                    this.connectedPeripheral.discoverServices([], (error: any, allServices: any[]) => {
                        if (error) {
                            clearTimeout(timeout);
                            console.error('❌ All services discovery error:', error);
                            reject(new Error(`All services discovery failed: ${error.message}`));
                            return;
                        }
                        
                        console.log(`Found ${allServices.length} total services:`);
                        allServices.forEach(service => {
                            console.log(`  Service UUID: ${service.uuid}`);
                        });
                        
                        // Look for BrickLab service in all services - CORRECTED UUID
                        const brickLabService = allServices.find(s => s.uuid === BRICKLAB_SERVICE_UUID);
                        
                        if (!brickLabService) {
                            clearTimeout(timeout);
                            reject(new Error(`BrickLab service (${BRICKLAB_SERVICE_UUID}) not found. Available services: ${allServices.map(s => s.uuid).join(', ')}`));
                            return;
                        }
                        
                        console.log('✅ Found BrickLab service in all services');
                        this.discoverCharacteristics(brickLabService, timeout, resolve, reject);
                    });
                } else {
                    const service = services[0];
                    console.log('✅ Found BrickLab service directly');
                    this.discoverCharacteristics(service, timeout, resolve, reject);
                }
            });
        });
    }

    /**
     * Discover characteristics for a service
     */
    private discoverCharacteristics(service: any, timeout: NodeJS.Timeout, resolve: Function, reject: Function): void {
        console.log('🔍 Discovering characteristics...');
        
        const allCharTimeout = setTimeout(() => {
            clearTimeout(timeout);
            reject(new Error('Characteristics discovery timeout'));
        }, 12000);
        
        service.discoverCharacteristics([], (error: any, allChars: any[]) => {
            clearTimeout(allCharTimeout);
            clearTimeout(timeout);
            
            if (error) {
                console.error('❌ Characteristics discovery error:', error);
                reject(new Error(`Characteristics discovery failed: ${error.message}`));
                return;
            }
            
            console.log(`Found ${allChars.length} total characteristics:`);
            allChars.forEach(char => {
                console.log(`  Characteristic UUID: ${char.uuid}, Properties: ${JSON.stringify(char.properties)}`);
            });
            
            if (allChars.length === 0) {
                reject(new Error('No characteristics found in service'));
                return;
            }
            
            this.mapCharacteristics(allChars, resolve, reject);
        });
    }

    /**
     * Map discovered characteristics with flexible UUID matching
     */
    private mapCharacteristics(characteristics: any[], resolve: Function, reject: Function): void {
        let foundGet = false, foundPost = false;
        
        console.log('🔗 Mapping characteristics...');
        
        for (const char of characteristics) {
            const charUuid = char.uuid.toLowerCase();
            
            console.log(`  Checking characteristic: ${charUuid}`);
            console.log(`    Properties: ${JSON.stringify(char.properties)}`);
            
            // Check for exact UUID matches first
            if (charUuid === BRICKLAB_CHAR_GET_UUID && !foundGet) {
                this.readCharacteristic = char;
                console.log(`  ✅ Mapped GET characteristic: ${charUuid}`);
                this.setupNotifications(char);
                foundGet = true;
            } else if (charUuid === BRICKLAB_CHAR_POST_UUID && !foundPost) {
                this.writeCharacteristic = char;
                console.log(`  ✅ Mapped POST characteristic: ${charUuid}`);
                foundPost = true;
            }
        }
        
        if (foundGet && foundPost) {
            console.log('✅ GATT characteristics configured successfully');
            resolve();
        } else {
            const availableUuids = characteristics.map(c => `${c.uuid} (${Object.keys(c.properties || {}).join(',')})`).join(', ');
            const missingChars = [];
            if (!foundGet) missingChars.push(`GET/READ (looking for ${BRICKLAB_CHAR_GET_UUID})`);
            if (!foundPost) missingChars.push(`POST/WRITE (looking for ${BRICKLAB_CHAR_POST_UUID})`);
            
            reject(new Error(`Missing characteristics: ${missingChars.join(', ')}. Available: ${availableUuids}`));
        }
    }

    /**
     * Setup notifications on the read characteristic
     */
    private setupNotifications(characteristic: any): void {
        console.log('🔔 Setting up notifications...');
        
        characteristic.subscribe((error: any) => {
            if (error) {
                console.error('❌ Failed to subscribe to notifications:', error.message);
            } else {
                console.log('✅ Subscribed to notifications');
            }
        });
        
        characteristic.on('data', (data: Buffer) => {
            this.handleNotification(data);
        });
        
        console.log('✅ Notification handlers configured');
    }

    /**
     * Handle incoming notifications from ESP32
     */
    private handleNotification(data: Buffer): void {
        const bytes = new Uint8Array(data);
        console.log(`📨 Received: [${Array.from(bytes).map(b => '0x' + b.toString(16).padStart(2, '0')).join(', ')}]`);
        
        if (bytes.length === 0) return;
        
        const responseType = bytes[0];
        
        // Check for registered handlers first
        const handler = this.notificationHandlers.get(responseType);
        if (handler) {
            console.log(`📋 Calling registered handler for response type: 0x${responseType.toString(16)}`);
            handler(data);
            return;
        }
        
        // Default handling for device list responses
        if (responseType === BLE_COMMANDS.DEVICE_LIST_RESPONSE) {
            console.log('📋 Device list response received (no handler registered)');
            try {
                const responseData = new Uint8Array(data.slice(1)); // Skip command byte
                const devices = parseBrickDeviceList(responseData.buffer);
                this.cachedBrickDevices = devices;
                console.log(`✅ Device list auto-processed: ${devices.length} devices`);
            } catch (parseError) {
                console.error('❌ Failed to auto-parse device list:', parseError);
            }
        } else if (responseType === BLE_COMMANDS.ERROR_RESPONSE) {
            const errorMsg = new TextDecoder().decode(bytes.slice(1));
            console.error('❌ ESP32 error:', errorMsg);
        } else {
            console.log(`❓ Unknown response type: 0x${responseType.toString(16)}`);
        }
    }

    /**
     * Disconnect from current device
     */
    async disconnect(): Promise<void> {
        try {
            if (this.connectedPeripheral) {
                this.connectedPeripheral.disconnect();
            }
        } catch (error) {
            console.error('Error during disconnect:', error);
        } finally {
            this.handleDisconnection();
            console.log('📱 Disconnected from BrickLab device');
        }
    }

    /**
     * Handle disconnection cleanup
     */
    private handleDisconnection(): void {
        this.isConnected = false;
        this.connectedPeripheral = null;
        this.connectedAddress = '';
        this.readCharacteristic = null;
        this.writeCharacteristic = null;
        this.cachedBrickDevices = [];
        this.notificationHandlers.clear();
    }

    /**
     * Check connection status
     */
    get connected(): boolean {
        return this.isConnected && this.connectedPeripheral !== null;
    }

    /**
     * Get cached device list
     */
    get deviceList(): BrickModule[] {
        return [...this.cachedBrickDevices];
    }

    /**
     * Send command to ESP32 with improved error handling
     */
    async sendCommand(command: Uint8Array): Promise<boolean> {
        if (!this.connected || !this.writeCharacteristic) {
            throw new Error('Not connected to BrickLab device');
        }

        try {
            const buffer = Buffer.from(command);
            
            return new Promise<boolean>((resolve) => {
                const timeout = setTimeout(() => {
                    console.warn(`⚠️  Write operation timeout (${command.length} bytes)`);
                    resolve(false);
                }, 10000); // Increased timeout to 10 seconds
                
                this.writeCharacteristic.write(buffer, false, (error: any) => {
                    clearTimeout(timeout);
                    
                    if (error) {
                        console.error('❌ Write failed:', error.message);
                        resolve(false);
                    } else {
                        console.log(`✅ Write successful (${command.length} bytes)`);
                        resolve(true);
                    }
                });
            });
        } catch (error) {
            console.error('❌ Send command failed:', error);
            return false;
        }
    }

    /**
     * Request device list from ESP32
     */
    async refreshDeviceList(): Promise<BrickModule[]> {
        if (!this.connected) {
            throw new Error('Not connected to device');
        }
        
        const command = new Uint8Array([BLE_COMMANDS.DEVICE_LIST_REQUEST]);
        
        return new Promise(async (resolve, reject) => {
            const timeout = setTimeout(() => {
                this.notificationHandlers.delete(BLE_COMMANDS.DEVICE_LIST_RESPONSE);
                reject(new Error('Device list request timeout'));
            }, 8000);

            // Set up response handler
            this.notificationHandlers.set(BLE_COMMANDS.DEVICE_LIST_RESPONSE, (data: Buffer) => {
                clearTimeout(timeout);
                this.notificationHandlers.delete(BLE_COMMANDS.DEVICE_LIST_RESPONSE);
                
                try {
                    const responseData = new Uint8Array(data.slice(1)); // Skip command byte
                    const devices = parseBrickDeviceList(responseData.buffer);
                    this.cachedBrickDevices = devices;
                    
                    console.log(`✅ Device list updated: ${devices.length} devices`);
                    resolve(devices);
                } catch (parseError) {
                    reject(new Error(`Failed to parse device list: ${parseError}`));
                }
            });
            
            // Send request
            const success = await this.sendCommand(command);
            if (!success) {
                clearTimeout(timeout);
                this.notificationHandlers.delete(BLE_COMMANDS.DEVICE_LIST_RESPONSE);
                reject(new Error('Failed to send device list request'));
            }
        });
    }

    /**
     * Send Lua script to ESP32 as single packet (SIMPLIFIED - NO CHUNKING)
     */
    async sendLuaScript(luaCode: string): Promise<boolean> {
        if (!this.connected) {
            throw new Error('Not connected to BrickLab device');
        }

        try {
            const luaBytes = Buffer.from(luaCode, 'utf8');
            const totalSize = luaBytes.length;
            
            console.log(`📤 Sending Lua script: ${totalSize} bytes as single packet`);
            
            // Check size limit for single packet transmission
            if (totalSize > 8192) { // 8KB limit
                throw new Error(`Lua script too large: ${totalSize} bytes (max 8KB)`);
            }
            
            // Create single command packet: [CMD_RUN_LUA_SCRIPT, ...lua_bytes]
            const command = new Uint8Array(luaBytes.length + 1);
            command[0] = BLE_COMMANDS.RUN_LUA_SCRIPT;
            command.set(luaBytes, 1);
            
            console.log(`📦 Sending ${command.length} bytes total (${luaBytes.length} Lua bytes)`);
            
            const success = await this.sendCommand(command);
            
            if (success) {
                console.log('✅ Lua script sent successfully');
                // Wait a bit for ESP32 to process
                await new Promise(resolve => setTimeout(resolve, 500));
                return true;
            } else {
                console.error('❌ Failed to send Lua script');
                return false;
            }
            
        } catch (error) {
            console.error('❌ Lua script transmission failed:', error);
            return false;
        }
    }

    /**
     * Clean up all resources
     */
    private async cleanup(): Promise<void> {
        this.handleDisconnection();
    }

    /**
     * Utility methods
     */
    
    getDevice(uuid: string): BrickModule | null {
        const cleanUuid = uuid.replace(/-/g, '').toUpperCase();
        return this.cachedBrickDevices.find(d => d.uuid === cleanUuid) || null;
    }

    getDevicesByType(deviceType: number): BrickModule[] {
        return this.cachedBrickDevices.filter(d => d.deviceType === deviceType);
    }

    getOnlineDevices(): BrickModule[] {
        return this.cachedBrickDevices.filter(d => d.online);
    }

    getDiscoveredDeviceInfo(address: string): { name: string; rssi: number } | null {
        const peripheral = this.discoveredDevices.get(address);
        if (!peripheral) return null;
        
        return {
            name: peripheral.advertisement?.localName || 'Unknown Device',
            rssi: peripheral.rssi || -999
        };
    }

    getAllDiscoveredDevices(): Array<{ address: string; name: string; rssi: number }> {
        const devices: Array<{ address: string; name: string; rssi: number }> = [];
        
        for (const [address, peripheral] of this.discoveredDevices) {
            devices.push({
                address,
                name: peripheral.advertisement?.localName || 'Unknown Device',
                rssi: peripheral.rssi || -999
            });
        }
        
        return devices;
    }

    getConnectionInfo(): object {
        return {
            connected: this.connected,
            deviceAddress: this.connectedAddress,
            deviceCount: this.cachedBrickDevices.length,
            onlineDevices: this.cachedBrickDevices.filter(d => d.online).length,
            bluetoothState: noble.state,
            discoveredPeripherals: this.discoveredDevices.size
        };
    }

    async getAdapters(): Promise<string[]> {
        try {
            await this.waitForBluetoothReady();
            return ['noble-default'];
        } catch (error) {
            return [];
        }
    }

    async testBluetooth(): Promise<{ success: boolean; message: string }> {
        try {
            await this.waitForBluetoothReady();
            return {
                success: true,
                message: `Bluetooth ready. State: ${noble.state}`
            };
        } catch (error) {
            return {
                success: false,
                message: (error as Error).message
            };
        }
    }
}

// Export singleton instance
export const bleService = new BrickBleService();