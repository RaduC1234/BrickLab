// src/utils/liveHints.ts
import * as vscode from 'vscode';

export function showLiveHint(message: string, uri: vscode.Uri, line: number) {
  const decoType = vscode.window.createTextEditorDecorationType({
    after: {
      contentText: `💡 Hint: ${message}`,
      color: new vscode.ThemeColor('editorHint.foreground'),
      margin: '10px'
    }
  });

  const editor = vscode.window.visibleTextEditors.find(e => e.document.uri.fsPath === uri.fsPath);
  if (editor) {
    const range = new vscode.Range(line, 0, line, 0);
    editor.setDecorations(decoType, [{ range }]);

    // Auto-remove the hint after 8 seconds
    setTimeout(() => decoType.dispose(), 8000);
  }
}
