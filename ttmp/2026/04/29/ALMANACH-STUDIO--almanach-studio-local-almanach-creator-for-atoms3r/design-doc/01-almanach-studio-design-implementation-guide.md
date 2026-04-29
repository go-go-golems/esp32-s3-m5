---
title: Almanach Studio Design & Implementation Guide
doc_type: design-doc
status: active
intent: long-term
topics:
  - esp32-s3
  - http-server
  - react
  - static-embed
  - thermal-printer
ticket: ALMANACH-STUDIO
created: 2026-04-29
---

# Almanach Studio — Design & Implementation Guide

## Executive Summary

Almanach Studio is a React-based single-page application (SPA) that lets users design beautiful thermal-printer-style almanac pages — daily digest sheets with sections for weather, news, habits, quotes, and more. The user wants to host this SPA locally on an **AtomS3R** device (ESP32-S3), served at the route `/almanach`, using a **static compile** strategy that avoids a heavy Node.js build toolchain on the device itself.

The core idea is straightforward: use **Babel in the browser** to transform the JSX at runtime, load React and Lucide icons from a CDN or from embedded copies, and serve everything as static files embedded into the ESP32 firmware binary. The AtomS3R already runs an HTTP server (see firmware `0017-atoms3r-web-ui`); we add new URI handlers for `/almanach` and its static assets.

This document is a **complete intern-friendly guide** that explains every layer of the system — the React component, the JSX toolchain strategy, the ESP32 HTTP server architecture, the asset embedding mechanism, and the step-by-step implementation plan.

---

## 1. What Is Almanach Studio?

### 1.1 Purpose

Almanach Studio is a **visual layout designer for daily almanac pages**. Think of it as a Canva-like tool specifically designed for thermal-printer output — those small receipt-sized prints you get from portable thermal printers (58mm or 80mm paper width).

The user:

1. Picks content blocks from a library (title, date, weather, news, habits, quote, etc.)
2. Edits the content of each block in a right-side inspector panel
3. Chooses a visual theme (Classic, Minimal, Botanical, Notebook, Vintage Ledger, Space Age)
4. Adjusts the paper width to match their thermal printer
5. Exports the result as PNG (for printing) or JSON (for saving/loading layouts)

### 1.2 Key Features

- **15 content block types**: title, date strip, divider, plan, news, weather, note, habit tracker, mood/energy, reading list, reflection, quote, word of the day, today in history, did you know
- **6 visual themes**: Classic (ornate double-border frame), Minimal, Botanical (corner ❦ decorations), Notebook (handwritten fonts + lined paper), Vintage Ledger (boxed sections), Space Age (dark background with star field)
- **Thermal paper preview**: zigzag torn edges, paper grain texture, drop shadows
- **PNG export**: renders the paper via SVG `foreignObject` at 2× resolution, with inlined fonts
- **JSON save/load**: full layout state serialization
- **Print mode**: `@media print` CSS strips the editor chrome and prints just the paper

### 1.3 Technology Stack

| Layer | Technology | Size (approx.) |
|-------|-----------|----------------|
| UI framework | React 18 | ~45 KB (min+gzip) |
| Icons | Lucide React | ~2 KB per icon (tree-shakeable) |
| Fonts | Google Fonts (Cormorant Garamond, EB Garamond, Caveat, Kalam, DM Sans) | ~200 KB total |
| Styling | Inline CSS-in-JS (no build step needed) | N/A |
| Transforms | Babel standalone (browser-side JSX) | ~3 MB (or precompiled: 0) |
| Runtime | Browser on laptop/tablet/phone connecting to AtomS3R | N/A |

### 1.4 Architecture Diagram

```
┌──────────────────────────────────────────────────────────┐
│                    BROWSER (CLIENT)                       │
│                                                          │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐  │
│  │  React 18   │  │ Babel        │  │  Lucide Icons  │  │
│  │  (CDN or    │  │ Standalone   │  │  (tree-shaken  │  │
│  │   embedded) │  │ (transforms  │  │   into bundle) │  │
│  │             │  │  JSX→JS)     │  │                │  │
│  └──────┬──────┘  └──────┬───────┘  └───────┬────────┘  │
│         │                │                   │           │
│         └────────────────┼───────────────────┘           │
│                          │                               │
│              ┌───────────▼───────────┐                   │
│              │  almanach-studio.jsx  │                   │
│              │  (the SPA component)  │                   │
│              └───────────┬───────────┘                   │
│                          │ HTTP GET                       │
└──────────────────────────┼───────────────────────────────┘
                           │
                     Wi-Fi / LAN
                           │
┌──────────────────────────┼───────────────────────────────┐
│                   ESP32-S3 (AtomS3R)                      │
│                          │                               │
│              ┌───────────▼───────────┐                   │
│              │  esp_http_server      │                   │
│              │  (port 80)            │                   │
│              ├───────────────────────┤                   │
│              │ GET /almanach         │ → index.html      │
│              │ GET /almanach/app.js  │ → almanach.js     │
│              │ GET /almanach/app.css │ → almanach.css    │
│              │ GET /almanach/react   │ → react+react-dom │
│              └───────────────────────┘                   │
│                          ▲                               │
│              ┌───────────┴───────────┐                   │
│              │  EMBEDDED ASSETS      │                   │
│              │  (in firmware binary) │                   │
│              │  via EMBED_TXTFILES   │                   │
│              └───────────────────────┘                   │
└──────────────────────────────────────────────────────────┘
```

