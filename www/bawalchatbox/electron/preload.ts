import { contextBridge, ipcRenderer } from 'electron'

contextBridge.exposeInMainWorld('electronAPI', {
  sendToBackend: (cmd: string) => ipcRenderer.send('execute-command', cmd),
  
  // Menerima output dari backend
  onBackendOut: (callback: (data: string) => void) => {
    const listener = (_event: any, data: string) => callback(data);
    ipcRenderer.on('engine-output', listener);
    
    return () => {
      ipcRenderer.removeListener('engine-output', listener);
    };
  },

  onBackendErr: (callback: (data: string) => void) => {
    const listener = (_event: any, data: string) => callback(data);
    ipcRenderer.on('engine-error', listener);
    return () => {
      ipcRenderer.removeListener('engine-error', listener);
    };
  }
})