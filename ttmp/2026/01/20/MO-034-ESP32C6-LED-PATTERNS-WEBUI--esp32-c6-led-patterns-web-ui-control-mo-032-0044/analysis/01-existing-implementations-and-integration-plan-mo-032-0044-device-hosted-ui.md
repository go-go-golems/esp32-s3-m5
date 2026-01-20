---
Title: Existing implementations and integration plan (MO-032 + 0044 + device-hosted UI)
Ticket: MO-034-ESP32C6-LED-PATTERNS-WEBUI
Status: active
Topics:
    - esp32c6
    - esp-idf
    - animation
    - http
    - webserver
    - ui
    - assets
    - preact
    - zustand
    - console
    - usb-serial-jtag
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c
      Note: Console command surface to mirror in the web UI
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c
      Note: Pattern math and speed mappings
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.h
      Note: Pattern config structs and ranges
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.c
      Note: Single-owner render task + queue semantics
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.h
      Note: Control message types and status snapshot struct (maps to API)
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_ws281x.h
      Note: WS281x driver config fields to expose in UI
    - Path: 0045-xiao-esp32c6-preact-web/main/http_server.c
      Note: Reference for serving embedded Vite assets (NUL trimming + optional sourcemaps)
    - Path: 0045-xiao-esp32c6-preact-web/web/vite.config.ts
      Note: Reference deterministic Vite output into firmware assets
    - Path: ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/analysis/01-speed-scaling-brightness-power-and-operator-scripts.md
      Note: Canonical semantics for speed/brightness/power and operator guidance
ExternalSources: []
Summary: Analyze existing MO-032/0044 LED pattern controls and propose a device-hosted web UI + API design to control them (reusing the MO-033/0045 embedded Preact+Zustand bundling pattern).
LastUpdated: 2026-01-20T17:10:41.265142806-05:00
WhatFor: Provide a detailed inventory of existing LED pattern controls (console + firmware), then design the minimum stable HTTP API + UI surface to control all patterns and driver settings from a browser.
WhenToUse: Use before implementing a new firmware/web UI that combines MO-032 (LED patterns console) with MO-033 (device-hosted embedded web UI) so we don't re-invent controls or break existing operator semantics.
---


# Existing implementations and integration plan (MO-032 + 0044 + device-hosted UI)

## Goal

Build a **device-hosted web UI** that can control **all WS281x patterns and driver settings** implemented in:

- Ticket **MO-032-ESP32C6-LED-PATTERNS-CONSOLE** (operator semantics, tuning, playbooks), and
- Firmware **`0044-xiao-esp32c6-ws281x-patterns-console/`** (actual implementation).

The UI should reuse the proven “embed a deterministic Vite bundle and serve it from `esp_http_server`” pattern from MO-033 / `0045-xiao-esp32c6-preact-web/`.

## Existing building blocks (what we already have)

### 1) Pattern engine and message interface (0044)

The core pattern system is already cleanly structured:

- `led_patterns.*` defines the configuration structs and implements the math for:
  - `off`
  - `rainbow`
  - `chase`
  - `breathing`
  - `sparkle`
- `led_task.*` owns a single “renderer task” and exposes a **queue message interface** (`led_task_send`) for:
  - pattern type selection + per-pattern config updates
  - global brightness
  - frame cadence (`frame_ms`)
  - WS281x driver config staging + apply (reinit)
  - pause/resume/clear
  - periodic log enable/disable
- `led_console.c` parses operator commands and sends those queue messages (no direct mutation).

This is ideal for a web UI: the UI can be *just another control surface* producing the same messages.

### 2) Operator semantics and tuning notes (MO-032)

MO-032 already documents the “why” behind certain ranges/mappings:

- Rainbow `speed` is **rotations per minute** (`0..20`)
- Breathing `speed` is **breaths per minute** (`0..20`)
- Chase `speed` is **LEDs per second** (`0..255`, time-integrated position Q16.16)
- Sparkle is **time-based spawn** (speed `0..20` → ~0..4 sparkles/sec) + **concurrent cap** via density

These semantics should be preserved in the UI to avoid “console says one thing, UI behaves differently”.

### 3) Device-hosted UI bundling pattern (MO-033 / 0045)

MO-033/0045 provides the exact playbook we want to reuse:

- Deterministic Vite output into `main/assets/` with stable names (`/assets/app.js`, `/assets/app.css`)
- Serving assets via `esp_http_server` routes
- Known pitfall: `EMBED_TXTFILES` includes a trailing NUL; the server should trim `0x00` before sending JS/CSS/HTML
- Optional: serve sourcemaps (`/assets/app.js.map`), preferably gzipped and embedded via `EMBED_FILES`

