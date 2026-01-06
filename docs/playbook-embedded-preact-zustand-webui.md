---
Title: "Playbook: Embedding a Preact + Zustand Web UI inside ESP-IDF firmware (Vite bundling)"
Ticket: 0013-ATOMS3R-WEBSERVER
Status: active
Topics:
  - preact
  - zustand
  - webserver
  - esp-idf
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
  - Path: esp32-s3-m5/0017-atoms3r-web-ui/web/vite.config.ts
    Note: Concrete Vite bundling config producing deterministic, embed-friendly outputs
  - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/CMakeLists.txt
    Note: Uses ESP-IDF `EMBED_TXTFILES` to compile web artifacts into firmware
  - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp
    Note: Serves embedded JS/CSS/HTML over `esp_http_server` and implements `/api/*` + `/ws`
  - Path: esp32-s3-m5/0017-atoms3r-web-ui/web/src/store/ws.ts
    Note: Minimal Zustand WebSocket store (binary + JSON text) suitable for embedded devices
Summary: "A practical, JS-focused playbook for bundling a Vite (Preact + Zustand) SPA into an ESP-IDF firmware image and serving it as deterministic embedded assets."
LastUpdated: 2026-01-05
WhatFor: "When you want a device-hosted web UI (SPA) with predictable asset names, tiny payloads, and a firmware-friendly serving strategy."
WhenToUse: "Use for ESP32/ESP-IDF projects embedding a small JS UI into flash; prefer over external hosting when you need offline + single-binary deployment."
---

# Playbook: Embedding a Preact + Zustand Web UI inside ESP-IDF firmware (Vite bundling)

## Executive summary

This playbook explains how to structure, build, and bundle a small **Vite + Preact + Zustand** web UI so it can be **embedded into an ESP-IDF firmware** and served by the device (no external web server needed). The concrete worked example is Tutorial `esp32-s3-m5/0017-atoms3r-web-ui`, but the patterns are reusable for any ESP-IDF project that exposes an HTTP API and an optional WebSocket.

The key idea is to make the frontend build output **deterministic and firmware-friendly**: one JS file, one CSS file, a stable `index.html`, and stable URL paths (no hashed filenames) so the firmware can register simple routes and never needs to be edited when the frontend is rebuilt.

## Scope / non-scope

This document focuses on the **JavaScript/TypeScript build and project layout**, plus the minimal firmware touchpoints needed to serve embedded files. It does not try to re-teach WiFi, FATFS, display rendering, or UART; those are covered by the broader `0013-ATOMS3R-WEBSERVER` ticket docs under `ttmp/`.

## Mental model (what you are building)

You are building two artifacts that must “click” together:

1) A browser application (SPA) that makes requests to:
- `GET /api/...` (HTTP JSON endpoints)
- `ws://<device>/ws` (WebSocket for realtime)

2) A firmware web server that:
- serves `/` with `index.html`
- serves `/assets/app.js` and `/assets/app.css`
- implements `/api/*` and `/ws`

The build pipeline ties them together by copying (or writing) the frontend outputs into a firmware-owned folder that is compiled into flash.

```
┌───────────────────────────────┐
│ web/ (Vite + Preact + Zustand)│
│  - src/*.tsx                  │
│  - vite.config.ts             │
└───────────────┬───────────────┘
                │ npm run build
                ▼
┌───────────────────────────────┐
│ firmware main/assets/          │
│  - index.html                  │
│  - assets/app.js               │
│  - assets/app.css              │
└───────────────┬───────────────┘
                │ idf.py build
                ▼
┌───────────────────────────────┐
│ firmware image                 │
│  - embedded blobs (_binary_*)  │
│  - HTTP routes return blobs    │
└───────────────────────────────┘
```

## Constraints (why the “one file, deterministic names” rule matters)

In desktop web apps, it’s normal to emit many chunks with hashed filenames (cache busting). In device firmware, that creates friction:

- The firmware must either (a) know all emitted filenames, or (b) parse a manifest at runtime.
- The browser can cache aggressively and then “brick” your UI view if filenames are stable but content changes.
- Each additional file usually means another route, another content-type decision, and another embedded symbol.

For embedded UIs, stability and simplicity often beat sophistication:

- Prefer **1 JS + 1 CSS** initially.
- Prefer **no hashed filenames**, and compensate with conservative cache headers.

## Recommended project layout (frontend)

Use a small but scalable layout. The Tutorial `0017` is deliberately minimal.

