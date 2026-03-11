import { defineConfig, loadEnv } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, '.', '')
  const backendOrigin = env.VITE_BACKEND_ORIGIN || 'http://127.0.0.1:18080'
  const backendWsOrigin = backendOrigin.replace(/^http/, 'ws')

  return {
    plugins: [react()],
    server: {
      proxy: {
        '/api': backendOrigin,
        '/ws': {
          target: backendWsOrigin,
          ws: true,
        },
      },
    },
  }
})