## Inventory: what the web UI must be able to control (surface area)

This section converts the existing console command API into UI controls.

### A) “Core” controls

From `led_console.c` help:

- **Status**: `led status`
  - show driver config, animation state, and active pattern parameters
- **Logging**: `led log on|off|status`
  - periodic ESP_LOGI output from LED task
- **Global brightness**: `led brightness <1..100>`
- **Frame cadence**: `led frame <ms>`
- **Pause/Resume/Clear**: `led pause` / `led resume` / `led clear`

UI mapping:

- Top-of-page “Device Status” panel (polling or streaming)
- Global controls panel:
  - Brightness slider (1..100)
  - Frame period input (ms)
  - Buttons: Pause / Resume / Clear
  - Toggle: periodic log enabled

### B) Pattern selection + per-pattern config

Pattern selection:

- `led pattern set <off|rainbow|chase|breathing|sparkle>`

Per-pattern config:

- **Rainbow**: `led rainbow set [--speed 0..20] [--sat 0..100] [--spread 1..50]`
- **Chase**:
  - `--speed 0..255` (LED/s)
  - `--tail 1..255`
  - `--gap 0..255`
  - `--trains 1..255`
  - `--fg #RRGGBB`
  - `--bg #RRGGBB`
  - `--dir forward|reverse|bounce`
  - `--fade 0|1`
- **Breathing**:
  - `--speed 0..20` (breaths/min)
  - `--color #RRGGBB`
  - `--min 0..255`
  - `--max 0..255`
  - `--curve sine|linear|ease`
- **Sparkle**:
  - `--speed 0..20` (spawn rate control)
  - `--color #RRGGBB`
  - `--density 0..100` (max concurrent %)
  - `--fade 1..255`
  - `--mode fixed|random|rainbow`
  - `--bg #RRGGBB`

UI mapping:

- Pattern selector (segmented buttons or dropdown)
- For the selected pattern, a form card:
  - numeric sliders/inputs with the *same ranges* as console
  - color picker (hex) for fg/bg
  - enums as dropdowns (dir, curve, sparkle mode)
  - “Apply” button (and optionally “Live update on change” toggle)

### C) WS281x driver config staging + apply

From `led_console.c` help:

- `led ws status`
- `led ws set gpio <n>`
- `led ws set count <n>`
- `led ws set order <grb|rgb>`
- `led ws set timing [--t0h ns] [--t0l ns] [--t1h ns] [--t1l ns] [--reset us] [--res hz]`
- `led ws apply`

Important behavioral note:

- WS config is **staged**; `apply` reinitializes the RMT driver and reinitializes patterns for the new LED count.

UI mapping:

- “Driver” panel:
  - Staged config editor (gpio/count/order/timing/resolution/reset)
  - “Apply driver config” button (with a confirmation prompt)
  - Show applied config (from status endpoint)

## Proposed architecture: minimal, stable, and testable

### High-level approach

Create a new “combined” firmware tutorial (likely `0046-...`) that:

1) Reuses `led_task.*`, `led_patterns.*`, `led_ws281x.*` from `0044`
2) Adds `esp_http_server` and embedded assets using the proven `0045` pattern
3) Exposes a small JSON API that maps 1:1 to `led_task_send` messages
4) Embeds a deterministic Preact+Zustand UI bundle that calls that API

We should avoid “web UI directly pokes pattern state” (that would bypass the queue model and create concurrency hazards).

### Why not “drive the console from the web UI”?

It’s tempting to implement “web UI just runs `led ...` commands”. That would:

- be fragile (text parsing, escaping, multi-command sequences)
- be harder to validate (you test parsing twice: UI + console)
- couple web UI to console formatting (breaking changes)

The queue message API is the correct abstraction boundary; the UI should target that.

## Proposed HTTP API (draft)

### General principles

- Keep endpoints coarse-grained and explicit.
- Validate all inputs server-side using the same range rules as the console.
- Responses are JSON (`application/json; charset=utf-8`).
- Prefer idempotent “set config” APIs over “increment” style APIs.

### `GET /api/led/status`

Returns a snapshot of:

- running/paused/log_enabled/frame_ms
- ws_cfg (applied)
- pat_cfg (active)

Suggested response shape:

```json
{
  "ok": true,
  "running": true,
  "paused": false,
  "log_enabled": false,
  "frame_ms": 25,
  "ws": {
    "gpio": 1,
    "led_count": 50,
    "order": "grb",
    "resolution_hz": 10000000,
    "t0h_ns": 350,
    "t0l_ns": 800,
    "t1h_ns": 700,
    "t1l_ns": 600,
    "reset_us": 50
  },
  "pattern": {
    "type": "chase",
    "global_brightness_pct": 25,
    "rainbow": { "speed": 5, "saturation": 100, "spread_x10": 10 },
    "chase": { "speed": 30, "tail_len": 6, "gap_len": 10, "trains": 3, "fg": "#00ffff", "bg": "#000010", "dir": "forward", "fade_tail": true },
    "breathing": { "speed": 10, "color": "#ff00aa", "min_bri": 0, "max_bri": 255, "curve": "sine" },
    "sparkle": { "speed": 10, "color": "#ffffff", "density_pct": 10, "fade_speed": 40, "color_mode": "random", "background": "#000000" }
  }
}
```

Notes:

- Returning the full union payload simplifies the UI (it can keep all forms “hydrated”).
- Enums should be stringified for readability/stability.

### Core controls

- `POST /api/led/pause`
- `POST /api/led/resume`
- `POST /api/led/clear`
- `POST /api/led/frame` body: `{ "frame_ms": 25 }`
- `POST /api/led/brightness` body: `{ "brightness_pct": 25 }`
- `POST /api/led/log` body: `{ "enabled": true }`

### Pattern selection + config

Two workable shapes:

Option A (simple, more endpoints):

- `POST /api/led/pattern` body: `{ "type": "rainbow" }`
- `POST /api/led/rainbow` body: `{ "speed": 5, "saturation": 100, "spread_x10": 10 }`
- `POST /api/led/chase` body: `{ ... }`
- `POST /api/led/breathing` body: `{ ... }`
- `POST /api/led/sparkle` body: `{ ... }`

Option B (single endpoint):

- `POST /api/led/pattern/config` body: `{ "type": "...", "global_brightness_pct": 25, "u": { ... } }`

Recommendation:

- Start with Option A so it mirrors `LED_MSG_SET_*` message types and keeps server code small.

### WS281x driver staging/apply

- `POST /api/led/ws/stage` body includes a `set_mask`-like shape:

```json
{
  "gpio": 1,
  "led_count": 50,
  "order": "grb",
  "timing": { "t0h_ns": 350, "t0l_ns": 800, "t1h_ns": 700, "t1l_ns": 600, "reset_us": 50, "resolution_hz": 10000000 }
}
```

- `POST /api/led/ws/apply` (no body)

Status endpoint should clearly show applied config (and optionally staged config if we decide to expose it).

## Proposed UI state model (Preact + Zustand)

### Core state slices

- `deviceStatus` (fetched from `/api/led/status`)
  - includes ws config + current pattern union config
- `pendingEdits`
  - separate from `deviceStatus` so typing doesn’t immediately spam the device
  - “Apply” submits edits and then refetches status
- `connection`
  - last error, last update timestamp, polling interval

### Fetch strategy

- Start with polling every 250–500ms (cheap to implement, sufficient for control UI).
- Consider WebSocket later if we want:
  - server-push status updates
  - live log streaming
  - higher-rate preview feedback

## Build and bundling considerations

Reuse the MO-033 / 0045 approach:

- deterministic asset names (`/assets/app.js`, `/assets/app.css`)
- embed HTML/JS/CSS via `EMBED_TXTFILES`
- **trim trailing NUL** from embedded TXT assets before `httpd_resp_send`
- if we keep sourcemaps:
  - generate `app.js.map`
  - gzip to `app.js.map.gz`
  - embed via `EMBED_FILES`
  - serve `/assets/app.js.map` with `Content-Encoding: gzip`

## Risks / gotchas

- **RMT reinit via WS apply**:
  - apply should be guarded (confirmation in UI; server should reject nonsensical configs)
  - applying new `led_count` reallocates buffers; watch heap fragmentation
- **Flash size**:
  - avoid dynamic chunks; keep one JS file
  - sourcemaps can be large; gzip them (or gate behind a config)
- **Concurrency**:
  - keep `led_task` as the single owner; all controls go through `led_task_send`
- **Parameter semantics drift**:
  - the UI must follow the MO-032 speed semantics to avoid confusion

## Suggested acceptance criteria (for the eventual implementation)

- Device hosts UI; browser loads with no console errors.
- UI can:
  - select each pattern and adjust all parameters
  - change brightness and frame rate
  - pause/resume/clear
  - stage/apply WS281x driver config
- UI state reflects device state (status endpoint matches what LEDs are doing).
- Console commands still work (if we keep the console enabled), and they stay consistent with the UI.

## Next steps

- Add a design doc specifying the exact endpoint set, JSON schemas, and error responses.
- Implement the combined firmware as a new tutorial directory (prefer new number rather than mutating 0044).
- Implement the UI using the existing Preact+Zustand scaffolding pattern from 0045.
