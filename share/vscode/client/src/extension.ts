import * as path from 'path';
import { platform } from 'os';

import * as vscode from 'vscode';

import {
	LanguageClient,
	LanguageClientOptions,
	ServerOptions,
	TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

class MintConfigurationProvider implements vscode.DebugConfigurationProvider {

	resolveDebugConfiguration(folder: vscode.WorkspaceFolder | undefined, config: vscode.DebugConfiguration, token?: vscode.CancellationToken): vscode.ProviderResult<vscode.DebugConfiguration> {
		if (!config.type && !config.request && !config.name) {
			const editor = vscode.window.activeTextEditor;
			if (editor && editor.document.languageId === 'mint') {
				config.type = 'mint';
				config.name = 'Launch ${file}';
				config.request = 'launch';
				config.program = '${file}';
				config.stopOnEntry = true;
			}
		}

		if (!config.program) {
			return vscode.window.showInformationMessage("Cannot find a program to debug").then(_ => {
				return undefined;	// abort launch
			});
		}

		return config;
	}
}

class MdbgAdapterFactory implements vscode.DebugAdapterDescriptorFactory {
	createDebugAdapterDescriptor(_session: vscode.DebugSession): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {

		// The debugger is implemented by mdbg
		const debuggerDir = path.normalize(platform() === 'win32' ? 'C:/mint/bin' : '/bin');
		const debuggerCommand = path.join(debuggerDir, platform() === 'win32' ? 'mdbg.exe' : 'mdbg');

		// Options
		const debuggerOptions = {
			cwd: debuggerDir
		};

		return new vscode.DebugAdapterExecutable(debuggerCommand, ["--stdio"], debuggerOptions);
	}
}

class MdbgAdapterTrackerFactory implements vscode.DebugAdapterTrackerFactory {
	static mdbgChannel = vscode.window.createOutputChannel("Mdbg");
	createDebugAdapterTracker(_session: vscode.DebugSession): vscode.ProviderResult<vscode.DebugAdapterTracker> {
		return {
			onWillReceiveMessage: (m: any) => MdbgAdapterTrackerFactory.mdbgChannel.appendLine(`[info] To server: ${JSON.stringify(m)}`),
			onDidSendMessage: (m: any) => MdbgAdapterTrackerFactory.mdbgChannel.appendLine(`[info] From server: ${JSON.stringify(m)}`),
			onError: (e: Error) => MdbgAdapterTrackerFactory.mdbgChannel.appendLine(`[error] ${e.name}: ${e.message}`)
		};
	}
}

export function activate(context: vscode.ExtensionContext) {

	context.subscriptions.push(
		vscode.commands.registerCommand('extension.mint.runEditorContents', (resource: vscode.Uri) => {
			let targetResource = resource;
			if (!targetResource && vscode.window.activeTextEditor) {
				targetResource = vscode.window.activeTextEditor.document.uri;
			}
			if (targetResource) {
				vscode.debug.startDebugging(undefined, {
					type: 'mint',
					name: 'Run File',
					request: 'launch',
					program: targetResource.fsPath
				},
					{ noDebug: true }
				);
			}
		}),
		vscode.commands.registerCommand('extension.mint.debugEditorContents', (resource: vscode.Uri) => {
			let targetResource = resource;
			if (!targetResource && vscode.window.activeTextEditor) {
				targetResource = vscode.window.activeTextEditor.document.uri;
			}
			if (targetResource) {
				vscode.debug.startDebugging(undefined, {
					type: 'mint',
					name: 'Debug File',
					request: 'launch',
					program: targetResource.fsPath,
					stopOnEntry: true
				});
			}
		}));

	context.subscriptions.push(vscode.commands.registerCommand('extension.mint.getProgramName', config => {
		return vscode.window.showInputBox({
			placeHolder: 'Please enter the name of a mint script in the workspace folder',
			value: 'script.mn'
		});
	}));

	const provider = new MintConfigurationProvider();
	context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('mint', provider));

	const factory = new MdbgAdapterFactory();
	context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('mint', factory));

	const tracker = new MdbgAdapterTrackerFactory();
	context.subscriptions.push(vscode.debug.registerDebugAdapterTrackerFactory('mint', tracker));

	// The server is implemented in mint
	const serverDir = path.normalize(platform() === 'win32' ? 'C:/mint/bin' : '/bin');
	const serverInterpreter = path.join(serverDir, platform() === 'win32' ? 'mint.exe' : 'mint');
	const logFile = path.normalize(platform() === 'win32' ? 'C:/mint/minter.log' : '/tmp/minter.log');

	// If the extension is launched in debug mode then the debug server options are used
	// Otherwise the run options are used
	const serverOptions: ServerOptions = {
		run: {
			command: serverInterpreter,
			transport: TransportKind.stdio,
			args: ['minter', '--check-parent-process', '--log-file', logFile, '-vv'],
			options: { cwd: serverDir }
		},
		debug: {
			command: serverInterpreter,
			transport: TransportKind.stdio,
			args: ['minter', '--check-parent-process', '--log-file', logFile, '-vv'],
			options: { cwd: serverDir }
		}
	};

	// Options to control the language client
	const clientOptions: LanguageClientOptions = {
		// Register the server for mint documents
		documentSelector: [{ scheme: 'file', language: 'mint' }],
		synchronize: {
			// Notify the server about file changes to '.clientrc files contained in the workspace
			fileEvents: vscode.workspace.createFileSystemWatcher('**/.clientrc')
		}
	};

	// Create the language client and start the client.
	client = new LanguageClient(
		'MinterLanguageServer',
		'Minter',
		serverOptions,
		clientOptions
	);

	// Start the client. This will also launch the server
	client.start();
}

export function deactivate(): Thenable<void> | undefined {
	if (client) {
		return client.stop();
	}
	return undefined;
}
