import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';

export function activate(context: vscode.ExtensionContext) {
    let disposable = vscode.commands.registerCommand('bricklab.createProject', async () => {
        const projectName = await vscode.window.showInputBox({ prompt: 'Enter BrickLab Project Name' });
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
        fs.writeFileSync(path.join(projectPath, 'main.lua'), `-- main.lua for ${projectName}\n\nprint("Project ready!")\n\n-- [Run Button Placeholder]`);

        vscode.window.showInformationMessage(`BrickLab project "${projectName}" created.`);
    });
    let runCommand = vscode.commands.registerCommand('bricklab.runLuaFile', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showErrorMessage('No active editor found.');
            return;
        }
    
        const code = editor.document.getText();
        // Future: Convert to bytes and send over BLE
        vscode.window.showInformationMessage('Code sent to ESP (simulated).');
    });
    
    context.subscriptions.push(disposable);
    context.subscriptions.push(runCommand);
}

export function deactivate() {}
