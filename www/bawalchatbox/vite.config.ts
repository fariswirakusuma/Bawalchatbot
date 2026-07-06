import { defineConfig } from 'vite'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import electron from 'vite-plugin-electron/simple'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'

const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

export default defineConfig({
  base: './',
  plugins: [
    react(),
    tailwindcss(),

    electron({
      main: {
        entry: 'electron/main.ts',
        vite: {
          build: {
            rollupOptions: {
              output: {
                entryFileNames: 'main.js'
              }
            }
          }
        }
      },
      preload: {
        input: path.join(__dirname, 'electron/preload.ts'),
        vite: {
          build: {
            rollupOptions: {
              output: {
                format: 'cjs', 
                entryFileNames: 'preload.cjs'
              }
            }
          }
        }
      },
      renderer: process.env.NODE_ENV === 'test' ? undefined : {},
    }),
  ],
})