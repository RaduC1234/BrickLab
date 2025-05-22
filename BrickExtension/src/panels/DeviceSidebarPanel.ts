import * as vscode from 'vscode';
import { getDeviceTypeName } from '../brickBleApi';
import { bleService } from '../bleService';

export class DeviceSidebarPanel {
  public static currentPanel: DeviceSidebarPanel | undefined;
  private readonly panel: vscode.WebviewView;

  constructor(panel: vscode.WebviewView) {
    this.panel = panel;

    panel.webview.options = { enableScripts: true };
    this.update();

    panel.onDidChangeVisibility(() => {
      if (panel.visible) this.update();
    });
  }

  public static register(context: vscode.ExtensionContext) {
    context.subscriptions.push(
      vscode.window.registerWebviewViewProvider('bricklab.devices', {
        resolveWebviewView: (view) => {
          DeviceSidebarPanel.currentPanel = new DeviceSidebarPanel(view);
        }
      })
    );
  }

  public static refresh() {
    if (DeviceSidebarPanel.currentPanel) {
      DeviceSidebarPanel.currentPanel.update();
    }
  }

    private update() {
    const devices = bleService.deviceList;

    const html = `
        <html>
        <head>
        <style>
            :root {
            color-scheme: light dark;
            }

            body {
            font-family: var(--vscode-font-family);
            color: var(--vscode-foreground);
            background-color: var(--vscode-sideBar-background);
            padding: 10px;
            }

            .device-card {
            background-color: var(--vscode-editorWidget-background);
            border: 1px solid var(--vscode-sideBar-border);
            border-radius: 8px;
            padding: 12px;
            margin-bottom: 10px;
            box-shadow: 0 1px 3px var(--vscode-widget-shadow);
            }

            .device-title {
            font-weight: bold;
            margin-bottom: 5px;
            font-size: 13px;
            color: var(--vscode-sideBarTitle-foreground);
            }

            .device-info {
            font-size: 12px;
            margin: 2px 0;
            }

            .status {
            margin-top: 5px;
            font-weight: bold;
            }

            .online {
            color: var(--vscode-editorInfo-foreground);
            }

            .offline {
            color: var(--vscode-editorError-foreground);
            }
        </style>
        </head>
        <body>
        <h3>Connected Devices</h3>
        ${devices.length === 0 ? '<p class="device-info">No devices connected.</p>' : ''}
        ${devices.map(device => `
            <div class="device-card">
            <div class="device-title">${getDeviceTypeName(device.deviceType)}</div>
            <div class="device-info">UUID: ${device.uuid}</div>
            <div class="device-info">I2C: 0x${device.i2cAddress.toString(16).padStart(2, '0')}</div>
            <div class="status ${device.online ? 'online' : 'offline'}">
                ${device.online ? '🟢 Online' : '🔴 Offline'}
            </div>
            </div>
        `).join('')}
        </body>
        </html>
    `;

    this.panel.webview.html = html;
    }

}

