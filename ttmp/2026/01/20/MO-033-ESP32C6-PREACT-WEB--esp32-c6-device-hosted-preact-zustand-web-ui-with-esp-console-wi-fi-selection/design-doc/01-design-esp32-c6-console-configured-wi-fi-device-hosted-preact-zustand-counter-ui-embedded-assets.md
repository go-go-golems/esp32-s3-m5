---
Title: 'Design: ESP32-C6 console-configured Wi-Fi + device-hosted Preact/Zustand counter UI (embedded assets)'
Ticket: MO-033-ESP32C6-PREACT-WEB
Status: active
Topics:
    - esp32c6
    - esp-idf
    - wifi
    - console
    - http
    - webserver
    - ui
    - assets
    - preact
    - zustand
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: Reference for serving embedded index/js/css
    - Path: 0017-atoms3r-web-ui/web/vite.config.ts
      Note: Reference for deterministic bundle outputs
    - Path: 0029-mock-zigbee-http-hub/main/wifi_console.c
      Note: Reference for REPL backend selection + wifi command
    - Path: 0029-mock-zigbee-http-hub/main/wifi_sta.c
      Note: Reference for Wi-Fi state machine + NVS credentials
    - Path: 0042-xiao-esp32c6-d0-gpio-toggle/README.md
      Note: Reference for ESP32-C6 target + flash commands
    - Path: docs/playbook-embedded-preact-zustand-webui.md
      Note: Reference for deterministic Vite bundling and embedding
ExternalSources: []
Summary: 'Detailed design for an ESP32-C6 demo firmware: esp_console Wi-Fi selection (scan/join/save) + esp_http_server serving an embedded Vite (Preact+Zustand) counter UI bundle with a deterministic build pipeline.'
LastUpdated: 2026-01-20T15:06:23.092223939-05:00
WhatFor: Provide a concrete, implementable blueprint (modules, commands, endpoints, build pipeline, and validation steps) for MO-033.
WhenToUse: Use when implementing MO-033 or reviewing the design for scope, UX, and technical choices.
---


# Design: ESP32-C6 console-configured Wi-Fi + device-hosted Preact/Zustand counter UI (embedded assets)

## Executive Summary

Implement a small, reproducible “device-hosted web UI” demo on **ESP32‑C6**:

- Use an interactive **`esp_console` REPL** (default **USB Serial/JTAG**) to:
  - scan nearby Wi‑Fi networks
  - select/join a network at runtime
  - optionally persist credentials in NVS
- Use ESP‑IDF native **`esp_http_server`** to serve:
  - an embedded **Vite + Preact + Zustand** “counter” SPA bundle from flash
  - an optional `/api/status` JSON endpoint for “prove device connectivity” UX
- Use a deterministic Vite build config (no hashed filenames, single JS bundle) so the firmware can hard-code 3 static routes.

This design is deliberately based on proven patterns already present in this repo:

- Wi‑Fi console + NVS persistence: `0029-mock-zigbee-http-hub/main/wifi_console.c` and `wifi_sta.c`
- Embedded Preact/Zustand bundle: `docs/playbook-embedded-preact-zustand-webui.md` and `0017-atoms3r-web-ui/`
- ESP32‑C6 console defaults: `0042-xiao-esp32c6-d0-gpio-toggle/sdkconfig.defaults`

## Problem Statement

We want a tiny ESP32‑C6 firmware that demonstrates three things with minimal friction:

1) **Runtime Wi‑Fi selection** without reflashing:
   - scan for networks
   - choose one
   - connect and print a browse URL
2) **Firmware-hosted static web UI** with modern tooling:
   - device serves its own SPA bundle
   - no external web server required
3) A **build pipeline that is predictable** for firmware embedding:
   - deterministic filenames so routes don’t change
   - small number of embedded assets
   - easy “edit web → rebuild assets → build firmware” loop

## Proposed Solution

### 1) User experience

Boot behavior:

- start `esp_console` and show prompt, e.g. `c6> `
- initialize Wi‑Fi STA stack
- if credentials exist in NVS, optionally auto-connect (configurable)

REPL workflow:

```text
c6> wifi scan
found 6 networks (showing up to 20):
  0: ch=6 rssi=-45 auth=WPA2 ssid=MyWifi
  1: ch=1 rssi=-71 auth=OPEN ssid=Guest
...
c6> wifi join 0 mypassword --save
credentials set (ssid=MyWifi, saved)
connect requested
STA IP: 192.168.1.123
browse: http://192.168.1.123/
```

Browser workflow:

- user visits `http://<ip>/`
- the page loads from embedded assets (`/`, `/assets/app.js`, `/assets/app.css`)
- the counter UI works (local Zustand state)
- the UI optionally shows device status fetched from `/api/status`

### 2) Firmware architecture (modules)

Recommended file/module split for a new project directory (name illustrative):

```
0045-xiao-esp32c6-preact-web/
  main/
    app_main.c
    console_repl.c/.h        # starts esp_console + registers wifi command
    wifi_mgr.c/.h            # STA state + NVS creds + scan + connect
    http_server.c/.h         # esp_http_server + static routes + /api/status
    assets/                  # embedded web assets
      index.html
      assets/app.js
      assets/app.css
  web/                       # Vite + Preact + Zustand
```

#### `wifi_mgr` responsibilities

- Initialize NVS, netif, event loop, and Wi‑Fi (`esp_wifi_init`, `esp_wifi_set_mode(WIFI_MODE_STA)`, `esp_wifi_start`).
- Own the “current credentials” state:
  - runtime credentials (in RAM)
  - saved credentials (NVS)
- Expose a small API (example):
  - `wifi_mgr_start()`
  - `wifi_mgr_get_status(wifi_mgr_status_t*)`
  - `wifi_mgr_scan(out, max, *n)`
  - `wifi_mgr_set_credentials(ssid, pass, save)`
  - `wifi_mgr_connect()`
  - `wifi_mgr_disconnect()`
  - `wifi_mgr_clear_credentials()`
- Emit a “got IP” callback or event so `http_server` can start once STA is connected.

This should be largely lifted from `0029-mock-zigbee-http-hub/main/wifi_sta.c`, with symbol renames and removal of unrelated hub concepts.

#### `console_repl` responsibilities

- Start `esp_console` over USB Serial/JTAG by default.
- Register a `wifi` command and subcommands:
  - `wifi status`
  - `wifi scan [max]`
  - `wifi set --ssid <ssid> --pass <pass> [--save]`
  - `wifi join <index> <pass> [--save]` (select from scan results)
  - `wifi connect`
  - `wifi disconnect`
  - `wifi clear`
- Reduce log noise after REPL start (optional Kconfig toggle), following `0029`’s “quiet logs while console” pattern.

#### `http_server` responsibilities

- Serve embedded assets:
  - `GET /` → `index.html`
  - `GET /assets/app.js` → JS
  - `GET /assets/app.css` → CSS
- Provide minimal API:
  - `GET /api/status` → JSON containing at least:
    - `ok: true`
    - `ip` (string) or `ip4` (u32)
    - `rssi` if available
    - `free_heap`
    - `build`/`version`
- Start only after STA is connected (has IP) initially (keeps scope small).

### 2.1) Console command specification (Wi‑Fi)

Command: `wifi`

Usage (suggested):

```text
wifi status
wifi scan [max]
wifi set <ssid> <password> [save]
wifi set --ssid <ssid> --pass <password> [--save]
wifi join <index> <password> [--save]
wifi connect
wifi disconnect
wifi clear
```

Behavior details:

- `wifi scan`:
  - prints at most `max` entries (default 20; clamp to sane bounds, e.g. 1..200)
  - prints index → (channel, RSSI, auth mode, SSID)
  - stores the displayed scan list in RAM so `wifi join <index>` works
- `wifi join <index>`:
  - requires a prior `wifi scan` (otherwise print “scan first”)
  - selects SSID from the stored list at the given index
  - uses provided password and optionally saves to NVS
  - triggers connect (or instruct user to run `wifi connect`, depending on chosen UX)
- `wifi set`:
  - sets runtime credentials by SSID directly (still useful even with `join`)
  - never prints passwords
- `wifi clear`:
  - clears saved creds in NVS and clears runtime creds in RAM

### 2.2) HTTP API contract (minimal)

Static routes:

- `GET /` → `text/html; charset=utf-8`
- `GET /assets/app.js` → `application/javascript`
- `GET /assets/app.css` → `text/css`

Recommended headers (during development, to reduce cache confusion):

- `Cache-Control: no-store`

Minimal JSON endpoint:

- `GET /api/status` → `application/json`

Example response shape:

```json
{
  "ok": true,
  "chip": "esp32c6",
  "ip": "192.168.1.123",
  "rssi": -52,
  "free_heap": 123456,
  "build": "MO-033 2026-01-20"
}
```

Notes:

- Avoid returning the Wi‑Fi password.
- SSID exposure is optional; consider a Kconfig toggle if included.

### 3) Frontend architecture (counter example)

