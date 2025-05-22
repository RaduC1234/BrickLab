// BrickExtension/src/extension.ts - Adapted to remove chunking

import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import { validateLuaCode, getLuaCodeInfo } from './luaStringConverter'; // Updated import
import { bleService, formatUuid } from './bleService';
import { getDeviceTypeName, formatUuidForDisplay } from './brickBleApi';
import { DeviceSidebarPanel } from './panels/DeviceSidebarPanel';

export function activate(context: vscode.ExtensionContext) {
    DeviceSidebarPanel.register(context);
    console.log('BrickLab extension is now active!');

    // Create new project command
    let createProjectCmd = vscode.commands.registerCommand('bricklab.createProject', async () => {
        const projectName = await vscode.window.showInputBox({ 
            prompt: 'Enter BrickLab Project Name' 
        });
        
        if (!projectName) {
            vscode.window.showErrorMessage('Project name is required!');
            return;
        }

        const workspaceFolders = vscode.workspace.workspaceFolders;
        if (!workspaceFolders) {
            vscode.window.showErrorMessage('Please open a folder in VSCode first.');
            return;
        }

        const rootPath = workspaceFolders[0].uri.fsPath;
        const projectPath = path.join(rootPath, projectName);

        // Create folder and files
        fs.mkdirSync(projectPath, { recursive: true });
        fs.writeFileSync(path.join(projectPath, 'project.brick'), `# BrickLab Project: ${projectName}\n`);
        fs.writeFileSync(path.join(projectPath, 'main.lua'), 
            `-- main.lua for ${projectName}\n\nlocal brick_labs = require("brick_lab")\nprint("Project ready!")\n`);

        vscode.window.showInformationMessage(`BrickLab project "${projectName}" created.`);
    });

    // Debug Bluetooth API
    let debugBluetoothCmd = vscode.commands.registerCommand('bricklab.debugBluetooth', async () => {
        try {
            vscode.window.showInformationMessage('Debugging Bluetooth API... Check console for details.');
            
            // Test Bluetooth functionality
            const testResult = await bleService.testBluetooth();
            console.log('Bluetooth test result:', testResult);
            
            const connectionInfo = bleService.getConnectionInfo();
            console.log('Connection info:', connectionInfo);
           
            vscode.window.showInformationMessage('Debug complete - check console output');
        } catch (error) {
            vscode.window.showErrorMessage(`Debug failed: ${(error as Error).message}`);
        }
    });

    // Scan for BrickLab devices
    let scanDevicesCmd = vscode.commands.registerCommand('bricklab.scanDevices', async () => {
        try {
            // Check if Bluetooth is available
            try {
                const adapters = await bleService.getAdapters();
                if (adapters.length === 0) {
                    vscode.window.showErrorMessage('No Bluetooth adapters found. Please ensure Bluetooth is enabled.');
                    return;
                }
                console.log(`Found ${adapters.length} Bluetooth adapter(s)`);
            } catch (adapterError) {
                vscode.window.showErrorMessage(`Bluetooth not available: ${(adapterError as Error).message}`);
                return;
            }

            vscode.window.showInformationMessage('Scanning for BrickLab devices... (10 seconds)');
            
            const deviceAddresses = await bleService.scanForDevices(10000);
            
            if (deviceAddresses.length === 0) {
                vscode.window.showWarningMessage('No BrickLab devices found. Make sure your ESP32 is powered on and advertising.');
                return;
            }

            // Show found devices for selection
            const deviceItems = deviceAddresses.map(address => ({
                label: address,
                description: 'BrickLab ESP32 Device',
                detail: 'Click to connect'
            }));

            const selected = await vscode.window.showQuickPick(deviceItems, {
                placeHolder: `Select device to connect (${deviceAddresses.length} found)`
            });

            if (selected) {
                vscode.window.showInformationMessage(`Connecting to ${selected.label}...`);
                
                const connected = await bleService.connect(selected.label);
                
                if (connected) {
                    vscode.window.showInformationMessage(`✓ Connected to BrickLab device!`);
                } else {
                    vscode.window.showErrorMessage('Failed to connect to selected device');
                }
            }

        } catch (error) {
            console.error('Scan error:', error);
            vscode.window.showErrorMessage(`Scan failed: ${(error as Error).message || error}`);
        }
    });

    // Connect to BrickLab device
    let connectCmd = vscode.commands.registerCommand('bricklab.connect', async () => {
        try {
            // If not connected, offer to scan first
            if (!bleService.connected) {
                const choice = await vscode.window.showQuickPick(
                    ['Scan for devices', 'Connect to last device'], 
                    { placeHolder: 'How would you like to connect?' }
                );

                if (!choice) return;

                if (choice === 'Scan for devices') {
                    vscode.commands.executeCommand('bricklab.scanDevices');
                    return;
                }
            }

            vscode.window.showInformationMessage('Connecting to BrickLab device...');

            const connected = await bleService.connect(); // Will auto-scan if no address

            if (connected) {
                vscode.window.showInformationMessage('✓ Connected to BrickLab ESP32!');

                try {
                    const devices = await bleService.refreshDeviceList();
                    const deviceCount = devices.length;
                    const onlineCount = devices.filter(d => d.online).length;

                    vscode.window.showInformationMessage(
                        `Found ${deviceCount} devices (${onlineCount} online)`
                    );

                    // 👇 Refresh sidebar after device list is loaded
                    DeviceSidebarPanel.refresh();

                } catch (error) {
                    console.error('Failed to get device list:', error);
                }
            } else {
                vscode.window.showErrorMessage('Failed to connect to BrickLab device');
            }
        } catch (error) {
            vscode.window.showErrorMessage(`Connection error: ${error}`);
        }
    });


    // Disconnect from device
    let disconnectCmd = vscode.commands.registerCommand('bricklab.disconnect', async () => {
        await bleService.disconnect();
        vscode.window.showInformationMessage('Disconnected from BrickLab device');
    });

    // Run Lua file with options (adapted to remove chunking)
    let runLuaCmd = vscode.commands.registerCommand('bricklab.runLuaFile', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showErrorMessage('No active editor found.');
            return;
        }

        const luaCode = editor.document.getText();
        
        if (!bleService.connected) {
            const connectFirst = await vscode.window.showWarningMessage(
                'Not connected to BrickLab device. Connect now?', 
                'Connect', 'Cancel'
            );
            
            if (connectFirst === 'Connect') {
                const connected = await bleService.connect();
                if (!connected) {
                    vscode.window.showErrorMessage('Failed to connect to device');
                    return;
                }
            } else {
                return;
            }
        }

        // Show run options (simplified - removed chunking options)
        const options = ['Run on Device', 'Show Code Info', 'Validate Code'];
        const choice = await vscode.window.showQuickPick(options, {
            placeHolder: 'Choose how to run the Lua code'
        });

        if (!choice) return;

        switch (choice) {
            case 'Run on Device':
                await runLuaOnDevice(luaCode);
                break;
            case 'Show Code Info':
                await showCodeInfo(luaCode);
                break;
            case 'Validate Code':
                await validateCode(luaCode);
                break;
        }
    });

    // Get device list
    let getDevicesCmd = vscode.commands.registerCommand('bricklab.getDevices', async () => {
        if (!bleService.connected) {
            vscode.window.showErrorMessage('Not connected to BrickLab device');
            return;
        }

        try {
            vscode.window.showInformationMessage('Getting device list...');
            const devices = await bleService.refreshDeviceList();
            
            // Show devices in a quick pick
            const deviceItems = devices.map(device => ({
                label: formatUuidForDisplay(device.uuid),
                description: `${getDeviceTypeName(device.deviceType)} • I2C: 0x${device.i2cAddress.toString(16).padStart(2, '0')}`,
                detail: device.online ? '🟢 Online' : '🔴 Offline',
                device: device
            }));

            if (deviceItems.length === 0) {
                vscode.window.showWarningMessage('No devices found');
                return;
            }

            const selected = await vscode.window.showQuickPick(deviceItems, {
                placeHolder: `Select device (${devices.length} found)`
            });

            if (selected) {
                vscode.window.showInformationMessage(`Selected: ${selected.label}`);
            }

        } catch (error) {
            vscode.window.showErrorMessage(`Failed to get devices: ${error}`);
        }
    });

    let scanUnknownDevicesCmd = vscode.commands.registerCommand('bricklab.scanUnknownDevices', async () => {
        try {
            console.log('🔍 Scanning for devices and checking "Unknown" entries...');
            
            // Use bleService methods
            const deviceAddresses = await bleService.scanForDevices(10000);
            console.log(`Initial scan found ${deviceAddresses.length} BrickLab devices`);
            
            const allDevices = bleService.getAllDiscoveredDevices();
            console.log(`Found ${allDevices.length} total devices`);
            
            // Show all devices for manual selection, including "Unknown" ones
            const deviceItems = allDevices.map((device, index) => ({
                label: `${index + 1}. ${device.address}`,
                description: `"${device.name}" (RSSI: ${device.rssi})`,
                detail: 'Click to test connection',
                address: device.address
            }));
            
            if (deviceItems.length === 0) {
                vscode.window.showWarningMessage('No devices found');
                return;
            }
            
            const selected = await vscode.window.showQuickPick(deviceItems, {
                placeHolder: `Select device to test (${deviceItems.length} found)`
            });
            
            if (selected) {
                console.log(`Testing connection to ${selected.address}...`);
                vscode.window.showInformationMessage(`Testing connection to ${selected.address}...`);
                
                try {
                    const connected = await bleService.connect(selected.address);
                    
                    if (connected) {
                        vscode.window.showInformationMessage(`✅ Connected! Testing if this is a BrickLab device...`);
                        
                        try {
                            // Try to get device list - this will work if it's a BrickLab device
                            const devices = await bleService.refreshDeviceList();
                            vscode.window.showInformationMessage(`🎯 SUCCESS! This is a BrickLab device with ${devices.length} connected modules!`);
                            console.log('🎯 Found BrickLab device!', devices);
                        } catch (error) {
                            vscode.window.showWarningMessage(`Not a BrickLab device: ${error}`);
                            await bleService.disconnect();
                        }
                    } else {
                        vscode.window.showErrorMessage(`Failed to connect to ${selected.address}`);
                    }
                    
                } catch (error) {
                    vscode.window.showErrorMessage(`Connection error: ${error}`);
                }
            }
            
        } catch (error) {
            console.error('Scan error:', error);
            vscode.window.showErrorMessage(`Scan failed: ${error}`);
        }
    });

    // Try to connect to all Unknown devices automatically
    let autoTestUnknownCmd = vscode.commands.registerCommand('bricklab.autoTestUnknown', async () => {
        try {
            console.log('🤖 Auto-testing all "Unknown" devices...');
            
            // Scan first
            const deviceAddresses = await bleService.scanForDevices(8000);
            console.log(`Initial scan found ${deviceAddresses.length} BrickLab devices`);
            
            const allDevices = bleService.getAllDiscoveredDevices();
            const unknownDevices = allDevices.filter(d => (d.name || '').toLowerCase() === 'unknown');
            
            console.log(`Found ${unknownDevices.length} "Unknown" devices to test`);
            vscode.window.showInformationMessage(`Testing ${unknownDevices.length} "Unknown" devices...`);
            
            for (let i = 0; i < unknownDevices.length; i++) {
                const device = unknownDevices[i];
                console.log(`Testing device ${i + 1}/${unknownDevices.length}: ${device.address}`);
                
                try {
                    const connected = await bleService.connect(device.address);
                    
                    if (connected) {
                        console.log(`✅ Connected to ${device.address}, testing...`);
                        
                        try {
                            // Short timeout for device list
                            const devices = await Promise.race([
                                bleService.refreshDeviceList(),
                                new Promise((_, reject) => setTimeout(() => reject(new Error('Timeout')), 3000))
                            ]);
                            
                            // Found a BrickLab device!
                            console.log(`🎯 BRICKLAB DEVICE FOUND: ${device.address}`);
                            vscode.window.showInformationMessage(`🎯 Found BrickLab device at ${device.address} with ${(devices as any).length} modules!`);
                            return; // Stop testing, we found it
                            
                        } catch (error) {
                            console.log(`❌ ${device.address} not a BrickLab device: ${error}`);
                            await bleService.disconnect();
                        }
                    } else {
                        console.log(`❌ Failed to connect to ${device.address}`);
                    }
                    
                } catch (error) {
                    console.log(`❌ Error testing ${device.address}: ${error}`);
                }
                
                // Small delay between tests
                await new Promise(resolve => setTimeout(resolve, 500));
            }
            
            vscode.window.showWarningMessage('No BrickLab devices found among the "Unknown" devices');
            
        } catch (error) {
            console.error('Auto test error:', error);
            vscode.window.showErrorMessage(`Auto test failed: ${error}`);
        }
    });

    // Manual device picker - let user select any device to test
    let manualDeviceTestCmd = vscode.commands.registerCommand('bricklab.manualDeviceTest', async () => {
        try {
            console.log('🔍 Manual device test - scan and pick any device...');
            
            // Scan first
            await bleService.scanForDevices(10000);
            const allDevices = bleService.getAllDiscoveredDevices();
            
            if (allDevices.length === 0) {
                vscode.window.showWarningMessage('No devices found');
                return;
            }
            
            // Show all devices with detailed info
            const deviceItems = allDevices.map((device, index) => ({
                label: `Device ${index + 1}: ${device.address}`,
                description: `"${device.name || 'No Name'}" (RSSI: ${device.rssi})`,
                detail: device.name === 'Unknown' ? '⚠️ Unknown device - could be BrickLab' : '📱 Named device',
                address: device.address
            }));
            
            const selected = await vscode.window.showQuickPick(deviceItems, {
                placeHolder: `Select ANY device to test for BrickLab compatibility (${allDevices.length} found)`
            });
            
            if (selected) {
                vscode.window.showInformationMessage(`Testing ${selected.address}...`);
                
                try {
                    const connected = await bleService.connect(selected.address);
                    
                    if (connected) {
                        console.log(`✅ Connected to ${selected.address}`);
                        
                        try {
                            const devices = await bleService.refreshDeviceList();
                            vscode.window.showInformationMessage(`🎯 SUCCESS! Found BrickLab device with ${devices.length} modules!`);
                            console.log('Device list:', devices);
                        } catch (error) {
                            vscode.window.showInformationMessage(`Connected but not a BrickLab device: ${error}`);
                            await bleService.disconnect();
                        }
                    } else {
                        vscode.window.showErrorMessage(`Failed to connect to ${selected.address}`);
                    }
                    
                } catch (error) {
                    vscode.window.showErrorMessage(`Test failed: ${error}`);
                }
            }
            
        } catch (error) {
            console.error('Manual test error:', error);
            vscode.window.showErrorMessage(`Manual test failed: ${error}`);
        }
    });

    // Register all commands
    context.subscriptions.push(
        createProjectCmd,
        debugBluetoothCmd,
        scanDevicesCmd,
        connectCmd, 
        disconnectCmd,
        runLuaCmd,
        getDevicesCmd,
        scanUnknownDevicesCmd,
        autoTestUnknownCmd,
        manualDeviceTestCmd
    );

    // Show connection status in status bar
    const statusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
    statusBarItem.command = 'bricklab.connect';
    statusBarItem.text = '$(debug-disconnect) BrickLab: Disconnected';
    statusBarItem.tooltip = 'Click to connect to BrickLab device';
    statusBarItem.show();
    context.subscriptions.push(statusBarItem);

    // Update status bar when connection changes
    const updateStatusBar = () => {
        if (bleService.connected) {
            statusBarItem.text = '$(debug-console) BrickLab: Connected';
            statusBarItem.tooltip = 'Connected to BrickLab device';
            statusBarItem.command = 'bricklab.disconnect';
        } else {
            statusBarItem.text = '$(debug-disconnect) BrickLab: Disconnected';
            statusBarItem.tooltip = 'Click to connect to BrickLab device';
            statusBarItem.command = 'bricklab.connect';
        }
    };

    // Check connection status periodically
    const connectionChecker = setInterval(updateStatusBar, 2000);
    context.subscriptions.push({ dispose: () => clearInterval(connectionChecker) });
}

