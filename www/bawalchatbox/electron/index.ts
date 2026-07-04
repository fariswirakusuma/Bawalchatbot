import { app, BrowserWindow, ipcMain } from 'electron';
import { join } from 'path';
import { spawn, ChildProcessWithoutNullStreams } from 'child_process';

let mainWindow: BrowserWindow | null = null;
let engineProcess: ChildProcessWithoutNullStreams | null = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1024,
    height: 768,
    webPreferences: {
      preload: join(__dirname, '../preload/index.js'),
      contextIsolation: true,
      nodeIntegration: false,
    }
  });

  if (process.env.ELECTRON_RENDERER_URL) {
    mainWindow.loadURL(process.env.ELECTRON_RENDERER_URL);
  } else {
    mainWindow.loadFile(join(__dirname, '../renderer/index.html'));
  }
}

function startEngine() {
  // Mengarah ke ../../dist/chatbox_core relatif dari folder www
  const enginePath = join(__dirname, '../../../dist/chatbox_core');
  engineProcess = spawn(enginePath, ['--cli']);

  // Membaca output (stdout) dari C++ CLI
  engineProcess.stdout.on('data', (data) => {
    if (mainWindow) {
      mainWindow.webContents.send('engine-output', data.toString());
    }
  });

  // Membaca error (stderr) dari C++ CLI
  engineProcess.stderr.on('data', (data) => {
    if (mainWindow) {
      mainWindow.webContents.send('engine-error', data.toString());
    }
  });
}

app.whenReady().then(() => {
  createWindow();
  startEngine();

  // Menerima perintah dari React dan meneruskannya ke engine C++
  ipcMain.on('execute-command', (event, command: string) => {
    if (engineProcess) {
      // Mengirim input ke std::cin di main_cli.cpp yang dievaluasi oleh std::getline[cite: 2]
      engineProcess.stdin.write(command + '\n');
    }
  });
});

app.on('window-all-closed', () => {
  if (engineProcess) {
    engineProcess.kill();
  }
  if (process.platform !== 'darwin') app.quit();
});