```
web/
  package.json
  package-lock.json
  index.html
  vite.config.ts
  tsconfig*.json
  src/
    main.tsx          # app bootstrap + global CSS
    app.tsx           # top-level UI
    api.ts            # typed wrappers around /api
    store/
      ws.ts           # Zustand WS state + reconnect + binary/text decoding
    *.css             # small, inlined CSS
```

### Why this split works well on firmware-hosted UIs

- `api.ts` is a single place to make `fetch` decisions (paths, error handling, timeouts).
- `store/ws.ts` centralizes WebSocket lifecycle and binary/text parsing.
- Your components stay “boring”: they read state and call actions.

## Vite bundling rules for firmware embedding

This section is the “recipe”. The goal is: **produce deterministic files directly into the firmware asset directory**.

### Target output shape

From `esp32-s3-m5/0017-atoms3r-web-ui/main/assets/`:

```
main/assets/
  index.html
  assets/
    app.js
    app.css
```

### Vite config (worked example)

See `esp32-s3-m5/0017-atoms3r-web-ui/web/vite.config.ts`:

- `outDir: '../main/assets'` writes outputs into the firmware folder (no extra copy step).
- `emptyOutDir: true` ensures removed assets don’t linger and get accidentally embedded.
- `cssCodeSplit: false` forces a single CSS file.
- `inlineDynamicImports: true` forces a single JS file (no chunks).
- deterministic names:
  - `assets/app.js`
  - `assets/app.css`
- `publicDir: false` avoids copying `public/` files (like `vite.svg`) into firmware by accident.

Pseudocode version (the intent):

```ts
export default defineConfig({
  base: '/',
  publicDir: false,
  build: {
    outDir: '../main/assets',
    emptyOutDir: true,
    cssCodeSplit: false,
    rollupOptions: {
      output: {
        inlineDynamicImports: true,
        entryFileNames: 'assets/app.js',
        chunkFileNames: 'assets/app.js',
        assetFileNames: (a) => a.name?.endsWith('.css') ? 'assets/app.css' : 'assets/[name][extname]',
      },
    },
  },
})
```

### `index.html` rule

Your built `index.html` must reference stable URLs that your firmware serves:

```html
<script type="module" src="/assets/app.js"></script>
<link rel="stylesheet" href="/assets/app.css">
```

This is exactly what the built file looks like in `esp32-s3-m5/0017-atoms3r-web-ui/main/assets/index.html`.

## Frontend API conventions (keep it firmware-friendly)

### Use relative URLs and a single origin

In the browser, prefer:
- `fetch('/api/status')` not `fetch('http://192.168.4.1/api/status')`

This makes the app work identically when:
- served from the device, and
- served from a dev server proxy (see the dev loop below).

### Prefer “raw body uploads” over `multipart/form-data`

The tutorial uses:

- `PUT /api/graphics/<name>` with the raw `File` body.

In `esp32-s3-m5/0017-atoms3r-web-ui/web/src/api.ts`:

```ts
await fetch(`/api/graphics/${name}`, { method: 'PUT', body: file })
```

This is a good embedded default because the firmware can stream bytes from the request (`httpd_req_recv`) into storage without building a multipart parser.

### Keep client-side filename rules aligned with firmware validation

The UI sanitizes names (see `sanitizeName` in `esp32-s3-m5/0017-atoms3r-web-ui/web/src/app.tsx`) to match firmware-side “no slashes / no dot-dot” style restrictions:

```ts
name.replace(/[^a-zA-Z0-9._-]/g, '_')
```

## Zustand WebSocket store: a minimal, robust pattern

The `esp32-s3-m5/0017-atoms3r-web-ui/web/src/store/ws.ts` store shows a good “firmware UI” baseline:

- one global WebSocket connection
- simple reconnect backoff
- `binaryType = 'arraybuffer'`
- two message channels:
  - **text** frames containing JSON control messages
  - **binary** frames containing UART bytes

Message handling pattern:

```text
onmessage(ev):
  if ev.data is string:
    parse JSON; append button events
  else:
    decode bytes as UTF-8 streaming; append to terminal buffer
```

### Recommended evolutions for larger UIs

If you expect the UI to grow, add just enough structure without turning it into “enterprise React”:

- Define a discriminated union for JSON messages:
  - `type: 'button' | 'status' | 'log' | ...`