---

## 2. The JSX Source — Deep Dive

### 2.1 File Overview

The source file is `almanach-studio(1).jsx` — a **single-file React component** (~2100 lines). It contains everything: data models, block renderers, block editors, the thermal paper component, export helpers, and the main app component.

This is the canonical structure of the file:

```
almanach-studio.jsx
├── THEMES (6 theme objects with colors, fonts, decorations)
├── DEFAULTS (default data for each block type)
├── BLOCK_TYPES (metadata: type, label, icon, group)
├── GROUPS (4 groups: Header, Daily, Trackers, Knowledge)
├── Helpers (uid generator, newBlock factory)
├── Block Renderers (15 components: TitleBlock, DateBlock, PlanBlock, ...)
├── Block Editors (15 editor forms: TitleEditor, DateEditor, PlanEditor, ...)
├── ThermalPaper component (paper shell + zigzag edges + grain)
├── Export helpers (PNG via SVG foreignObject, JSON save/load)
├── Main App component (AlmanachStudio)
│   ├── State management (useState)
│   ├── Event handlers (add/delete/move/update/duplicate/export)
│   ├── Top bar (brand + actions)
│   ├── Left rail (block library)
│   ├── Canvas (thermal paper preview)
│   └── Right rail (inspector + theme picker + width slider)
└── CSS (full stylesheet in a <style> tag injected by React)
```

### 2.2 Dependencies

The JSX file imports from these packages:

```javascript
import React, { useState, useEffect, useRef, useMemo, useCallback } from "react";
import {
  Plus, Trash2, ChevronUp, ChevronDown, Printer, Download,
  Type, Calendar, ListTodo, Newspaper, CloudSun, Quote as QuoteIcon,
  BookOpen, Clock, Sparkles, Brain, Smile, Minus, X,
  GripVertical, Pencil, Copy, Eye, FileText, Star, Layers,
  Sun, Moon, Leaf, Mountain, Rocket, BookMarked, Coffee
} from "lucide-react";
```

That's it — **React** and **Lucide React icons**. No router, no state library, no CSS framework.

### 2.3 Export Convention

The component uses ES module default export:

```javascript
export default function AlmanachStudio() { ... }
```

This is the standard pattern that JSX hosting systems (like `serve-claude-experiments`) recognize and auto-mount into `<div id="root">`.

### 2.4 Data Model

Each block in the layout is an object with this shape:

```javascript
{
  id: "a1b2c3d",       // random UID
  type: "plan",         // one of 15 block types
  data: {               // type-specific data
    label: "Today's Plan",
    items: [
      { time: "08:30", text: "Morning routine", done: true },
      ...
    ]
  }
}
```

The layout state is an array of these blocks:

```javascript
const [blocks, setBlocks] = useState(STARTER_BLOCKS);
```

### 2.5 Styling Approach

All styling is **inline CSS-in-JS** via React `style` props on every element. The main app also injects a `<style>` tag with global CSS (for the editor chrome, layout grid, print rules, etc.).

This means: **no external CSS files are required for the component itself.** The Google Fonts are loaded via `@import url(...)` in the injected `<style>` tag.

---

## 3. The Target Platform: AtomS3R (ESP32-S3)

### 3.1 Hardware Overview

The **M5Stack AtomS3R** is a tiny ESP32-S3 development board with:

- **ESP32-S3 dual-core processor** (240 MHz, 320 KB SRAM, 8 MB PSRAM)
- **Wi-Fi 4** (802.11 b/g/n, 2.4 GHz)
- **USB-C** for power and programming
- **0.85" IPS LCD** (128×128)
- **IR transmitter**
- **Button**
- **Grove port** (for I2C/UART/GPIO)

### 3.2 Why Host Almanach Studio on the AtomS3R?

The AtomS3R can run a Wi-Fi access point (SoftAP mode) or connect to an existing Wi-Fi network (STA mode). It runs `esp_http_server` — a lightweight HTTP server built into ESP-IDF. This means any device on the same network can open a browser and interact with the Almanach Studio SPA.

The use case: you carry the tiny AtomS3R in your bag, it creates a Wi-Fi hotspot, you connect your phone or laptop to it, open `http://192.168.4.1/almanach`, design your daily almanac page, and print it to a connected thermal printer (or export PNG for later printing).

### 3.3 Memory Constraints

| Resource | Capacity | Notes |
|----------|----------|-------|
| Flash | 8–16 MB | Firmware binary + embedded assets |
| SRAM | 320 KB | Runtime heap |
| PSRAM | 8 MB | Available for large buffers |
| HTTP server | ~40 KB RAM | Per-connection overhead |

The **key constraint** is flash storage for the embedded assets. The full Almanach Studio SPA (React + Babel + fonts + icons + JSX source) needs to fit within the firmware partition alongside the application code.

### 3.4 Partitions

A typical AtomS3R partition table includes:

```
# Name,    Type, SubType,  Offset,   Size
nvs,       data, nvs,      0x9000,   0x4000
phy_init,  data, phy,      0xd000,   0x1000
factory,   app,  factory,  0x10000,  3M
storage,   data, fat,      0x310000, 1M
```

