---
title: Almanach Render Service — Analysis, Design, and Implementation Guide
ticket: ALMANACH-RENDER-SERVICE
doc_type: design-doc
status: active
intent: long-term
topics:
  - go
  - chrome-headless
  - api-server
  - almanach
  - stoms3r
  - rendering
created: 2026-04-29
---

# Almanach Render Service — Analysis, Design, and Implementation Guide

## Executive Summary

The **Almanach Render Service** is a Go HTTP server that runs alongside the ESP32 thermal printer firmware (stoms3r) and provides a REST API for generating almanac page images. It loads the Almanach Studio React SPA in a headless Chrome browser, injects data (weather, news, quotes, habits), takes a screenshot of the rendered thermal paper, converts it to a 1-bit monochrome bitmap, and either returns the bitmap to the caller or forwards it directly to the ESP32's print API.

The service solves a fundamental limitation: the ESP32-S3 has no internet access and cannot fetch live data. By moving the rendering to a companion Go server with a real browser, we can produce daily almanac pages filled with real-time information and send them to the printer on a schedule — turning the thermal printer into an automatic daily digest machine.

This document explains every part of the system in detail, written for a developer who is new to the project. It covers the existing codebase, the new Go server architecture, the Chrome DevTools Protocol, the bitmap conversion pipeline, and step-by-step implementation instructions with pseudocode.

---

## Table of Contents