- Put the schema in one place (`src/protocol.ts`) and import it from both store and UI.
- Consider splitting the store into slices (`wsSlice`, `uiSlice`, `telemetrySlice`) to avoid giant state objects.

## Firmware touchpoints (only what the JS developer must know)

Even when “focusing on JS”, there are three firmware integration facts you must keep straight:

1) How the artifacts are embedded into flash
2) What URL paths the firmware serves
3) What headers the firmware sets (content-type, caching, optional gzip)

### Embedding files: `EMBED_TXTFILES` vs `EMBED_FILES`

In ESP-IDF component CMake, `EMBED_TXTFILES` compiles files into the firmware and provides linkable symbols. In `esp32-s3-m5/0017-atoms3r-web-ui/main/CMakeLists.txt`:

```cmake
idf_component_register(
  ...
  EMBED_TXTFILES
    "assets/index.html"
    "assets/assets/app.js"
    "assets/assets/app.css"
)
```

Rules of thumb:

- Use `EMBED_TXTFILES` for HTML/CSS/JS when serving them as text.
- Use `EMBED_FILES` for binary blobs (e.g., `.gz`, `.png`, `.wasm`).

Official ESP-IDF build-system docs (embedding data):
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html

### Symbol naming (practical note)

The embedded data is exposed as linker symbols that are usually based on the **basename** of the file (e.g., `index.html` → `_binary_index_html_start`). That is why `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp` can declare:

```c
extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[]   asm("_binary_index_html_end");
```

Avoid embedding multiple files with the same basename in one component (or you risk symbol collisions).

### Serving assets (content types)

The firmware must respond with correct `Content-Type`:

- `index.html` → `text/html; charset=utf-8`
- `app.js` → `application/javascript`
- `app.css` → `text/css`

In `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`, each handler sets a type and calls `httpd_resp_send(...)` over the embedded byte range.

ESP-IDF HTTP server reference:
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_http_server.html

## Build integration patterns (how to keep the assets current)

There are two common ways teams keep the embedded assets in sync with firmware builds. Pick intentionally; both are valid.

### Pattern 1: commit the built assets (`main/assets/`) to the repo

This matches the spirit of “single-toolchain firmware builds”:

- CI and developers can run `idf.py build` without Node installed.
- Your firmware build is stable even if the npm ecosystem changes.

Trade-offs:
- You must remember to rebuild assets when UI changes.
- Diffs can be noisy (minified JS).

### Pattern 2: make the firmware build run `npm run build`

This keeps assets always current and avoids committing generated files.

The usual ESP-IDF/CMake approach is:

```text
add_custom_target(web_assets
  COMMAND npm ci
  COMMAND npm run build
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../web
)

add_dependencies(${COMPONENT_LIB} web_assets)
```

Notes:
- Prefer `npm ci` when you have a lockfile.
- If Vite writes into `main/assets/`, you must ensure the embed step sees the updated files (CMake configure vs build ordering can be subtle).
- For large repos, consider a stamp file (only rebuild when `web/src/**` changes).

## Compression and caching (the two sharp edges of deterministic filenames)

### Gzip (recommended once the UI is stable)

JS/CSS compress extremely well. A typical approach:

1) Build normally (produces `app.js`, `app.css`)
2) Gzip them at build time, embed the `.gz` files
3) Serve with `Content-Encoding: gzip`

Firmware serving rules:
- Response body must be the gzipped bytes
- Set:
  - `Content-Type: application/javascript` (or `text/css`)
  - `Content-Encoding: gzip`

In ESP-IDF, this usually means:
- embed `.gz` via `EMBED_FILES` (binary)
- serve bytes and add header:
  - `httpd_resp_set_hdr(req, "Content-Encoding", "gzip")`

### Cache-Control (especially during development)

If you keep deterministic filenames (`/assets/app.js`), browsers may cache aggressively and you’ll think your firmware is “not updating”.

Pragmatic defaults:
- For `index.html`: `Cache-Control: no-store`
- For JS/CSS during active development: `Cache-Control: no-store`
- For “release-ish” builds: allow caching, but accept you may need a manual refresh on phones (or move to hashed filenames + manifest)

## Scaling beyond 1 JS + 1 CSS

The “single file” configuration is a deliberate constraint. If you grow beyond it, decide which complexity you want:

### Option 1: keep deterministic names, serve multiple files

- Remove `inlineDynamicImports: true`
- Vite emits additional chunks
- You must embed and serve every emitted file under `/assets/*`

At that point, consider a small “asset table” in firmware:

```text
asset_table = [
  { path: "/assets/app.js",  type: "application/javascript", start: _binary_app_js_start,  end: _binary_app_js_end },
  { path: "/assets/chunk1.js", type: "...", start: ..., end: ... },
  ...
]

handler("/assets/*"):
  look up by req->uri
  set Content-Type
  send bytes
```

### Option 2: hashed filenames + Vite manifest (more web-like)

This is more complex but solves caching cleanly:
- Vite emits `manifest.json` mapping logical entries → hashed filenames
- You embed the manifest and assets
- Your firmware serves:
  - `/` by returning an `index.html` that points to the current hashed filenames (either by templating `index.html` at build time, or by serving `index.html` generated by Vite and ensuring the hashed files are also served)

For embedded projects, Option 1 is usually enough until the UI becomes genuinely large.

## Development loop options (pick one)

### Option A (simplest): rebuild assets, then rebuild firmware

This is the most reliable and easiest to reason about:

```bash
cd esp32-s3-m5/0017-atoms3r-web-ui/web
npm ci
npm run build   # emits into ../main/assets/

cd ..
idf.py build
```

Trade-off: frontend iteration is slower because it’s tied to firmware builds.

### Option B (fast UI iteration): Vite dev server + proxy to device API/WS

For UI-heavy work, run Vite locally and proxy calls to the device. This keeps the device as the backend but avoids rebuilding firmware for every CSS change.

Pseudocode Vite proxy config:

```ts
export default defineConfig({
  server: {
    proxy: {
      '/api': { target: 'http://192.168.4.1', changeOrigin: true },
      '/ws':  { target: 'ws://192.168.4.1', ws: true },
    },
  },
})
```

Key requirement: keep frontend requests **relative** (`/api/...`, `/ws`) so the same UI works when served from the device.

### Option C (runtime-updatable assets): serve from a filesystem partition

Instead of embedding, store the built assets in FATFS/SPIFFS/LittleFS and serve them from there. This allows OTA-like updates of the UI without reflashing the whole app.

Trade-off: significantly more complexity (filesystem, caching, integrity checks, update mechanism). Start with embedding unless you really need runtime updates.

## Troubleshooting / common failure modes

### The UI loads but JS 404s

Symptom: browser shows a blank page, devtools network tab shows `GET /assets/app.js 404`.

Checklist:
- Does the built `main/assets/index.html` reference `/assets/app.js` (not `assets/app.js`)?
- Does the firmware register a handler for `/assets/app.js`?
- Did Vite output into the exact directory the firmware embeds?
  - `outDir: '../main/assets'`
  - `assetsDir: 'assets'`

### “It works once, then changes don’t show up”

This is usually caching. If you use deterministic filenames, you must be intentional:

- During active development, set:
  - `Cache-Control: no-store` for HTML (and optionally JS/CSS).
- For “release-like” behavior, prefer:
  - hashed filenames + a manifest, or
  - pre-gzip + long cache, and accept that the firmware must embed/serve the new hashes.

### WebSocket connects but you receive gibberish in the terminal

Checklist:
- Ensure `ws.binaryType = 'arraybuffer'`.
- Ensure you decode bytes intentionally:
  - for text logs: `TextDecoder('utf-8', { fatal: false })`
  - for arbitrary bytes: render hex, don’t assume UTF-8
- Ensure the firmware is sending text vs binary consistently (don’t mix without schema).

### Bundle size grows and you hit flash limits

Quick wins:
- remove dependencies (the fastest way to shrink)
- avoid large icon packs / UI frameworks
- ensure no sourcemaps are embedded
- consider gzip (serve `Content-Encoding: gzip`), which often yields large wins for JS/CSS

## Appendix: “small UI” checklist (copy/paste for new projects)

- [ ] `web/` uses Vite + Preact preset; dependencies limited to `preact` + `zustand`.
- [ ] `vite.config.ts` emits deterministic `assets/app.js` and `assets/app.css`.
- [ ] Vite build output goes directly into `../main/assets` (or there is a deterministic copy step).
- [ ] Firmware embeds `index.html`, `app.js`, `app.css` via `EMBED_TXTFILES` (or `.gz` via `EMBED_FILES`).
- [ ] Firmware serves:
  - `GET /` → `index.html`
  - `GET /assets/app.js`
  - `GET /assets/app.css`
- [ ] Frontend uses relative URLs (`/api/*`, `/ws`) and has one WS store with reconnect.