With a 3 MB factory partition, we need the entire firmware + embedded SPA to fit within ~3 MB. This is feasible if we:
- **Precompile JSX to plain JavaScript** (eliminates Babel standalone at ~3 MB)
- **Minify the JavaScript** (typical savings: 40-60%)
- **Use embedded web fonts** only if needed, or rely on system fonts

---

## 4. Strategy: Babel in the Browser vs. Precompiled

### 4.1 The Two Approaches

We have two strategies for running JSX in the browser, with a hybrid option:

#### Strategy A: Babel in the Browser (Runtime Transform)

```
Browser loads:  index.html
                → react.js (CDN)
                → babel-standalone.js (~3 MB from CDN)
                → almanach-studio.jsx (served as text/babel)
                
Babel transforms JSX → JS in the browser, then React renders.
```

**Pros:**
- Zero build step — just serve the raw `.jsx` file
- Fast iteration — edit JSX, save, reload
- Exact same workflow as `serve-claude-experiments`

**Cons:**
- Babel standalone is ~3 MB — too large to embed in ESP32 flash
- Requires CDN internet access (or we embed the 3 MB Babel)
- Page load is slow (~5-10 seconds for first transform)

#### Strategy B: Precompiled Static Bundle (Build-Time Transform)

```
Build machine:  JSX → Babel/esbuild → minified JS bundle (~150 KB)
                Inline CSS, embed fonts as base64
                
ESP32 serves:   index.html (~2 KB)
                almanach-bundle.js (~150 KB minified)
                
No Babel needed in browser. No CDN needed.
```

**Pros:**
- Tiny footprint — fits easily in ESP32 flash
- Fast page load — no runtime transform
- Works offline (no CDN dependency)
- Can tree-shake unused Lucide icons

**Cons:**
- Requires a build step on the development machine
- Changing the JSX means rebuilding and reflashing firmware

#### Strategy C: Hybrid (Recommended)

```
Build machine:  JSX → esbuild → precompiled JS bundle (~150 KB)
                Generate index.html with embedded CSS

ESP32 serves:   /almanach         → index.html (2 KB)
                /almanach/bundle.js → bundle.js (150 KB)
                
Optional:       /almanach/src.jsx → raw JSX (for dev mode with Babel from CDN)
```

**This is the recommended approach.** We precompile the JSX into a static JavaScript bundle using **esbuild** (fast, zero-config JSX transform), embed it in the firmware, and serve it from `/almanach`. No Babel needed at runtime. No CDN dependency.

### 4.2 Why esbuild?

esbuild is the ideal tool for this job:

- **Built-in JSX transform** — no Babel plugin needed
- **Tree-shaking** — only includes the Lucide icons actually used
- **Bundling** — React + Lucide + the component all become one file
- **Minification** — whitespace, dead code, and variable name compression
- **Speed** — builds in ~50ms
- **No config** — single command line

### 4.3 Build Command (Pseudocode)

```bash
# Install dependencies
npm init -y
npm install react react-dom lucide-react
npm install -D esbuild

# Build the bundle
npx esbuild almanach-studio.jsx \
  --bundle \
  --minify \
  --jsx=automatic \
  --external:react \
  --external:react-dom \
  --outfile=dist/almanach-bundle.js \
  --format=esm

# Or: bundle React INTO the output (no CDN needed)
npx esbuild almanach-studio.jsx \
  --bundle \
  --minify \
  --jsx=automatic \
  --outfile=dist/almanach-bundle.js \
  --format=iife \
  --global-name=AlmanachStudio
```

The choice between **external React** (loaded from CDN or embedded separately) vs. **bundled React** (everything in one file) depends on whether other SPAs on the same device share React.

For the AtomS3R where Almanach Studio is the only React app, **bundle React in** — it simplifies the embedding and eliminates CDN dependency entirely.

### 4.4 Estimated Asset Sizes

| Asset | Raw | Minified | Gzipped |
|-------|-----|----------|---------|
| React 18 + react-dom | ~130 KB | ~45 KB | ~15 KB |
| Lucide icons (30 icons used) | ~15 KB | ~8 KB | ~3 KB |
| almanach-studio.jsx | ~70 KB | ~25 KB | ~8 KB |
| Google Fonts (subset) | ~200 KB | N/A | ~60 KB |
| **Total (no fonts)** | **~215 KB** | **~78 KB** | **~26 KB** |
| **Total (with fonts)** | **~415 KB** | **~278 KB** | **~86 KB** |

Even the "with fonts" total of ~278 KB minified fits comfortably in a 3 MB firmware partition alongside the ESP32 application (~500 KB).

---

## 5. The ESP32 HTTP Server Architecture

### 5.1 esp_http_server Overview

ESP-IDF provides `esp_http_server` — a lightweight HTTP/1.1 server optimized for embedded use. Key characteristics:

- **Event-driven** — runs in a FreeRTOS task
- **URI handler registration** — you register callbacks for specific routes
- **Wildcard URI matching** — supports `/*` patterns
- **Chunked responses** — can send large responses in chunks
- **WebSocket support** — bidirectional real-time communication
- **Minimal RAM footprint** — ~40 KB per connection

### 5.2 How Static Assets Get Embedded

ESP-IDF's CMake system supports `EMBED_TXTFILES` and `EMBED_BINARIES` directives in `idf_component_register()`. These directives take files from the source tree and embed them into the firmware binary as byte arrays accessible via C symbols.