1. [Problem Statement](#1-problem-statement)
2. [System Context — What Already Exists](#2-system-context--what-already-exists)
3. [The React SPA — How Almanach Studio Renders a Page](#3-the-react-spa--how-almanach-studio-renders-a-page)
4. [The ESP32 Firmware — How Printing Works](#4-the-esp32-firmware--how-printing-works)
5. [Proposed Architecture](#5-proposed-architecture)
6. [API Design](#6-api-design)
7. [Chrome Headless Integration](#7-chrome-headless-integration)
8. [Bitmap Conversion Pipeline](#8-bitmap-conversion-pipeline)
9. [Go Project Structure](#9-go-project-structure)
10. [Detailed Implementation Plan](#10-detailed-implementation-plan)
11. [Configuration and Deployment](#11-configuration-and-deployment)
12. [Error Handling and Reliability](#12-error-handling-and-reliability)
13. [Testing Strategy](#13-testing-strategy)
14. [Security Considerations](#14-security-considerations)
15. [File Reference Map](#15-file-reference-map)

---

## 1. Problem Statement

### The Current Workflow (Manual)

Right now, printing an almanac page requires a human to:

1. Connect to the ESP32's Wi-Fi network (or be on the same LAN)
2. Open `http://<device-ip>/almanach` in a browser
3. Manually edit the content blocks — type in the weather, the news, the quote, the day's plan
4. Click "Print" to send the rendered page to the thermal printer

This works for a single demo, but it cannot produce a daily almanac automatically. The ESP32 cannot fetch weather data, RSS feeds, or calendar events. The React SPA has no backend — all data is hardcoded or manually entered.

### What We Want (Automated)

A companion service that:

- Runs on a machine with internet access (a Raspberry Pi, a laptop, a cloud VM)
- Fetches real data from APIs (weather, news, quotes, calendar, habits)
- Loads the Almanach Studio SPA with that data injected
- Renders the page to a monochrome bitmap
- Sends the bitmap to the ESP32's `/api/print/bitmap` endpoint
- Can be triggered on a schedule (cron, systemd timer) or on demand via HTTP

The result: every morning at 7 AM, the thermal printer automatically prints a personalized daily almanac.

### Why Go?

- **Single binary deployment** — `go build` produces one static binary. No Node.js runtime, no `node_modules`, no Python virtualenv. Copy it to a Raspberry Pi and it runs.
- **Excellent Chrome DevTools Protocol support** — the `chromedp` package is mature, well-documented, and handles the complex async protocol cleanly.
- **Good HTTP server** — the standard library `net/http` is production-grade. No framework needed.
- **Cross-compilation** — `GOOS=linux GOARCH=arm64 go build` produces a binary for a Raspberry Pi from a laptop in seconds.
- **Low resource usage** — important when running alongside Chrome on a resource-constrained device.

---

## 2. System Context — What Already Exists

Before building the new service, you need to understand every piece of the existing system. This section walks through each component.

### 2.1 The stoms3r Firmware

**Location:** `stoms3r/`

The stoms3r firmware is an ESP-IDF application for the M5Stack AtomS3R (ESP32-S3). It controls a K118 thermal printer over UART. The firmware provides:

- **Wi-Fi connectivity** — connects to an existing network (STA mode) or creates its own hotspot (SoftAP mode). Credentials are persisted in NVS (non-volatile storage).
- **HTTP server** — runs on port 80 using `esp_http_server`. Serves a web UI and exposes REST APIs.
- **Printer driver** — sends ESC/POS commands over UART1 at 9600 baud. Supports text printing, bitmap printing, barcodes, QR codes, density/speed control, and status queries.
- **USB Serial/JTAG console** — provides a REPL (`esp_console`) for direct printer control.

**Key files:**

| File | Purpose |
|------|---------|
| `main/web_server.c` | HTTP server — all URI handlers, request parsing |
| `main/printer_drv.h` | Printer driver API (header) |
| `main/printer_drv.c` | Printer driver implementation |
| `main/index.html` | Printer web UI (text printing, image upload, dithering) |
| `main/CMakeLists.txt` | Build config — embeds HTML/JS assets |
| `main/assets/almanach/almanach.html` | Almanach Studio host page (435 B) |
| `main/assets/almanach/almanach-bundle.js` | Precompiled React SPA (216 KB) |

### 2.2 The ESP32 HTTP API

The firmware exposes these HTTP endpoints:

```
┌──────────────────────────────────┬────────┬──────────────────────────────────────────┐
│ Endpoint                         │ Method │ Purpose                                  │
├──────────────────────────────────┼────────┼──────────────────────────────────────────┤
│ /                                │ GET    │ Printer web UI (index.html)              │
│ /almanach                        │ GET    │ Almanach Studio SPA (host page)          │
│ /almanach/bundle.js              │ GET    │ Almanach Studio React bundle (216 KB)    │
│ /api/status                      │ GET    │ JSON: Wi-Fi + printer state              │
│ /api/print/text                  │ POST   │ Print plain text string                  │
│ /api/print/bitmap                │ POST   │ Print 1-bit monochrome bitmap            │
│ /api/printer/status              │ GET    │ K118 4-byte status packet                │
│ /api/printer/temp                │ GET    │ Printhead temperature (°C)               │
│ /api/printer/baud                │ GET    │ Current printer baud rate                │
│ /api/printer/density             │ POST   │ Set print density (0-39)                 │
│ /api/printer/speed               │ POST   │ Set mechanism speed                      │
│ /api/printer/graphics-mode       │ POST   │ Set graphics mode (30/31/32)             │
└──────────────────────────────────┴────────┴──────────────────────────────────────────┘
```

**The critical endpoint for this project is `POST /api/print/bitmap`.** It accepts:

- **Headers:**
  - `X-Width: <pixels>` — must be divisible by 8
  - `X-Height: <pixels>`
  - `X-Feed: <lines>` — optional, number of lines to feed after printing (default 3)
  - `Content-Type: application/octet-stream`
- **Body:** Raw 1-bit packed bitmap, MSB first. Each byte represents 8 pixels, left-to-right. Each row is `width / 8` bytes. Rows are top-to-bottom.
- **Total body size:** `(width / 8) * height` bytes

**Example:** For a 384×800 pixel image, the body is `48 * 800 = 38,400 bytes`.

### 2.3 The K118 Thermal Printer

The K118 is a 58mm thermal receipt printer with these specifications:

- **Resolution:** 203 DPI (8 dots/mm)
- **Paper width:** 58mm → 384 printable dots
- **Print width:** 48mm (the paper has ~5mm margins)
- **Color:** Monochrome only — black dots on white paper
- **Interface:** UART (9600 baud default, supports up to 115200)
- **Protocol:** ESC/POS-compatible with M5Stack extensions
- **Flow control:** Hardware CTS — the printer can pause the host's UART transmission when its buffer is full or the thermal engine is busy

The printer cannot print gray. Every pixel is either a black dot or no dot. Gray values must be converted to black-and-white using either simple thresholding or error-diffusion dithering (Floyd-Steinberg).

### 2.4 The Almanach Studio React SPA

**Location:** `stoms3r/web/almanach/src/almanach-studio.jsx` (~2200 lines)

A single-file React component that renders a daily almanac page as if it were printed on thermal paper. Key characteristics:

- **15 block types:** title, date, divider, plan, news, weather, note, habits, mood, reading, reflection, quote, word, history, did_you_know
- **6 visual themes:** Classic, Minimal, Botanical, Notebook, Vintage Ledger, Space Age — all forced to monochrome (#000 on #fff) for thermal printing
- **State:** An array of blocks (`{ id, type, data }`), each block has type-specific content (text fields, arrays of items, etc.)
- **Settings:** Theme key, paper width (default 384px), font scale (default 1.6×), feed lines (default 3)
- **Persistence:** `localStorage` for settings, JSON export/import for full layouts
- **No external dependencies at runtime:** All fonts use system font stacks. No Google Fonts, no CDN. The bundle is 216 KB and fully self-contained.

The component has two export mechanisms built in:

1. **PNG export** — renders the paper to a canvas via SVG `foreignObject`, downloads as PNG
2. **Direct print** — renders to canvas, binarizes to 1-bit, POSTs raw bitmap to `/api/print/bitmap`

For the render service, we will use approach similar to the direct print, but triggered from Go via Chrome instead of from the user's browser.

---

## 3. The React SPA — How Almanach Studio Renders a Page

This section explains the React component in enough detail that you can write Go code to inject data into it programmatically.

### 3.1 The Data Model

The component's state is a single array of block objects:

```javascript
// Simplified example of the state
const blocks = [
  { id: "abc123", type: "title", data: { title: "Daily Almanac", subtitle: "April 29, 2026" } },
  { id: "def456", type: "date",  data: { date: "2026-04-29", day: "Tuesday" } },
  { id: "ghi789", type: "weather", data: { temp: "18°C", condition: "Partly cloudy", ... } },
  { id: "jkl012", type: "news",  data: { items: [
    { headline: "Headline 1", source: "BBC", summary: "..." },
    { headline: "Headline 2", source: "Reuters", summary: "..." },
  ]}},
  // ... more blocks
];
```

Each block type has a specific `data` shape. The full list of block types and their data fields:

| Block Type | Data Fields |
|------------|-------------|
| `title` | `title: string`, `subtitle: string` |
| `date` | `date: string`, `day: string` |
| `divider` | *(no data fields — just renders a horizontal line)* |
| `plan` | `label: string`, `items: [{ time, text, done }]` |
| `news` | `items: [{ headline, source, summary }]` |
| `weather` | `temp, high, low, condition, humidity, wind, icon` |
| `note` | `title: string`, `content: string` |
| `habits` | `items: [{ name, done }]` |
| `mood` | `mood: 1-5`, `energy: 1-5`, `note: string` |
| `reading` | `title: string`, `author: string`, `pages: string`, `current: { page, total, progress }` |
| `reflection` | `prompt: string`, `response: string` |
| `quote` | `text: string`, `author: string`, `source: string` |
| `word` | `word: string`, `definition: string`, `partOfSpeech: string`, `example: string` |
| `history` | `year: string`, `event: string` |
| `did_you_know` | `text: string` |

### 3.2 How Settings Work

The component stores settings in state and syncs them to `localStorage`:

```javascript
// State initialization from localStorage (with fallback defaults)
const [themeKey, setThemeKey] = useState(() => {
  try { const v = localStorage.getItem("almanach_themeKey"); return (v && THEMES[v]) ? v : "classic"; } catch { return "classic"; }
});
const [paperWidth, setPaperWidth] = useState(() => {
  try { const v = JSON.parse(localStorage.getItem("almanach_paperWidth")); return (typeof v === "number" && v >= 280 && v <= 600) ? v : 384; } catch { return 384; }
});
const [bodyScale, setBodyScale] = useState(() => { /* ... 1.6 default */ });
const [feedLines, setFeedLines] = useState(() => { /* ... 3 default */ });
```

### 3.3 The JSON Layout Format

The SPA can export/import layouts as JSON. This is the format the Go server will generate:

```json
{
  "almanach_studio_version": 1,
  "exported_at": "2026-04-29T07:00:00.000Z",
  "theme": "minimal",
  "paperWidth": 384,
  "bodyScale": 1.6,
  "feedLines": 3,
  "blocks": [
    { "id": "a1", "type": "title", "data": { "title": "Daily Almanac", "subtitle": "April 29" } },
    { "id": "a2", "type": "weather", "data": { "temp": "18°C", "condition": "Cloudy" } },
    { "id": "a3", "type": "quote", "data": { "text": "The only way...", "author": "Socrates" } }
  ]
}
```

### 3.4 Exposing a JavaScript API for Headless Control

Currently, the SPA has no external JavaScript API. To make it controllable from Chrome DevTools, we need to add global functions that the Go server can call via `chromedp.Evaluate()`.

The minimal API surface we need:

```javascript
// Add these to the AlmanachStudio component, exposed on window:

// 1. Load a complete layout from JSON
window.almanachLoadLayout = function(jsonString) {
  // Parse the JSON, validate blocks, call setBlocks/setThemeKey/etc.
  // This replaces the current handleImportFile logic but takes a string instead of a File
};

// 2. Get the rendered paper as a PNG data URL
window.almanachExportPng = function() {
  // Returns a Promise<string> — a data URL like "data:image/png;base64,..."
  // Uses the existing exportPaperToPng logic but returns the data instead of downloading
};

// 3. Get the rendered paper as a raw 1-bit bitmap (Uint8Array)
window.almanachExportBitmap = function() {
  // Returns a Promise<{width, height, data: Uint8Array}>
  // Uses the same binarization + MSB packing as handlePrint
};

// 4. Signal that the SPA is ready
window.almanachReady = true;
```

These four functions are the entire interface between the Go server and the React SPA. Everything else (rendering, fonts, layout) happens inside the component.

---

## 4. The ESP32 Firmware — How Printing Works

This section explains the complete path from a bitmap POST request to physical dots on paper.

### 4.1 The POST /api/print/bitmap Handler

The handler in `web_server.c` does the following in order:

1. **Parse headers** — extract `X-Width`, `X-Height`, `X-Feed`
2. **Validate** — width must be divisible by 8, content-length must equal `(width/8) * height`
3. **Reset printer** — sends `ESC @` to clear any stale state
4. **Read entire body** — reads the full request body into heap memory (the printer driver needs the complete bitmap before starting the raster command)
5. **Print bitmap** — calls `printer_drv_print_bitmap(width, height, pixels)`
6. **Feed paper** — calls `printer_drv_feed(N)` where N comes from `X-Feed` header (default 3)
7. **Return JSON** — `{ "ok": true }` on success, `{ "error": "message" }` on failure

### 4.2 The Printer Driver Bitmap Path

`printer_drv_print_bitmap()` in `printer_drv.c`:

1. Sends the `GS v 0` raster command header (width, height in ESC/POS format)
2. Sends the entire pixel payload as one continuous UART write
3. Hardware CTS flow control (GPIO6) lets the printer pause transmission when its buffer is full or the thermal line is heating

The `GS v 0` command format:

```
1D 76 30 mode xH xL yH yL d1...dk
```

Where:
- `1D 76 30` = "GS v 0" command bytes
- `mode` = 0 (normal, no scaling)
- `xH xL` = bytes per row (big-endian), e.g., `00 30` for 384px = 48 bytes/row
- `yH yL` = rows total (big-endian), e.g., `03 20` for 800 rows
- `d1...dk` = pixel data, k = bytesPerRow × height

### 4.3 The 1-Bit Bitmap Format

The bitmap is packed MSB-first. This means:

```
Byte layout for one row of 384 pixels (48 bytes):

Byte 0: pixels 0-7    (bit 7 = pixel 0, bit 0 = pixel 7)
Byte 1: pixels 8-15   (bit 7 = pixel 8, bit 0 = pixel 15)
...
Byte 47: pixels 376-383

A bit value of 1 = black dot (heated)
A bit value of 0 = white (no heat)
```

In Go pseudocode, packing a row:

```go
func packRow(pixels []bool, width int) []byte {
    bytesPerRow := width / 8
    row := make([]byte, bytesPerRow)
    for x := 0; x < width; x++ {
        if pixels[x] { // pixel is black
            row[x/8] |= byte(0x80) >> (x % 8)
        }
    }
    return row
}
```

---

## 5. Proposed Architecture

### 5.1 High-Level Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         Go Render Service                               │
│                    (runs on Pi/Laptop/Cloud)                            │
│                                                                         │
│  ┌─────────────┐    ┌──────────────┐    ┌───────────────────┐          │
│  │ Data Fetchers│───>│ Layout Builder│───>│ Chrome Headless   │          │
│  │             │    │              │    │ (chromedp)        │          │
│  │ • Weather   │    │ JSON layout  │    │                   │          │
│  │ • News RSS  │    │ with blocks  │    │ 1. Load /almanach │          │
│  │ • Quotes    │    │              │    │ 2. Inject JSON    │          │
│  │ • Calendar  │    │              │    │ 3. Wait for render│          │
│  │ • Word      │    │              │    │ 4. Screenshot     │          │          │
│  └─────────────┘    └──────────────┘    └───────┬───────────┘          │
│                                                  │                      │
│                                         PNG or raw bitmap               │
│                                                  │                      │
│                                        ┌─────────▼──────────┐          │
│                                        │ Bitmap Converter    │          │
│                                        │ (image → 1-bit BW) │          │
│                                        └─────────┬──────────┘          │
│                                                  │                      │
│                                     Raw 1-bit bitmap bytes              │
│                                                  │                      │
│                                        ┌─────────▼──────────┐          │
│                                        │ Print Forwarder     │          │
│                                        │ POST /api/print/    │          │
│                                        │      bitmap         │          │
│                                        └─────────┬──────────┘          │
└──────────────────────────────────────────────────┼──────────────────────┘
                                                   │ HTTP POST
                                                   │ (X-Width, X-Height,
                                                   │  X-Feed headers)
                                                   ▼
                                    ┌──────────────────────────┐
                                    │  ESP32 (stoms3r)         │
                                    │  http://<device-ip>:80   │
                                    │                          │
                                    │  POST /api/print/bitmap  │
                                    │       → printer_drv      │
                                    │         → K118 printer   │
                                    └──────────────────────────┘
```

### 5.2 Component Responsibilities

| Component | Responsibility | Package/Library |
|-----------|---------------|-----------------|
| **HTTP Server** | Expose REST API, handle requests | `net/http` (stdlib) |
| **Data Fetchers** | Fetch weather, news, quotes, etc. | `net/http` + custom per-source |
| **Layout Builder** | Assemble block data into JSON layout | Pure Go structs + `encoding/json` |
| **Chrome Controller** | Drive headless Chrome to render the page | `github.com/chromedp/chromedp` |
| **Bitmap Converter** | Convert PNG/screenshot to 1-bit packed bitmap | `image` stdlib + custom packing |
| **Print Forwarder** | POST bitmap to ESP32 | `net/http` (stdlib) |
| **Scheduler** | Run on cron schedule | `github.com/robfig/cron/v3` |

### 5.3 Data Flow for a Single Render Request

```
1. HTTP request arrives:  POST /api/render?print=true

2. Data fetchers run concurrently:
   ├── fetchWeather("Berlin")     → { temp: "18°C", condition: "Cloudy", ... }
   ├── fetchNews()                → [{ headline: "...", source: "BBC" }, ...]
   ├── fetchQuote()               → { text: "...", author: "..." }
   ├── fetchWord()                → { word: "serendipity", definition: "..." }
   └── fetchDate()                → { date: "2026-04-29", day: "Tuesday" }

3. Layout builder assembles JSON:
   {
     "theme": "minimal",
     "paperWidth": 384,
     "bodyScale": 1.6,
     "feedLines": 3,
     "blocks": [
       { "type": "title", "data": { "title": "Daily Almanac", "subtitle": "April 29" } },
       { "type": "weather", "data": { ... } },
       { "type": "news", "data": { "items": [ ... ] } },
       { "type": "quote", "data": { ... } }
     ]
   }

4. Chrome headless renders:
   a. Open http://localhost:8199/almanach  (the Go server also serves the SPA static files)
   b. Wait for window.almanachReady === true
   c. Call window.almanachLoadLayout(JSON.stringify(layout))
   d. Wait 500ms for React to re-render
   e. Call window.almanachExportBitmap()
   f. Receive { width, height, data: ArrayBuffer }

5. Bitmap converter (if needed):
   - If the SPA returns a PNG, decode it, binarize at threshold 128, pack MSB-first
   - If the SPA returns raw bitmap bytes, use them directly

6. Print forwarder:
   - POST http://<device-ip>/api/print/bitmap
   - Headers: X-Width: 384, X-Height: <computed>, X-Feed: 3
   - Body: raw bitmap bytes

7. Return JSON response:
   { "ok": true, "width": 384, "height": 800, "printed": true }
```

---

## 6. API Design

### 6.1 Go Server Endpoints

```
┌──────────────────────────┬────────┬──────────────────────────────────────────┐
│ Endpoint                 │ Method │ Purpose                                  │
├──────────────────────────┼────────┼──────────────────────────────────────────┤
│ /api/render              │ POST   │ Render an almanac page                   │
│ /api/render-and-print    │ POST   │ Render + forward to ESP32 printer        │
│ /api/schedule            │ POST   │ Set up a cron schedule for auto-printing │
│ /api/schedule            │ GET    │ Get current schedule                     │
│ /api/schedule            │ DELETE │ Remove the schedule                      │
│ /almanach                │ GET    │ Serve Almanach Studio HTML (for Chrome)  │
│ /almanach/bundle.js      │ GET    │ Serve Almanach Studio JS bundle          │
│ /health                  │ GET    │ Health check                             │
└──────────────────────────┴────────┴──────────────────────────────────────────┘
```

### 6.2 POST /api/render

**Request body** (optional — if omitted, fetches live data automatically):

```json
{
  "theme": "minimal",
  "paperWidth": 384,
  "bodyScale": 1.6,
  "feedLines": 3,
  "printerIp": "192.168.0.126",
  "data": {
    "weather": { "location": "Berlin" },
    "news": { "sources": ["bbc", "reuters"], "maxItems": 5 }
  }
}
```

If `data` is omitted, all fetchers run with defaults. If individual fields are provided inside `data`, those values override the defaults (e.g., set a specific city for weather).

**Response** (with `Accept: application/json`):

```json
{
  "ok": true,
  "width": 384,
  "height": 812,
  "theme": "minimal",
  "renderedAt": "2026-04-29T07:00:05Z",
  "imageUrl": "/api/render/2026-04-29T07:00:05Z.png"
}
```

**Response** (with `Accept: image/png`):

Returns the raw PNG image directly.

**Response** (with `Accept: application/octet-stream`):

Returns the raw 1-bit bitmap with `X-Width` and `X-Height` headers.

### 6.3 POST /api/render-and-print

Same request body as `/api/render`, but also forwards the bitmap to the ESP32:

```json
{
  "ok": true,
  "width": 384,
  "height": 812,
  "printed": true,
  "printerResponse": { "ok": true },
  "renderedAt": "2026-04-29T07:00:05Z"
}
```

### 6.4 POST /api/schedule

```json
{
  "cron": "0 7 * * *",
  "printerIp": "192.168.0.126",
  "theme": "minimal",
  "feedLines": 3,
  "data": {
    "weather": { "location": "Berlin" },
    "news": { "maxItems": 3 }
  }
}
```

**Response:**

```json
{
  "ok": true,
  "schedule": {
    "cron": "0 7 * * *",
    "nextRun": "2026-04-30T07:00:00Z",
    "lastRun": null
  }
}
```

---

## 7. Chrome Headless Integration

This is the most complex part of the system. The Go server must control a real browser to render the React SPA, because React components use the browser's DOM and CSS engine to lay out text, compute flexbox, apply fonts, etc. There is no practical way to render React to a pixel-perfect image without a browser.

### 7.1 What is Chrome Headless?

Chrome Headless is Google Chrome running without a visible window. It loads web pages, executes JavaScript, and renders CSS exactly like a normal browser, but writes its output to files or memory instead of a screen. It's controlled programmatically via the **Chrome DevTools Protocol (CDP)**.

CDP is a WebSocket-based protocol. You connect to Chrome's debugging port, send JSON-RPC commands, and receive events and responses. The protocol supports:

- **Page navigation** — load a URL
- **JavaScript evaluation** — run arbitrary JS in the page context
- **DOM manipulation** — query and modify DOM elements
- **Screenshot capture** — render the page to a PNG
- **Network interception** — mock requests, block URLs

### 7.2 Why chromedp?

`chromedp` is the standard Go package for CDP. It provides:

- **Context-based lifecycle** — Chrome processes are started and stopped via Go contexts
- **Action-based API** — complex CDP sequences are expressed as chains of actions
- **Automatic target management** — handles tabs, frames, iframes
- **Built-in timeout and cancellation** — leverages Go's context cancellation

### 7.3 Chrome Startup Configuration

The Go server starts Chrome with specific flags for headless rendering:

```go
func newChromeAllocator() chromedp.ContextOption {
    opts := append(chromedp.DefaultExecAllocatorOptions[:],
        chromedp.Flag("headless", true),
        chromedp.Flag("disable-gpu", true),
        chromedp.Flag("no-sandbox", true),
        chromedp.Flag("disable-dev-shm-usage", true),
        chromedp.Flag("hide-scrollbars", true),
        chromedp.Flag("disable-extensions", true),
        chromedp.Flag("disable-background-networking", true),
        chromedp.Flag("disable-default-apps", true),
        chromedp.Flag("disable-sync", true),
        chromedp.Flag("metrics-recording-only", true),
        chromedp.Flag("mute-audio", true),
        // Render at 1x scale — we want exact 384px width, no HiDPI doubling
        chromedp.Flag("force-device-scale-factor", 1.0),
        // Window size: 384px wide (printer width), 2000px tall (enough for any page)
        chromedp.WindowSize(384, 2000),
    )
    return chromedp.WithExecAllocator(opts...)
}
```

**Why these flags matter:**

- `force-device-scale-factor: 1.0` — prevents Chrome from doubling the resolution on HiDPI displays. We want exactly 384 pixels wide, not 768.
- `disable-dev-shm-usage` — critical on Linux (especially Docker), where `/dev/shm` may be too small for Chrome's shared memory
- `hide-scrollbars` — prevents scrollbars from appearing in screenshots
- `WindowSize(384, 2000)` — Chrome renders the page at this viewport size. The height is generous to avoid needing to scroll

### 7.4 The Render Sequence

The complete sequence to render an almanac page in Chrome:

```go
func renderAlmanac(ctx context.Context, layoutJSON string) ([]byte, int, int, error) {
    // 1. Navigate to the SPA
    //
    // The Go server serves the SPA's static files itself on port 8199.
    // Chrome loads from localhost, not from the ESP32.
    err := chromedp.Run(ctx,
        chromedp.Navigate("http://localhost:8199/almanach"),
    )
    if err != nil {
        return nil, 0, 0, fmt.Errorf("navigate: %w", err)
    }

    // 2. Wait for the SPA to signal it's ready
    err = chromedp.Run(ctx,
        chromedp.WaitReady("window.almanachReady", chromedp.ByJSPath),
    )
    if err != nil {
        return nil, 0, 0, fmt.Errorf("wait ready: %w", err)
    }

    // 3. Inject the layout data
    //
    // This calls the JavaScript function we added to the SPA,
    // passing the JSON layout as a string argument.
    escaped := strings.ReplaceAll(layoutJSON, "`", "\\`")
    escaped = strings.ReplaceAll(escaped, `\`, `\\`)
    loadJS := fmt.Sprintf(`window.almanachLoadLayout(%s)`, layoutJSON)
    err = chromedp.Run(ctx,
        chromedp.Evaluate(loadJS, nil),
    )
    if err != nil {
        return nil, 0, 0, fmt.Errorf("load layout: %w", err)
    }

    // 4. Wait for React to finish rendering
    //
    // 500ms is generous — React re-renders a ~15-block layout in <50ms.
    // But we also wait for fonts to load, which can take longer on first run.
    time.Sleep(500 * time.Millisecond)

    // 5. Get the bitmap from the SPA
    //
    // The SPA's almanachExportBitmap() function returns an object:
    //   { width: number, height: number, data: ArrayBuffer }
    //
    // We use CDP's Runtime.evaluate to call this and read back the result.
    var result struct {
        Width  int    `json:"width"`
        Height int    `json:"height"`
        Data   string `json:"data"` // base64-encoded
    }
    err = chromedp.Run(ctx,
        chromedp.Evaluate(`
            window.almanachExportBitmap().then(r => ({
                width: r.width,
                height: r.height,
                data: btoa(String.fromCharCode.apply(null, new Uint8Array(r.data)))
            }))
        `, &result),
    )
    if err != nil {
        return nil, 0, 0, fmt.Errorf("export bitmap: %w", err)
    }

    // 6. Decode the base64 bitmap data
    bitmap, err := base64.StdEncoding.DecodeString(result.Data)
    if err != nil {
        return nil, 0, 0, fmt.Errorf("decode bitmap: %w", err)
    }

    return bitmap, result.Width, result.Height, nil
}
```

### 7.5 Alternative: Screenshot + Convert

If injecting a bitmap export API into the SPA proves unreliable (font loading races, foreignObject issues), we can fall back to Chrome's built-in screenshot capability:

```go
func screenshotAlmanac(ctx context.Context, layoutJSON string) ([]byte, int, int, error) {
    // ... same navigation and injection as above ...

    // Take a screenshot of just the .paper-shell element
    var buf []byte
    err = chromedp.Run(ctx,
        chromedp.Evaluate(fmt.Sprintf(`window.almanachLoadLayout(%s)`, layoutJSON), nil),
        chromedp.Sleep(500*time.Millisecond),
        chromedp.Screenshot(".paper-shell", &buf, chromedp.NodeVisible),
    )
    // buf is a PNG image
    // Decode it, binarize, pack to 1-bit bitmap
    return pngToBitmap(buf)
}
```

The Screenshot approach is simpler but produces a PNG that must then be decoded and converted to a 1-bit bitmap in Go. The SPA-side bitmap export is faster (no image decode step) and more precise (exact pixel-level control).

### 7.6 Chrome Process Lifecycle

Chrome is a heavy process (~100-200 MB RAM). We do not want to start and stop it for every render request. Instead, we use a persistent Chrome instance with per-request tabs:

```go
// Global allocator (starts Chrome once)
var allocatorCtx context.Context
var cancelAllocator context.CancelFunc

func init() {
    allocatorCtx, cancelAllocator = chromedp.NewExecAllocator(
        context.Background(),
        newChromeAllocatorFlags()...,
    )
}

func render(ctx context.Context, layout string) (Result, error) {
    // Each render gets its own tab (context), but shares the Chrome process
    ctx, cancel := chromedp.NewContext(allocatorCtx)
    defer cancel() // closes the tab, not Chrome

    // Set a timeout for this specific render
    ctx, timeoutCancel := context.WithTimeout(ctx, 30*time.Second)
    defer timeoutCancel()

    return renderAlmanac(ctx, layout)
}
```

---

## 8. Bitmap Conversion Pipeline

This section covers the case where we need to convert a PNG screenshot to a 1-bit bitmap in Go. If the SPA's `almanachExportBitmap()` is used, this step is not needed — the SPA returns already-packed bitmap bytes. But the Go-side converter is useful as a fallback and for debugging.

### 8.1 PNG to 1-Bit Bitmap Conversion

```go
package render

import (
    "bytes"
    "image"
    _ "image/png" // register PNG decoder
)

// Bitmap represents a 1-bit monochrome image in MSB-first packed format.
type Bitmap struct {
    Width       int      // in pixels (must be divisible by 8)
    Height      int      // in pixels
    BytesPerRow int      // Width / 8
    Data        []byte   // packed pixel data
}

// ToBitmap converts a PNG image to a 1-bit monochrome bitmap.
// Threshold: pixels with gray < threshold become black (1), else white (0).
func PngToBitmap(pngData []byte, threshold uint8) (*Bitmap, error) {
    img, err := png.Decode(bytes.NewReader(pngData))
    if err != nil {
        return nil, err
    }
    return imageToBitmap(img, threshold)
}

func imageToBitmap(img image.Image, threshold uint8) (*Bitmap, error) {
    bounds := img.Bounds()
    w := bounds.Dx()
    h := bounds.Dy()

    // Pad width to next multiple of 8
    wPadded := ((w + 7) / 8) * 8
    bytesPerRow := wPadded / 8
    data := make([]byte, bytesPerRow*h)

    for y := 0; y < h; y++ {
        for x := 0; x < w; x++ {
            // Convert to grayscale using luminance weights
            r, g, b, _ := img.At(bounds.Min.X+x, bounds.Min.Y+y).RGBA()
            gray := uint8((0.299*float64(r) + 0.587*float64(g) + 0.114*float64(b)) / 256.0)

            if gray < threshold {
                // Black pixel — set the bit
                data[y*bytesPerRow+x/8] |= byte(0x80) >> (x % 8)
            }
            // White pixel — leave bit as 0 (already zeroed)
        }
    }

    return &Bitmap{
        Width:       wPadded,
        Height:      h,
        BytesPerRow: bytesPerRow,
        Data:        data,
    }, nil
}
```

### 8.2 Floyd-Steinberg Dithering (Optional)

For images that have grayscale content (e.g., photographs), simple thresholding produces harsh edges. Floyd-Steinberg dithering distributes quantization error to neighboring pixels, creating a more natural-looking halftone:

```go
func floydSteinbergDither(gray [][]float64, width, height int) [][]bool {
    bw := make([][]bool, height)
    for y := 0; y < height; y++ {
        bw[y] = make([]bool, width)
    }

    for y := 0; y < height; y++ {
        for x := 0; x < width; x++ {
            old := gray[y][x]
            var newVal float64
            if old > 128 {
                newVal = 255
                bw[y][x] = false // white
            } else {
                newVal = 0
                bw[y][x] = true // black
            }
            err := old - newVal
            if x+1 < width {
                gray[y][x+1] += err * 7 / 16
            }
            if y+1 < height {
                if x > 0 {
                    gray[y+1][x-1] += err * 3 / 16
                }
                gray[y+1][x] += err * 5 / 16
                if x+1 < width {
                    gray[y+1][x+1] += err * 1 / 16
                }
            }
        }
    }
    return bw
}
```

For the almanac use case, Floyd-Steinberg is not needed — all content is already pure black and white. But the Go server should support it for future use (e.g., printing photographs).

### 8.3 Sending the Bitmap to the ESP32

```go
func (s *Server) printBitmap(printerURL string, bitmap *Bitmap, feedLines int) error {
    body := bytes.NewReader(bitmap.Data)

    req, err := http.NewRequest("POST", printerURL+"/api/print/bitmap", body)
    if err != nil {
        return err
    }

    req.Header.Set("Content-Type", "application/octet-stream")
    req.Header.Set("X-Width", fmt.Sprintf("%d", bitmap.Width))
    req.Header.Set("X-Height", fmt.Sprintf("%d", bitmap.Height))
    req.Header.Set("X-Feed", fmt.Sprintf("%d", feedLines))

    resp, err := http.DefaultClient.Do(req)
    if err != nil {
        return fmt.Errorf("printer request failed: %w", err)
    }
    defer resp.Body.Close()

    if resp.StatusCode != 200 {
        body, _ := io.ReadAll(resp.Body)
        return fmt.Errorf("printer returned %d: %s", resp.StatusCode, body)
    }

    return nil
}
```

---

## 9. Go Project Structure

The Go server lives in `stoms3r/cmd/almanach-render-service/`:

```
stoms3r/
├── cmd/
│   └── almanach-render-service/
│       ├── main.go                  # Entry point — config, HTTP server, graceful shutdown
│       ├── config.go                # Configuration struct + env/var loading
│       ├── handlers.go              # HTTP handlers for /api/render, /api/schedule, etc.
│       ├── renderer.go              # Chrome headless render orchestration
│       ├── bitmap.go                # PNG → 1-bit bitmap conversion, MSB packing
│       ├── printer.go               # ESP32 print API client
│       ├── fetcher.go               # Interface + registry for data fetchers
│       ├── fetchers/
│       │   ├── weather.go           # OpenWeatherMap / wttr.in fetcher
│       │   ├── news.go              # RSS feed fetcher
│       │   ├── quote.go             # Quotable API / local pool fetcher
│       │   ├── word.go              # Wordnik / local dictionary fetcher
│       │   ├── history.go           # Wikipedia "On this day" fetcher
│       │   └── date.go              # Date/day name computation (local)
│       ├── layout.go                # Block data types, JSON layout builder
│       ├── scheduler.go             # Cron-based auto-print scheduler
│       └── static.go                # Serve Almanach Studio SPA files
├── web/
│   └── almanach/
│       ├── src/
│       │   ├── almanach-studio.jsx  # (modified) — adds window.almanach* API
│       │   └── index.jsx
│       └── dist/
│           ├── almanach.html
│           └── almanach-bundle.js   # Embedded via Go embed or loaded from disk
├── go.mod
├── go.sum
├── Makefile
└── Dockerfile                       # Optional: Docker build with Chrome + Go binary
```

### 9.1 Key Go Dependencies

```
require (
    github.com/chromedp/chromedp v0.11.x    // Chrome DevTools Protocol
    github.com/robfig/cron/v3 v3.0.x         // Cron scheduling
)
```

That's it. Two external dependencies. Everything else is standard library.

### 9.2 Static File Serving

The Go server needs to serve the SPA files to Chrome. Two approaches:

**Option A: Go `embed`**

```go
//go:embed web/almanach/dist/almanach.html
var almanachHTML []byte

//go:embed web/almanach/dist/almanach-bundle.js
var almanachBundle []byte

func serveStatic(mux *http.ServeMux) {
    mux.HandleFunc("/almanach", func(w http.ResponseWriter, r *http.Request) {
        w.Header().Set("Content-Type", "text/html; charset=utf-8")
        w.Write(almanachHTML)
    })
    mux.HandleFunc("/almanach/bundle.js", func(w http.ResponseWriter, r *http.Request) {
        w.Header().Set("Content-Type", "application/javascript; charset=utf-8")
        w.Header().Set("Cache-Control", "public, max-age=86400")
        w.Write(almanachBundle)
    })
}
```

**Option B: Load from disk at startup** (better for development — no rebuild needed when editing JSX):

```go
func serveStatic(mux *http.ServeMux, webDir string) {
    mux.Handle("/almanach", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        http.ServeFile(w, r, filepath.Join(webDir, "almanach.html"))
    }))
    mux.Handle("/almanach/bundle.js", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        http.ServeFile(w, r, filepath.Join(webDir, "almanach-bundle.js"))
    }))
}
```

Recommendation: Use Option B for development, Option A for production builds.

---

## 10. Detailed Implementation Plan

### Phase 1: Skeleton Server (Day 1)

**Goal:** Get a Go HTTP server running that can serve the SPA files and expose a health endpoint.

```go
// main.go

func main() {
    cfg := loadConfig()

    mux := http.NewServeMux()
    mux.HandleFunc("/health", handleHealth)
    serveStatic(mux, cfg.WebDir)

    srv := &http.Server{
        Addr:         fmt.Sprintf(":%d", cfg.Port),
        Handler:      mux,
        ReadTimeout:  10 * time.Second,
        WriteTimeout: 60 * time.Second, // long for render requests
    }

    log.Printf("Almanach Render Service listening on :%d", cfg.Port)
    log.Fatal(srv.ListenAndServe())
}
```

**Tasks:**
- [ ] Create `go.mod` with module path `github.com/mmanuel/stoms3r/cmd/almanach-render-service`
- [ ] Implement `main.go` with config loading, static file serving, graceful shutdown
- [ ] Implement `config.go` with struct: `{ Port, WebDir, PrinterIP, ChromePath }`
- [ ] Verify: `go run .` starts, serves `/almanach` and `/almanach/bundle.js` correctly

### Phase 2: Chrome Headless Integration (Day 2)

**Goal:** Render an almanac page in Chrome and get a screenshot.

**Tasks:**
- [ ] Add `chromedp` dependency: `go get github.com/chromedp/chromedp`
- [ ] Implement `renderer.go` with `renderAlmanac()` function
- [ ] Modify `almanach-studio.jsx` to add `window.almanachReady`, `window.almanachLoadLayout()`, `window.almanachExportBitmap()`
- [ ] Rebuild the SPA bundle
- [ ] Test: load the SPA in Chrome headless, inject a layout JSON, take a screenshot

### Phase 3: Bitmap Conversion + Print Forwarding (Day 3)

**Goal:** Convert the rendered output to a 1-bit bitmap and send it to the ESP32.

**Tasks:**
- [ ] Implement `bitmap.go` with `PngToBitmap()` and `imageToBitmap()`
- [ ] Implement `printer.go` with `printBitmap()` HTTP client
- [ ] Wire up `POST /api/render-and-print` handler
- [ ] Test: render a page and print it on the thermal printer

### Phase 4: Data Fetchers (Day 4-5)

**Goal:** Fetch live data from real APIs.

**Tasks:**
- [ ] Implement `fetcher.go` with `Fetcher` interface and `FetcherRegistry`
- [ ] Implement `fetchers/date.go` — local date computation, day names, week numbers
- [ ] Implement `fetchers/weather.go` — wttr.in (no API key needed) or OpenWeatherMap
- [ ] Implement `fetchers/news.go` — parse RSS feeds (BBC, Reuters)
- [ ] Implement `fetchers/quote.go` — local quote pool or API
- [ ] Implement `fetchers/word.go` — local word pool or Wordnik API
- [ ] Implement `fetchers/history.go` — Wikipedia "On this day" API
- [ ] Implement `layout.go` — assemble fetched data into JSON layout

### Phase 5: Scheduler + Production (Day 6)

**Goal:** Automated daily printing on a schedule.

**Tasks:**
- [ ] Add `robfig/cron` dependency
- [ ] Implement `scheduler.go` with cron-based auto-print
- [ ] Implement `POST/GET/DELETE /api/schedule` handlers
- [ ] Add Dockerfile with Chrome + Go binary
- [ ] Add systemd unit file for Raspberry Pi deployment
- [ ] Write README with usage instructions

---

## 11. Configuration and Deployment

### 11.1 Configuration

The server is configured via environment variables or a config file:

```
# Required
ALMANACH_PORT=8199                        # HTTP listen port
ALMANACH_PRINTER_IP=192.168.0.126         # ESP32 IP address

# Optional
ALMANACH_WEB_DIR=./web/almanach/dist      # SPA static files directory
ALMANACH_CHROME_PATH=/usr/bin/chromium     # Chrome binary path
ALMANACH_DEFAULT_THEME=minimal             # Theme for auto-generated layouts
ALMANACH_DEFAULT_FEED=3                    # Lines to feed after printing
ALMANACH_FONT_SCALE=1.6                    # Font scale for body text
ALMANACH_PAPER_WIDTH=384                   # Paper width in pixels
ALMANACH_LOG_LEVEL=info                    # Log verbosity
```

### 11.2 Raspberry Pi Deployment

```bash
# Build for ARM64
GOOS=linux GOARCH=arm64 go build -o almanach-render-service .

# Copy to Pi
scp almanach-render-service pi@raspberrypi:~/

# Install Chrome on Pi
sudo apt install chromium-browser

# Run
./almanach-render-service
```

### 11.3 Docker Deployment

```dockerfile
FROM golang:1.24-bookworm AS builder
WORKDIR /app
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 go build -o /almanach-render-service ./cmd/almanach-render-service

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    chromium \
    fonts-noto-core \
    && rm -rf /var/lib/apt/lists/*
COPY --from=builder /almanach-render-service /usr/local/bin/
EXPOSE 8199
ENTRYPOINT ["almanach-render-service"]
```

---

## 12. Error Handling and Reliability

### 12.1 Chrome Crashes

Chrome can crash due to OOM, segfaults, or resource exhaustion. The chromedp allocator detects this and can restart Chrome automatically:

```go
// Use chromedp.WithTargets to detect when Chrome dies
// Use a health check goroutine that periodically creates a test tab
```

### 12.2 ESP32 Unreachable

The ESP32 may be offline, rebooting, or on a different IP. The print forwarder should:

- Use a 10-second timeout for the HTTP request
- Return a clear error message if the connection fails
- Not block the render — the render succeeds even if printing fails
- Cache the bitmap so it can be retried when the printer comes back online

### 12.3 Render Timeouts

A single render should complete in under 5 seconds (Chrome page load ~1s, JS render ~0.5s, export ~0.5s). The Go context timeout is set to 30 seconds as a safety net. If a render exceeds this, the Chrome tab is cancelled and an error is returned.

### 12.4 Data Fetcher Failures

Each data fetcher runs independently with its own timeout. If a fetcher fails (API down, network error, rate limit), it returns an empty block rather than failing the entire render. The layout builder simply omits blocks with no data.

---

## 13. Testing Strategy

### 13.1 Unit Tests

- `bitmap_test.go` — test PNG → bitmap conversion with known test images (all-black, all-white, checkerboard, gradient)
- `layout_test.go` — test JSON layout construction, verify block types and data shapes
- `printer_test.go` — test the HTTP client with a mock HTTP server
- `fetchers/*_test.go` — test each fetcher with mock HTTP responses

### 13.2 Integration Tests

- Start the Go server + Chrome headless
- Call `POST /api/render` with a test layout
- Verify the response contains valid bitmap data with correct dimensions
- Verify `width % 8 == 0`

### 13.3 End-to-End Tests

- Requires a real ESP32 with a connected printer
- Call `POST /api/render-and-print`
- Verify the printer outputs a page
- Visually inspect the printed output for correctness

---

## 14. Security Considerations

- **The render service should NOT be exposed to the public internet.** It runs a headless browser, which is a significant attack surface. Bind to `localhost` or an internal network only.
- **Input validation:** The layout JSON must be validated before being passed to `almanachLoadLayout()`. Sanitize string fields to prevent XSS in the Chrome context.
- **No authentication on the ESP32 API:** The ESP32's HTTP server has no auth. Anyone on the same network can print. This is acceptable for a home network but not for production deployment.
- **Chrome sandboxing:** The `--no-sandbox` flag is needed in Docker but should be avoided on bare metal. Detect the environment and set it conditionally.

---

## 15. File Reference Map

This section maps every existing file you need to understand to its role in the system.

### 15.1 Firmware Source (C)

| File | Lines | What to Study |
|------|-------|---------------|
| `stoms3r/main/web_server.c` | 418 | HTTP handler registration, `api_print_bitmap_post()` — understand the exact bitmap format, headers, and response |
| `stoms3r/main/printer_drv.h` | 215 | Printer driver API — `printer_drv_print_bitmap()`, `printer_drv_feed()`, `printer_drv_reset()` |
| `stoms3r/main/printer_drv.c` | ~300 | UART implementation, `GS v 0` command construction, CTS flow control |
| `stoms3r/main/index.html` | 352 | Existing printer web UI — has JS code for Floyd-Steinberg dithering, bitmap packing, and API calls. Useful as a reference implementation. |
| `stoms3r/main/CMakeLists.txt` | 25 | `EMBED_TXTFILES` — how the SPA assets get into the firmware binary |
| `stoms3r/main/assets/almanach/almanach.html` | ~20 | Host page for the SPA — minimal HTML with `<div id="root">` and script tag |
| `stoms3r/main/assets/almanach/almanach-bundle.js` | ~1 line | Minified React bundle (216 KB) — don't read this, read the source JSX instead |

### 15.2 SPA Source (JSX)

| File | Lines | What to Study |
|------|-------|---------------|
| `stoms3r/web/almanach/src/almanach-studio.jsx` | ~2300 | The entire React component — block types, data shapes, themes, settings, export logic, print logic |
| `stoms3r/web/almanach/src/index.jsx` | 11 | Entry point — mounts the component |
| `stoms3r/web/almanach/esbuild.mjs` | 46 | Build script — produces IIFE bundle, generates HTML host page |

### 15.3 New Go Files (To Be Created)

| File | Purpose |
|------|---------|
| `stoms3r/cmd/almanach-render-service/main.go` | Entry point — config, HTTP server, graceful shutdown |
| `stoms3r/cmd/almanach-render-service/config.go` | Configuration from env vars |
| `stoms3r/cmd/almanach-render-service/handlers.go` | HTTP handlers for /api/render, /api/schedule |
| `stoms3r/cmd/almanach-render-service/renderer.go` | Chrome headless render orchestration (chromedp) |
| `stoms3r/cmd/almanach-render-service/bitmap.go` | PNG → 1-bit bitmap, MSB packing |
| `stoms3r/cmd/almanach-render-service/printer.go` | ESP32 print API client |
| `stoms3r/cmd/almanach-render-service/layout.go` | Block data types, JSON layout builder |
| `stoms3r/cmd/almanach-render-service/scheduler.go` | Cron scheduling |
| `stoms3r/cmd/almanach-render-service/static.go` | Serve SPA files to Chrome |
| `stoms3r/cmd/almanach-render-service/fetcher.go` | Fetcher interface + registry |
| `stoms3r/cmd/almanach-render-service/fetchers/*.go` | Individual data source fetchers |

---

## Appendix A: Quick Reference — ESP32 Bitmap API

```
POST /api/print/bitmap HTTP/1.1
Host: 192.168.0.126
Content-Type: application/octet-stream
X-Width: 384
X-Height: 812
X-Feed: 3
Content-Length: 38976

<38976 bytes of raw 1-bit bitmap data, MSB first>
```

Success response:
```json
{ "ok": true }
```

Error response:
```json
{ "error": "missing or invalid X-Width/X-Height headers" }
```

## Appendix B: Quick Reference — Almanach Studio Layout JSON

```json
{
  "almanach_studio_version": 1,
  "theme": "minimal",
  "paperWidth": 384,
  "bodyScale": 1.6,
  "feedLines": 3,
  "blocks": [
    { "id": "1", "type": "title", "data": { "title": "Daily Almanac", "subtitle": "April 29, 2026" } },
    { "id": "2", "type": "date", "data": { "date": "2026-04-29", "day": "Tuesday" } },
    { "id": "3", "type": "divider", "data": {} },
    { "id": "4", "type": "weather", "data": { "temp": "18°C", "high": "22°C", "low": "14°C", "condition": "Partly cloudy", "humidity": "65%", "wind": "12 km/h NW" } },
    { "id": "5", "type": "news", "data": { "items": [
      { "headline": "Breaking news headline", "source": "BBC", "summary": "One sentence summary." },
      { "headline": "Another headline", "source": "Reuters", "summary": "Another summary." }
    ]} },
    { "id": "6", "type": "plan", "data": { "label": "Today's Plan", "items": [
      { "time": "08:00", "text": "Morning standup", "done": false },
      { "time": "10:00", "text": "Deep work: Project Atlas", "done": false }
    ]} },
    { "id": "7", "type": "quote", "data": { "text": "The unexamined life is not worth living.", "author": "Socrates", "source": "Apology" } },
    { "id": "8", "type": "word", "data": { "word": "serendipity", "definition": "The occurrence of events by chance in a happy way.", "partOfSpeech": "noun", "example": "A fortunate stroke of serendipity." } },
    { "id": "9", "type": "history", "data": { "year": "1945", "event": "Adolf Hitler and Eva Braun commit suicide." } },
    { "id": "10", "type": "habits", "data": { "items": [
      { "name": "Exercise", "done": false },
      { "name": "Read 30 min", "done": false },
      { "name": "Meditate", "done": false }
    ]} },
    { "id": "11", "type": "did_you_know", "data": { "text": "Honey never spoils. Archaeologists have found 3000-year-old honey in Egyptian tombs that was still edible." } }
  ]
}
```

## Appendix C: wttr.in Weather API (No API Key)

```bash
# Simple JSON format
curl "wttr.in/Berlin?format=j1"

# Returns temperature, condition, humidity, wind speed, etc.
# Key fields in the response:
#   .current_condition[0].temp_C
#   .current_condition[0].weatherDesc[0].value
#   .current_condition[0].humidity
#   .current_condition[0].windspeedKmph
#   .weather[0].maxtempC
#   .weather[0].mintempC
```

## Appendix D: Wikipedia "On this Day" API

```bash
# Events that happened on April 29
curl "https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/all/04/29"

# Returns an array of events with .text, .year, .pages[]
# Pick one random event for the "history" block
```