Minimal Vite app structure:

```
web/src/
  main.tsx
  app.tsx
  store/counter.ts
  api/status.ts       # optional
  app.css
```

Zustand counter store:

- state:
  - `count: number`
- actions:
  - `inc()`, `dec()`, `reset()`

Optional: `api/status.ts` provides a typed helper to fetch `/api/status` and display info like “connected IP”.

### 4) Build pipeline (Vite → embed)

Adopt the repo’s established deterministic approach (`docs/playbook-embedded-preact-zustand-webui.md`):

- Vite emits deterministic files:
  - `assets/app.js`
  - `assets/app.css`
- Vite outputs directly into firmware’s asset directory:
  - `outDir: '../main/assets'`
- No extra files are copied:
  - `publicDir: false`
- Bundle stays single-file:
  - `inlineDynamicImports: true`

Firmware embeds:

- `EMBED_TXTFILES`:
  - `assets/index.html`
  - `assets/assets/app.js`
  - `assets/assets/app.css`

Firmware serves:

- correct `Content-Type`
- conservative caching headers (`Cache-Control: no-store`) due to deterministic names

### 4.1) Suggested `sdkconfig.defaults` baseline (ESP32‑C6)

Mirror existing C6 projects in this repo:

- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
- `# CONFIG_ESP_CONSOLE_UART is not set`

And keep Wi‑Fi settings minimal (STA mode; credentials set via console + NVS).

## Design Decisions

### ESP-IDF native approach

- Use `esp_http_server` and `esp_console`.
- Avoid Arduino or AsyncWebServer to keep the tutorial consistent with existing ESP‑IDF-first work.

### Deterministic asset names

- Hardcode routes and keep the asset list tiny (3 files).
- Avoid manifest parsing or filesystem serving complexity.

### NVS persistence with explicit “save”

- Permit trying credentials without persisting secrets.
- Save only when `--save` is supplied.

### Console backend: USB Serial/JTAG by default

- Mirrors `0042`/`0043` (ESP32‑C6) defaults.
- Avoids UART pin conflicts.

## Alternatives Considered

### SoftAP fallback

- Adds significant scope (AP config, UX, routing), not required for the stated goal.
- Defer until after STA-only workflow is stable.

### FS-partition asset serving (SPIFFS/LittleFS/FATFS)

- Great for large UIs, but unnecessary for a tiny counter demo.
- Introduces partition build and runtime filesystem complexity.

### Hashed asset filenames

- Better caching semantics for web apps, but complicates firmware routing.
- Deterministic names are simpler and match existing repo guidance for embedded UIs.

## Implementation Plan

1) Create a new ESP32‑C6 tutorial directory (likely XIAO ESP32C6) based on `0042`.
2) Implement `wifi_mgr` by extracting the minimal pieces of `0029`’s `wifi_sta.c`:
   - start + scan + connect + NVS save/clear + status
3) Implement `console_repl` by extracting the Wi‑Fi pieces of `0029`’s `wifi_console.c`:
   - add `wifi join <index>` convenience command (store scan results in RAM)
4) Implement `http_server` based on `0017`’s static serving pattern (but simpler):
   - embed+serve `index.html`, `app.js`, `app.css`
   - add `/api/status`
5) Add `web/` Vite project:
   - Preact + Zustand counter example
   - deterministic `vite.config.ts` output to `../main/assets`
6) Add a project playbook (this ticket already includes one) and verify on real hardware.

## Open Questions

1) Should auto-connect happen on boot when saved creds exist?
   - Recommended: yes, but provide `wifi disconnect` and `wifi clear` for recovery.
2) Should `/api/status` expose SSID by default?
   - Recommended: no by default (privacy); allow via Kconfig toggle if desired.
3) Do we need `wifi join <index>` in v1?
   - It’s the most literal interpretation of “select wifi network”; recommended.

## References

- Analysis: `../analysis/01-codebase-analysis-existing-patterns-for-esp-console-wi-fi-selection-embedded-preact-zustand-assets.md`
- Wi‑Fi console pattern: `../../../../0029-mock-zigbee-http-hub/main/wifi_console.c`
- Wi‑Fi STA manager pattern: `../../../../0029-mock-zigbee-http-hub/main/wifi_sta.c`
- Embedded UI playbook: `../../../../docs/playbook-embedded-preact-zustand-webui.md`
- Embedded UI example: `../../../../0017-atoms3r-web-ui/`
- ESP32‑C6 console defaults: `../../../../0042-xiao-esp32c6-d0-gpio-toggle/sdkconfig.defaults`
