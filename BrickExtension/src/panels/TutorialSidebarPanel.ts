import * as vscode from 'vscode';

export class TutorialSidebarPanel {
  public static currentPanel: TutorialSidebarPanel | undefined;
  private readonly panel: vscode.WebviewView;

  constructor(panel: vscode.WebviewView) {
    this.panel = panel;

    panel.webview.options = { enableScripts: true };
    this.update();

    panel.webview.onDidReceiveMessage(message => {
      if (message.command === 'highlightConnect') {
        vscode.window.showInformationMessage('Look for the status bar to connect to your BrickLab device!');
      }
    });
  }

  public static register(context: vscode.ExtensionContext) {
    context.subscriptions.push(
      vscode.window.registerWebviewViewProvider('bricklab.tutorial', {
        resolveWebviewView: (view) => {
          TutorialSidebarPanel.currentPanel = new TutorialSidebarPanel(view);
        }
      })
    );
  }

private update() {
  this.panel.webview.html = `
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <style>
        body {
          display: flex;
          margin: 0;
          padding: 10px;
          font-family: var(--vscode-font-family);
          color: var(--vscode-foreground);
          background-color: var(--vscode-sideBar-background);
          height: 100vh;
          box-sizing: border-box;
        }
        .sidebar {
          width: 30%;
          border-right: 1px solid var(--vscode-editorWidget-border);
          padding-right: 10px;
        }
        .sidebar h4 {
          margin-top: 0;
        }
        .sidebar ul {
          list-style: none;
          padding-left: 0;
        }
        .sidebar button {
          width: 100%;
          text-align: left;
          padding: 6px;
          border: none;
          background: none;
          color: var(--vscode-foreground);
          font-size: 13px;
          cursor: pointer;
        }
        .sidebar button:hover {
          background-color: var(--vscode-list-hoverBackground);
        }
        .content {
          flex: 1;
          padding-left: 20px;
          overflow-y: auto;
        }
        code {
          display: block;
          background-color: var(--vscode-editor-background);
          color: var(--vscode-editor-foreground);
          padding: 8px;
          margin: 6px 0;
          white-space: pre;
          border-radius: 4px;
          font-family: monospace;
        }
        button.vs-button {
          margin-top: 10px;
          padding: 5px 10px;
          background-color: var(--vscode-button-background);
          color: var(--vscode-button-foreground);
          border: none;
          border-radius: 3px;
          cursor: pointer;
        }
        button.vs-button:hover {
          background-color: var(--vscode-button-hoverBackground);
        }
      </style>
    </head>
    <body>

      <div class="sidebar">
        <h4>📚 Tutorials</h4>
        <ul>
          <li><button onclick="showTutorial('gettingStarted')">🧭 Getting Started</button></li>
          
          <li><button onclick="showTutorial('uploadLua')">📤 Upload Lua Script</button></li>

          <li><button onclick="showTutorial('ledTutorial')">Tutorial: Create an RGB Color Cycle Script</button></li>
        </ul>
      </div>

      <div id="content" class="content">
        <!-- Tutorial content loaded dynamically -->
      </div>

      <script>
        const vscode = acquireVsCodeApi();

        const tutorials = {
          gettingStarted: \`
            <h3>🧭 Getting Started with BrickLab</h3>
            <ol>
              <li>Plug in your ESP32</li>
              <li>Click <b>Connect</b> in the status bar</li>
              <li>Load a Lua script and click <b>Run</b></li>
            </ol>
            <button class="vs-button" onclick="highlight()">🔍 Where is Connect?</button>


          \`,

          uploadLua: \`
            <h3>📤 Uploading Lua Scripts</h3>
            <p>To upload a Lua script:</p>
            <ol>
              <li>Open your <code>.lua</code> file in the editor</li>
              <li>Click <b>Run Active Lua File</b> in the status bar</li>
              <li>Wait for the confirmation message</li>
            </ol>
          \`,
          ledTutorial:\`
            <h3>🎨 Tutorial: Create an RGB Color Cycle Script</h3>
            <p>Let’s build this step by step and understand <b>why</b> we do each part:</p>

            <h5>1️⃣ Load the BrickLab module</h5>
            <code>local brick_labs = require("brick_lab")</code>
            <p><b>Why?</b> This loads the BrickLab library. Without it, your script won’t know how to talk to your hardware.</p>

            <h5>2️⃣ Get the DeviceRgb class</h5>
            <code>local DeviceRgb = brick_labs.DeviceRgb</code>
            <p><b>Why?</b> This gives you access to the class used to control RGB lights.</p>

            <h5>3️⃣ Set the UUID of your device</h5>
            <code>local uuid = "424C1010-0000-0000-87CB-CF832BF0EFAD"</code>
            <p><b>Why?</b> This tells your script exactly which device to control. You’ll get this UUID when you connect your device in VS Code.</p>

            <h5>4️⃣ Set a delay between color changes</h5>
            <code>local delay_ms = 500</code>
            <p><b>Why?</b> This determines how long the device waits before switching to the next color. Without a delay, it changes too fast.</p>

            <h5>5️⃣ Define the RGB colors</h5>
            <code>
colors = {
  { red = 255, green = 255, blue = 255 },
  { red = 255, green = 0,   blue = 255 },
  ...
}
            </code>
            <p><b>Why?</b> This is your list of colors. Each color is a table with red, green, and blue values from 0 to 255.</p>

            <h5>6️⃣ Create the RGB device</h5>
            <code>local device = DeviceRgb.new(uuid)</code>
            <p><b>Why?</b> This creates the object that controls your RGB light. It connects your script to your real device.</p>

            <h5>7️⃣ Loop through the colors forever</h5>
            <code>
while true do
  for _, color in ipairs(colors) do
    device:set_rgb(color)
    delay(delay_ms)
  end
end
            </code>
            <p><b>Why?</b> This loop will run forever, changing the color of your device every 500ms through the list of colors.</p>
            \`
        };

        function showTutorial(key) {
          document.getElementById('content').innerHTML = tutorials[key];
        }

        function highlight() {
          vscode.postMessage({ command: 'highlightConnect' });
        }

        window.onload = () => showTutorial('gettingStarted');
      </script>

    </body>
    </html>
  `;
}


}
