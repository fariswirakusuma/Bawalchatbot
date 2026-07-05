import { contextBridge, ipcRenderer } from 'electron'

contextBridge.exposeInMainWorld('electronAPI', {
  sendToBackend: (cmd: string) => ipcRenderer.send('send-to-backend', cmd),
  onBackendOut: (callback: (data: string) => void) =>
    ipcRenderer.on('backend-out', (_event, data) => callback(data)),
  onBackendErr: (callback: (data: string) => void) =>
    ipcRenderer.on('backend-err', (_event, data) => callback(data))
})