// Simplified Lua execution - no chunking
async function runLuaOnDevice(luaCode: string): Promise<void> {
    try {
        // Validate code first
        const validation = validateLuaCode(luaCode);
        if (!validation.valid) {
            vscode.window.showErrorMessage(`Lua validation failed: ${validation.error}`);
            return;
        }

        vscode.window.showInformationMessage('Sending Lua code to device...');
        
        // Use simplified single-packet transmission
        const success = await bleService.sendLuaScript(luaCode);
        
        if (success) {
            vscode.window.showInformationMessage('✓ Lua code sent and executed!');
        } else {
            vscode.window.showErrorMessage('Failed to send Lua code to device');
        }

    } catch (error) {
        vscode.window.showErrorMessage(`Error running Lua: ${error}`);
    }
}

// Show code information instead of byte conversion
async function showCodeInfo(luaCode: string): Promise<void> {
    const info = getLuaCodeInfo(luaCode);
    const validation = validateLuaCode(luaCode);
    
    const message = `Lua Code Information:
• Characters: ${info.characterCount}
• Bytes (UTF-8): ${info.byteSize}
• Lines: ${info.lines}
• Status: ${validation.valid ? '✅ Valid' : `❌ ${validation.error}`}
• Transmission: ${info.byteSize <= 8192 ? '✅ Fits in single packet' : '❌ Too large for BLE'}`;

    vscode.window.showInformationMessage(message);
}

// Validate code function
async function validateCode(luaCode: string): Promise<void> {
    const validation = validateLuaCode(luaCode);
    
    if (validation.valid) {
        vscode.window.showInformationMessage('✅ Lua code is valid and ready for transmission');
    } else {
        vscode.window.showErrorMessage(`❌ Lua validation failed: ${validation.error}`);
    }
}

export function deactivate() {
    bleService.disconnect();
}