From `0017-atoms3r-web-ui/main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS
        "hello_world_main.cpp"
        "http_server.cpp"
        # ... other source files
    EMBED_TXTFILES
        "assets/index.html"
        "assets/assets/app.js"
        "assets/assets/app.css"
    # ...
)
```

This creates C symbols like:

```c
extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[]   asm("_binary_index_html_end");
```

The naming convention is `_binary_{filename_without_extension}_{start|end}` where the filename has dots and slashes replaced with underscores.

### 5.3 The httpd_assets_embed Helper

The reusable component `httpd_assets_embed` provides a thin wrapper around the "send embedded byte range as HTTP response" pattern:

```c
// Send an embedded asset as an HTTP response
esp_err_t httpd_assets_embed_send(
    httpd_req_t *req,
    const uint8_t *start,       // pointer to embedded data start
    const uint8_t *end,         // pointer to embedded data end
    const char *content_type,   // MIME type (e.g. "text/html")
    const char *cache_control,  // Cache-Control header (or NULL)
    bool trim_trailing_nul      // remove trailing NUL byte from data
);
```

This helper:
1. Sets the `Content-Type` header
2. Optionally sets `Cache-Control`
3. Calculates the length from `end - start`
4. Optionally trims the trailing NUL that `EMBED_TXTFILES` adds
5. Sends the data via `httpd_resp_send()`

### 5.4 URI Handler Registration

Each route is registered as a `httpd_uri_t` struct:

```c
httpd_uri_t root = {
    .uri       = "/",               // URI pattern
    .method    = HTTP_GET,          // HTTP method
    .handler   = root_get,          // callback function
    .user_ctx  = nullptr,           // optional user data
};
httpd_register_uri_handler(s_server, &root);
```

For wildcard routes (like `/almanach/*`):

```c
httpd_uri_t almanach_asset = {
    .uri       = "/almanach/*",
    .method    = HTTP_GET,
    .handler   = almanach_asset_get,
    .user_ctx  = nullptr,
};
httpd_register_uri_handler(s_server, &almanach_asset);
```

The `httpd_uri_match_wildcard` function must be set in the server config:

```c
httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
cfg.uri_match_fn = httpd_uri_match_wildcard;
```

---

## 6. The serve-claude-experiments Reference

### 6.1 How It Serves JSX

The `serve-claude-experiments` Go project (at `/home/manuel/code/wesen/2026-03-29--serve-claude-experiments/`) is the primary reference for JSX hosting. Its approach:

1. **HTML artifacts** (`.html`): Served directly, with a navigation bar injected before `</body>`.
2. **JSX artifacts** (`.jsx`): Served via a host HTML page (`jsx-host.html`) that:
   - Loads React 18 from esm.sh CDN via `<script type="importmap">`
   - Optionally loads Babel standalone from unpkg CDN
   - Creates `<div id="root">`
   - Loads the JSX as either:
     - **Precompiled JS** (`<script type="module">`) if a compiled bundle exists
     - **Raw JSX** (`<script type="text/babel" data-type="module">`) as a fallback
   - The JSX file is transformed by `jsx.BuildModuleSource()` which:
     - Rewrites `export default function Name()` to plain `function Name()` + `const __artifactDefault = Name`
     - Appends auto-mount code: `createRoot(document.getElementById("root")).render(...)`

### 6.2 JSX Host Template

The host HTML page (`jsx-host.html`) is the critical piece:

```html
<!DOCTYPE html>
<html>
<head>
  <script type="importmap">
  {
    "imports": {
      "react": "https://esm.sh/react@18.3.1",
      "react-dom": "https://esm.sh/react-dom@18.3.1",
      "react-dom/client": "https://esm.sh/react-dom@18.3.1/client",
      "react/jsx-runtime": "https://esm.sh/react@18.3.1/jsx-runtime"
    }
  }
  </script>
  <!-- If using runtime Babel: -->
  <script src="https://unpkg.com/@babel/standalone/babel.min.js"></script>
</head>
<body>
  <div id="root"></div>
  <script type="text/babel" data-type="module" src="/jsx/artifact-name"></script>
</body>
</html>
```

### 6.3 What We Adapt

For the AtomS3R, we adapt this pattern:

- **Replace CDN URLs with embedded asset routes**: Instead of `https://esm.sh/react@18.3.1`, use `/almanach/react.js`
- **Eliminate Babel**: Use precompiled JS bundle
- **Simplify the mount code**: Since we control the entire bundle, we can have the entry point auto-mount

---

## 7. Implementation Plan

### 7.1 Phase 1: Build the Static Bundle

This is done on the **development machine**, not on the ESP32.

#### Step 1.1: Create the build directory

```
firmware/
  web/
    almanach/
      package.json
      esbuild.mjs            # Build script
      src/
        index.jsx            # Entry point (wraps AlmanachStudio)
        almanach-studio.jsx  # The component (copied from source)
      dist/                  # Build output
        index.html
        almanach-bundle.js
```

#### Step 1.2: Create package.json

```json
{
  "name": "almanach-studio-build",
  "private": true,
  "scripts": {
    "build": "node esbuild.mjs"
  },
  "dependencies": {
    "react": "^18.3.1",
    "react-dom": "^18.3.1",
    "lucide-react": "^0.400.0"
  },
  "devDependencies": {
    "esbuild": "^0.21.0"
  }
}
```

