import { app, BrowserWindow, ipcMain } from 'electron';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn, ChildProcessWithoutNullStreams } from 'node:child_process';

// FIX: Definisikan __dirname secara manual agar valid di lingkungan ES Module (.js)
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
      // Pastikan mengarah ke preload.js hasil kompilasi asli
      preload: path.join(__dirname, 'preload.js'),
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false
    },
  });

  if (VITE_DEV_SERVER_URL) {
    mainWindow.loadURL(VITE_DEV_SERVER_URL);
    mainWindow.webContents.openDevTools();
  } else {
    mainWindow.loadFile(path.join(RENDERER_DIST, 'index.html')).catch((err) => {
      console.error("CRITICAL: Gagal memuat HTML produksi ->", err);
    });
  }

  mainWindow.on('closed', () => {
    mainWindow = null;
    if (engineProcess) {
      engineProcess.kill();
    }
  });
}

function initBackend() {
  // Jalur absolut mundur 3 tingkat menuju WORKSPACE_ROOT/dist/chatbox_core
  const enginePath = path.join(__dirname, '../../../dist/chatbox_core');

  try {
    engineProcess = spawn(enginePath, []);

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
      console.error(err); // Variabel sekarang dibaca untuk mencetak stack trace error asli
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