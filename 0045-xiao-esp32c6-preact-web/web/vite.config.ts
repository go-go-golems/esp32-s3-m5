import { defineConfig } from 'vite'
import preact from '@preact/preset-vite'

// Deterministic asset names (no hashes) so firmware routes stay stable.
export default defineConfig({
  plugins: [preact()],
  base: '/',
  // Avoid copying `public/` into the firmware asset directory (e.g. `vite.svg`).
  publicDir: false,
  build: {
    // Write build outputs directly into the firmware-embedded asset directory.
    outDir: '../main/assets',
    emptyOutDir: true,
    cssCodeSplit: false,
    assetsDir: 'assets',
    rollupOptions: {
      output: {
        // Single JS file (no dynamic chunks) for embedding in firmware.
        inlineDynamicImports: true,
        entryFileNames: 'assets/app.js',
        chunkFileNames: 'assets/app.js',
        assetFileNames: (assetInfo) => {
          if (assetInfo.name && assetInfo.name.endsWith('.css')) {
            return 'assets/app.css'
          }
          return 'assets/[name][extname]'
        },
      },
    },
  },
})

