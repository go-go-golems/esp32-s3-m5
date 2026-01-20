---
name: esp-idf-embedded-preact-zustand-webui
description: Find, reuse, and implement the repo’s proven pattern for a device-hosted web UI bundled with Vite (Preact + Zustand) and embedded into an ESP-IDF firmware image, then served over esp_http_server (optionally with WebSocket). Use when you need the existing playbook + working firmware example, want the exact Vite/CMake knobs for deterministic asset names, or need to locate the key docs/files (0017 tutorial + 0013 ticket) quickly.
---

# ESP-IDF embedded Preact+Zustand Web UI (repo pattern)

## Quick anchors (open these first)

- Open `docs/playbook-embedded-preact-zustand-webui.md` for the JS/Vite→firmware embedding playbook.
- Use `0017-atoms3r-web-ui/` as the worked firmware example:
  - `0017-atoms3r-web-ui/web/vite.config.ts` (deterministic output, single JS, outputs into `main/assets/`)
  - `0017-atoms3r-web-ui/main/CMakeLists.txt` (`EMBED_TXTFILES` for `index.html`, `assets/app.js`, `assets/app.css`)
  - `0017-atoms3r-web-ui/main/http_server.cpp` (routes for `/`, `/assets/app.js`, `/assets/app.css`, `/api/*`, `/ws`)
  - `0017-atoms3r-web-ui/web/src/store/ws.ts` (minimal Zustand WebSocket store pattern)

## Fast search workflow (docmgr + rg)

1) Use docmgr to find the authored docs and ticket trail:
   - `docmgr doc search --query "preact"`
   - `docmgr doc search --query "zustand"`
   - `docmgr doc search --query "deterministic filenames"`
2) Use ripgrep to jump to implementation:
   - `rg -n -S "outDir: '../main/assets'|inlineDynamicImports|entryFileNames: 'assets/app.js'|EMBED_TXTFILES|/assets/app.js|/ws" 0017-atoms3r-web-ui`
3) If you need end-to-end bring-up steps, open:
   - `ttmp/2025/12/26/0013-ATOMS3R-WEBSERVER--atoms3r-web-server-graphics-upload-and-websocket-terminal/playbooks/01-verification-playbook-atoms3r-web-ui.md`

## Reuse checklist (keep firmware-friendly)

- Emit deterministic asset names (`/assets/app.js`, `/assets/app.css`) and avoid extra files (`publicDir: false`, `emptyOutDir: true`).
- Keep the initial UI to “1 JS + 1 CSS” (disable chunking via `inlineDynamicImports: true`).
- Embed assets with `EMBED_TXTFILES` (or `EMBED_FILES` for binary/gz) and ensure basenames won’t collide.
