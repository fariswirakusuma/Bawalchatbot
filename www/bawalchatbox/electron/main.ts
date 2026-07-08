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
      sandbox: true // PERBAIKAN: Aktifkan sandbox untuk mengamankan proses renderer
    },
  });

  if (VITE_DEV_SERVER_URL) {
    mainWindow.loadURL(VITE_DEV_SERVER_URL);
  } else {
    mainWindow.loadFile(path.join(RENDERER_DIST, 'index.html')).catch((err) => {
      console.error("CRITICAL: Gagal memuat HTML produksi ->", err);
    });
  }

  if (process.env.DEBUG === 'true') {
    mainWindow.webContents.openDevTools();
  }

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
        const rawOutput = data.toString();
        const lines = rawOutput.split('\n');
        
        for (const line of lines) {
            const trimmedLine = line.trim();
            if (!trimmedLine) continue; 
            mainWindow.webContents.send('engine-output', trimmedLine);
        }
      }
    });

    engineProcess.stderr.on('data', (data) => {
      if (mainWindow) {
        mainWindow.webContents.send('engine-error', data.toString());
      }
    });

    engineProcess.on('close', (code) => {
      console.log(`Proses backend C++ keluar dengan kode: ${code}`);
      mainWindow = null;
      app.quit();
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

function sanitizeCommand(cmd: string): string {
  const controlRegex = new RegExp('[\\r\\x00-\\x1F\\x7F]', 'g');
  return cmd.replace(controlRegex, ''); 
}
ipcMain.on('execute-command', (_event, command: string) => {
  if (typeof command !== 'string') return; 

  const sanitized = sanitizeCommand(command);
  if (engineProcess && engineProcess.stdin.writable) {
    engineProcess.stdin.write(sanitized + '\n');
  }
});