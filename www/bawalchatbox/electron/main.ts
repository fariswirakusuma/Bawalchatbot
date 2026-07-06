import { app, BrowserWindow, ipcMain } from 'electron';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn, ChildProcessWithoutNullStreams } from 'node:child_process';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const VITE_DEV_SERVER_URL = process.env['VITE_DEV_SERVER_URL'];
const RENDERER_DIST = path.join(__dirname, '../dist');
process.env.VITE_PUBLIC = VITE_DEV_SERVER_URL ? path.join(__dirname, '../public') : RENDERER_DIST;

let mainWindow: BrowserWindow | null = null;
let engineProcess: ChildProcessWithoutNullStreams | null = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    backgroundColor: '#0f111a',
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false
    },
  });

  if (VITE_DEV_SERVER_URL) {
    mainWindow.loadURL(VITE_DEV_SERVER_URL);
  } else {
    mainWindow.loadFile(path.join(RENDERER_DIST, 'index.html')).catch((err) => {
      console.error("CRITICAL: Gagal memuat HTML produksi ->", err);
    });
  }

  mainWindow.webContents.openDevTools();

  mainWindow.on('closed', () => {
    mainWindow = null;
    if (engineProcess) {
      engineProcess.kill();
    }
  });
}

function initBackend() {
  const enginePath = path.join(__dirname, '../../../dist/chatbox_core');
  const workspaceRoot = path.join(__dirname, '../../../');
  try {
    engineProcess = spawn(enginePath, [], { cwd: workspaceRoot });
    engineProcess.stdout.on('data', (data) => {
      if (mainWindow) {
        mainWindow.webContents.send('engine-output', data.toString());
      }
    });

    engineProcess.stderr.on('data', (data) => {
      if (mainWindow) {
        mainWindow.webContents.send('engine-error', data.toString());
      }
    });

    engineProcess.on('close', (code) => {
      console.log(`Proses backend C++ keluar dengan kode: ${code}`);
    });

    engineProcess.on('error', (err) => {
      console.error(`CRITICAL ERROR: Biner backend C++ tidak ditemukan atau gagal dieksekusi di: ${enginePath}`);
      console.error(err); 
    });

  } catch (error) {
    console.error("Gagal melakukan spawn process:", error);
  }
}

app.whenReady().then(() => {
  initBackend();
  createWindow();
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

ipcMain.on('execute-command', (_event, command: string) => {
  if (engineProcess && engineProcess.stdin.writable) {
    engineProcess.stdin.write(command + '\n');
  }
});