#### Step 1.3: Create entry point (src/index.jsx)

```jsx
import React from "react";
import { createRoot } from "react-dom/client";
import AlmanachStudio from "./almanach-studio";

const root = createRoot(document.getElementById("root"));
root.render(React.createElement(AlmanachStudio));
```

This is needed because the original component uses `export default` — we need a separate entry point that imports and mounts it.

#### Step 1.4: Create the esbuild build script (esbuild.mjs)

```javascript
import * as esbuild from "esbuild";

await esbuild.build({
  entryPoints: ["src/index.jsx"],
  bundle: true,
  minify: true,
  format: "iife",
  globalName: "AlmanachStudio",
  outfile: "dist/almanach-bundle.js",
  target: ["es2020"],
  define: {
    "process.env.NODE_ENV": '"production"',
  },
  logLevel: "info",
});

console.log("✓ Built dist/almanach-bundle.js");
```

The `format: "iife"` creates a self-executing bundle that doesn't require ES module support. The `globalName: "AlmanachStudio"` is optional but useful for debugging.

#### Step 1.5: Create the host HTML page (dist/index.html)

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Almanach Studio</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body, #root { width: 100%; height: 100%; }
    body { background: #1a1612; }
  </style>
</head>
<body>
  <div id="root"></div>
  <script src="almanach-bundle.js"></script>
</body>
</html>
```

#### Step 1.6: Run the build

```bash
cd web/almanach
npm install
npm run build
ls -la dist/
# Should show:
#   index.html        ~0.5 KB
#   almanach-bundle.js ~150 KB (minified)
```

### 7.2 Phase 2: Embed Assets into ESP32 Firmware

#### Step 2.1: Copy build outputs to the firmware assets directory

```bash
mkdir -p firmware/main/assets/almanach
cp web/almanach/dist/index.html firmware/main/assets/almanach/
cp web/almanach/dist/almanach-bundle.js firmware/main/assets/almanach/
```

#### Step 2.2: Update CMakeLists.txt to embed the assets

In `firmware/main/CMakeLists.txt`, add the Almanach files to `EMBED_TXTFILES`:

```cmake
idf_component_register(
    SRCS
        "hello_world_main.cpp"
        "http_server.cpp"
        # ... existing sources
    EMBED_TXTFILES
        "assets/index.html"
        "assets/assets/app.js"
        "assets/assets/app.css"
        # Almanach Studio assets
        "assets/almanach/index.html"
        "assets/almanach/almanach-bundle.js"
    # ...
)
```

This creates the C symbols:

```c
extern const uint8_t assets_almanach_index_html_start[];
extern const uint8_t assets_almanach_index_html_end[];
extern const uint8_t assets_almanach_almanach_bundle_js_start[];
extern const uint8_t assets_almanach_almanach_bundle_js_end[];
```

> **Note**: The symbol names are derived from the file path with dots and slashes replaced by underscores. Verify the exact names by checking the build output or using `riscv32-esp-elf-objdump -t` on the ELF binary.

### 7.3 Phase 3: Add URI Handlers for /almanach

#### Step 3.1: Declare the embedded asset symbols

At the top of `http_server.cpp`:

```cpp
// Almanach Studio embedded assets
extern const uint8_t assets_almanach_index_html_start[] asm("_binary_almanach_index_html_start");
extern const uint8_t assets_almanach_index_html_end[]   asm("_binary_almanach_index_html_end");
extern const uint8_t assets_almanach_almanach_bundle_js_start[] asm("_binary_almanach_bundle_js_start");
extern const uint8_t assets_almanach_almanach_bundle_js_end[]   asm("_binary_almanach_bundle_js_end");
```

> **Important**: The exact `asm()` names depend on the file path in `EMBED_TXTFILES`. ESP-IDF replaces `/` and `.` with `_` and strips leading directory components. Check the generated symbol names after the first build.

#### Step 3.2: Create handler functions

```cpp
// GET /almanach — serve the Almanach Studio index page
static esp_err_t almanach_root_get(httpd_req_t *req) {
    return httpd_assets_embed_send(req,
        assets_almanach_index_html_start,
        assets_almanach_index_html_end,
        "text/html; charset=utf-8",
        "public, max-age=3600",  // Cache for 1 hour
        true);
}

// GET /almanach/bundle.js — serve the precompiled JS bundle
static esp_err_t almanach_bundle_js_get(httpd_req_t *req) {
    return httpd_assets_embed_send(req,
        assets_almanach_almanach_bundle_js_start,
        assets_almanach_almanach_bundle_js_end,
        "application/javascript; charset=utf-8",
        "public, max-age=3600",
        true);
}
```

#### Step 3.3: Register the URI handlers

In the `http_server_start()` function, after the existing registrations:

```cpp
// Almanach Studio routes
httpd_uri_t almanach_root = {
    .uri       = "/almanach",
    .method    = HTTP_GET,
    .handler   = almanach_root_get,
    .user_ctx  = nullptr,
};
httpd_register_uri_handler(s_server, &almanach_root);

httpd_uri_t almanach_bundle = {
    .uri       = "/almanach/bundle.js",
    .method    = HTTP_GET,
    .handler   = almanach_bundle_js_get,
    .user_ctx  = nullptr,
};
httpd_register_uri_handler(s_server, &almanach_bundle);
```

### 7.4 Phase 4: Handle Google Fonts

The Almanach Studio loads 5 Google Fonts via `@import url(...)`. On the AtomS3R (which likely won't have internet access when serving in SoftAP mode), these fonts won't load.

**Options:**

1. **Embed the fonts as base64 in the CSS** — adds ~200 KB to the bundle
2. **Serve font files from the ESP32** — additional embedded assets
3. **Use system fonts** — fall back gracefully with a CSS change
4. **Subset and embed only the glyphs needed** — smallest but requires subsetting tools

**Recommended approach for MVP**: Modify the Almanach Studio component to remove the `@import url(...)` font loading and use a system font stack instead. Add a comment indicating where to re-enable Google Fonts when internet access is available.

In the component's `<style>` tag, replace:

```css
@import url('https://fonts.googleapis.com/css2?family=Cormorant+Garamond:...');
```

With:

```css
/* Fonts: use system stack when offline (AtomS3R local mode) */
/* To restore Google Fonts, replace the font-family stacks below
   and add the @import url(...) at the top of this <style> block. */
```

And update the theme objects to use system font stacks:

```javascript
// In THEMES.classic:
fontDisplay: "'Palatino Linotype', 'Book Antiqua', Palatino, serif",
fontBody: "'Georgia', 'Times New Roman', serif",
```

For a **Phase 2** improvement, embed the actual font files as WOFF2 and serve them from `/almanach/fonts/...`.

### 7.5 Phase 5: Build, Flash, and Test

```bash
# 1. Build the web assets
cd web/almanach && npm run build && cd ../..

# 2. Copy to firmware assets
cp web/almanach/dist/* main/assets/almanach/

# 3. Build the firmware
idf.py build

# 4. Flash to AtomS3R
idf.py flash

# 5. Monitor console
idf.py monitor

# 6. Test in browser
# Connect to the AtomS3R's Wi-Fi network
# Open http://192.168.4.1/almanach (SoftAP mode)
# or http://<device-ip>/almanach (STA mode)
```

---

## 8. Complete File Map

Here is every file that needs to be created or modified:

### 8.1 New Files (Web Build)

```
web/almanach/
├── package.json              # npm dependencies (react, react-dom, lucide-react, esbuild)
├── esbuild.mjs               # Build script
├── src/
│   ├── index.jsx             # Entry point — mounts AlmanachStudio into #root
│   └── almanach-studio.jsx   # The component (copied from source)
└── dist/                     # Build output (gitignored)
    ├── index.html            # Host page
    └── almanach-bundle.js    # Precompiled, minified JS bundle
```

### 8.2 New Files (Firmware Assets)

```
main/assets/
├── almanach/
│   ├── index.html            # Copied from web/almanach/dist/
│   └── almanach-bundle.js    # Copied from web/almanach/dist/
├── index.html                # (existing — the main web UI)
└── assets/
    ├── app.js                # (existing)
    └── app.css               # (existing)
```

### 8.3 Modified Files

| File | Change |
|------|--------|
| `main/CMakeLists.txt` | Add `assets/almanach/index.html` and `assets/almanach/almanach-bundle.js` to `EMBED_TXTFILES` |
| `main/http_server.cpp` | Add `almanach_root_get`, `almanach_bundle_js_get` handlers and register `/almanach` + `/almanach/bundle.js` URIs |

---

## 9. API Reference

### 9.1 ESP-IDF Functions Used

| Function | Header | Purpose |
|----------|--------|---------|
| `httpd_start()` | `esp_http_server.h` | Start the HTTP server |
| `httpd_register_uri_handler()` | `esp_http_server.h` | Register a URI handler |
| `httpd_resp_send()` | `esp_http_server.h` | Send complete response |
| `httpd_resp_set_type()` | `esp_http_server.h` | Set Content-Type header |
| `httpd_resp_set_hdr()` | `esp_http_server.h` | Set a custom header |
| `httpd_resp_send_err()` | `esp_http_server.h` | Send error response |
| `httpd_assets_embed_send()` | `httpd_assets_embed.h` | Send embedded asset as response |
| `httpd_assets_embed_len()` | `httpd_assets_embed.h` | Calculate embedded asset length |

### 9.2 ESP-IDF CMake Directives

| Directive | Purpose |
|-----------|---------|
| `EMBED_TXTFILES "path"` | Embed text files as NUL-terminated byte arrays |
| `EMBED_BINARIES "path"` | Embed binary files as raw byte arrays |

### 9.3 esbuild CLI Options

| Option | Purpose |
|--------|---------|
| `--bundle` | Bundle all imports into a single file |
| `--minify` | Minify the output |
| `--jsx=automatic` | Use React 17+ automatic JSX runtime |
| `--format=iife` | Output as immediately-invoked function expression |
| `--global-name=X` | Name of the global variable for IIFE format |
| `--target=es2020` | Target JavaScript version |
| `--define K=V` | Replace identifiers at build time |
| `--outfile=path` | Output file path |

---

## 10. Detailed Implementation Pseudocode

### 10.1 Build Pipeline (esbuild.mjs)

```javascript
import * as esbuild from "esbuild";
import { readFileSync, writeFileSync } from "fs";

// 1. Bundle and minify the JSX
await esbuild.build({
  entryPoints: ["src/index.jsx"],
  bundle: true,
  minify: true,
  format: "iife",
  outfile: "dist/almanach-bundle.js",
  target: ["es2020"],
  define: {
    "process.env.NODE_ENV": '"production"',
  },
});

// 2. Generate the host HTML page
const html = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Almanach Studio</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    html, body, #root { width: 100%; height: 100%; }
    body { background: #1a1612; }
  </style>
</head>
<body>
  <div id="root"></div>
  <script src="almanach-bundle.js"></script>
</body>
</html>`;

writeFileSync("dist/index.html", html);

console.log("✓ Built dist/almanach-bundle.js");
console.log("✓ Built dist/index.html");
```

### 10.2 Entry Point (src/index.jsx)

```jsx
import React from "react";
import { createRoot } from "react-dom/client";
import AlmanachStudio from "./almanach-studio";

// Mount the Almanach Studio component into the page
const rootElement = document.getElementById("root");
if (rootElement) {
  const root = createRoot(rootElement);
  root.render(React.createElement(AlmanachStudio));
} else {
  console.error("Almanach Studio: #root element not found");
}
```

### 10.3 HTTP Handler Registration (C++ pseudocode)

```cpp
// In http_server.cpp, within http_server_start():

// --- Almanach Studio routes ---

// Handler: GET /almanach → index.html
static esp_err_t almanach_root_get(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=3600");
    return httpd_assets_embed_send(req,
        _binary_almanach_index_html_start,
        _binary_almanach_index_html_end,
        "text/html; charset=utf-8",
        NULL,   // already set Cache-Control above
        true);  // trim trailing NUL
}

// Handler: GET /almanach/bundle.js → precompiled JS
static esp_err_t almanach_bundle_get(httpd_req_t *req) {
    return httpd_assets_embed_send(req,
        _binary_almanach_bundle_js_start,
        _binary_almanach_bundle_js_end,
        "application/javascript; charset=utf-8",
        "public, max-age=86400",  // Cache JS for 24 hours
        true);
}

// Register routes
httpd_uri_t almanach_root_uri = { .uri="/almanach", .method=HTTP_GET, .handler=almanach_root_get };
httpd_register_uri_handler(s_server, &almanach_root_uri);

httpd_uri_t almanach_bundle_uri = { .uri="/almanach/bundle.js", .method=HTTP_GET, .handler=almanach_bundle_get };
httpd_register_uri_handler(s_server, &almanach_bundle_uri);
```

### 10.4 Font Handling Strategy (CSS modification)

In `almanach-studio.jsx`, the `<style>` block at the bottom of the component contains:

```javascript
@import url('https://fonts.googleapis.com/css2?family=Cormorant+Garamond:...&display=swap');
```

For the embedded/offline version, modify this to:

```javascript
/* Offline mode: system font fallbacks */
/* Uncomment the next line when internet access is available:
@import url('https://fonts.googleapis.com/css2?family=Cormorant+Garamond:wght@400;500;600;700&family=EB+Garamond:ital,wght@0,400;0,500;0,600;1,400;1,500&family=Caveat:wght@400;500;700&family=Kalam:wght@300;400;700&family=DM+Sans:wght@400;500;600;700&display=swap');
*/
```

And update each theme to use fallback system fonts:

```javascript
// In each theme object, change fontDisplay and fontBody:
classic: {
  fontDisplay: "'Palatino Linotype', 'Book Antiqua', 'Georgia', serif",
  fontBody: "'Georgia', 'Cambria', 'Times New Roman', serif",
  // ...
},
notebook: {
  fontDisplay: "'Comic Sans MS', 'Chalkboard SE', cursive",
  fontBody: "'Comic Sans MS', 'Chalkboard SE', cursive",
  // ...
},
```

---

## 11. Testing and Validation

### 11.1 Build Validation

```bash
# 1. Verify the web bundle was built
ls -la web/almanach/dist/
# Expected: index.html (~0.5 KB), almanach-bundle.js (~100-200 KB)

# 2. Verify the JS bundle is valid
node -e "require('./web/almanach/dist/almanach-bundle.js')"
# Should not throw syntax errors

# 3. Check the firmware binary size
idf.py size
# The "Total flash used" should be well under 3 MB
```

### 11.2 Runtime Validation

```bash
# 1. Flash and monitor
idf.py flash monitor

# 2. Check console for HTTP server startup
# Expected: "starting http server on port 80"

# 3. From a browser on the same network:
#    Open http://192.168.4.1/almanach (SoftAP) or http://<sta-ip>/almanach

# 4. Verify:
#    - Page loads with the Almanach Studio editor
#    - Thermal paper preview renders
#    - Block library appears in left panel
#    - Can add, edit, move, and delete blocks
#    - Theme switcher works
#    - PNG export works (download)
#    - JSON save/load works (download/upload)
#    - Print mode works (Ctrl+P)
```

### 11.3 Performance Expectations

| Metric | Expected Value |
|--------|---------------|
| First page load (no cache) | < 2 seconds |
| Subsequent loads (cached) | < 0.5 seconds |
| Memory usage (ESP32 heap) | ~50 KB for HTTP serving |
| Flash usage (embedded assets) | ~150-200 KB |

---

## 12. Future Improvements

### 12.1 Embed Google Fonts

Download the WOFF2 files for the 5 font families, subset them to only the glyphs used in the Almanach Studio (ASCII + common symbols), and embed them in the firmware. This would restore the beautiful typography for offline use.

Estimated additional flash: ~100-200 KB (subsets only).

### 12.2 Thermal Printer Integration

If the AtomS3R is connected to a thermal printer via UART/GPIO:

1. Add a "Print" button that sends the PNG data to the printer
2. Use the ESP32's PNG encoding capabilities
3. Convert the rendered output to ESC/POS commands
4. Stream the print data over the serial connection

### 12.3 Data API

Add ESP32 endpoints that serve real data:

| Endpoint | Data Source |
|----------|------------|
| `GET /api/weather` | OpenWeatherMap (needs internet) |
| `GET /api/news` | RSS feed aggregator (needs internet) |
| `GET /api/quote` | Embedded daily quote database |
| `GET /api/word` | Embedded word-of-the-day database |
| `GET /api/date` | ESP32 RTC (NTP-synced) |

The Almanach Studio could fetch from these APIs instead of using hardcoded demo data.

### 12.4 Layout Persistence

Store saved layouts on the ESP32's FAT filesystem (the `storage` partition). Add endpoints:

```
GET  /api/layouts        → list saved layouts
GET  /api/layouts/{name} → load a layout
PUT  /api/layouts/{name} → save a layout
DEL  /api/layouts/{name} → delete a layout
```

### 12.5 PWA (Progressive Web App)

Add a service worker and manifest to make Almanach Studio installable as a PWA on mobile devices. This would enable:

- Offline use after first load
- Home screen icon
- Full-screen mode (no browser chrome)

---

## 13. Glossary

| Term | Definition |
|------|-----------|
| **ESP32-S3** | Espressif's dual-core microcontroller with Wi-Fi and USB support |
| **AtomS3R** | M5Stack's tiny ESP32-S3 development board |
| **esp_http_server** | ESP-IDF's built-in HTTP server library |
| **EMBED_TXTFILES** | CMake directive that embeds text files into the firmware binary |
| **esbuild** | A fast JavaScript/TypeScript bundler written in Go |
| **IIFE** | Immediately Invoked Function Expression — a JavaScript pattern for self-executing code |
| **JSX** | JavaScript XML — a syntax extension for React that looks like HTML |
| **Babel** | A JavaScript compiler that transforms JSX into plain JS |
| **SPA** | Single Page Application — a web app that loads a single HTML page and dynamically updates it |
| **SoftAP** | Software Access Point — the ESP32 acts as a Wi-Fi hotspot |
| **STA** | Station mode — the ESP32 connects to an existing Wi-Fi network |
| **FAT filesystem** | A simple file system supported by ESP-IDF, typically on a flash partition |
| **ESC/POS** | The command protocol used by thermal printers |
| **Lucide** | A library of open-source icons, available as React components |

---

## 14. Key Decisions and Rationale

| Decision | Rationale |
|----------|-----------|
| Use esbuild instead of Babel/Vite/Webpack | Fastest build, simplest config, built-in JSX transform |
| Bundle React into the JS file | No CDN dependency, works fully offline |
| Use IIFE format instead of ESM | Simplest loading via `<script src="...">`, no module server needed |
| Embed assets via EMBED_TXTFILES | Proven pattern from firmware 0017, no FAT filesystem complexity |
| System font fallbacks for MVP | Avoids embedding 200+ KB of font files in initial version |
| Separate /almanach route namespace | Clean separation from existing web UI, easy to add/remove |
| Cache headers on static assets | ESP32 bandwidth is limited — browser caching reduces re-fetches |
| Single JS bundle (no code splitting) | Fewer HTTP requests on a slow ESP32 connection |

---

## 15. Related Files and References

### Source Files

| File | Role |
|------|------|
| `sources/local/almanach-studio(1).jsx` | Original Almanach Studio JSX component |
| `/home/manuel/code/wesen/2026-03-29--serve-claude-experiments/` | Reference project for JSX hosting |
| `/home/manuel/code/wesen/2026-03-29--serve-claude-experiments/pkg/server/templates/jsx-host.html` | JSX host page template |
| `/home/manuel/code/wesen/2026-03-29--serve-claude-experiments/pkg/jsx/module.go` | JSX source transformation logic |

### ESP32 Reference Files

| File | Role |
|------|------|
| `components/httpd_assets_embed/httpd_assets_embed.c` | Reusable embedded-asset HTTP sender |
| `components/httpd_assets_embed/include/httpd_assets_embed.h` | Header for the helper |
| `0017-atoms3r-web-ui/main/http_server.cpp` | Reference HTTP server with embedded assets |
| `0017-atoms3r-web-ui/main/CMakeLists.txt` | Reference CMake with EMBED_TXTFILES |
| `0017-atoms3r-web-ui/web/vite.config.ts` | Reference Vite config for building web assets |
| `0017-atoms3r-web-ui/web/package.json` | Reference npm dependencies |

### ESP-IDF Documentation

- [esp_http_server API Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/protocols/esp_http_server.html)
- [Build System (CMake)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/build-system.html)
- [Embedded Files](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/build-system.html#embedding-binary-data)
