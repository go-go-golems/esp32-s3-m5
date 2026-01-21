import { defineConfig } from 'vite'
import preact from '@preact/preset-vite'

// Deterministic asset names (no hashes) so firmware routes stay stable.
export default defineConfig({
  plugins: [preact()],
  base: '/',
  publicDir: false,
  build: {
    outDir: '../main/assets',
    emptyOutDir: true,
    cssCodeSplit: false,
    assetsDir: 'assets',
    rollupOptions: {
      output: {
        inlineDynamicImports: true,
        entryFileNames: 'assets/app.js',
        chunkFileNames: 'assets/app.js',
        assetFileNames: (assetInfo) => {
          if (assetInfo.name && assetInfo.name.endsWith('.css')) return 'assets/app.css'
          return 'assets/[name][extname]'
        },
      },
    },
  },
})

