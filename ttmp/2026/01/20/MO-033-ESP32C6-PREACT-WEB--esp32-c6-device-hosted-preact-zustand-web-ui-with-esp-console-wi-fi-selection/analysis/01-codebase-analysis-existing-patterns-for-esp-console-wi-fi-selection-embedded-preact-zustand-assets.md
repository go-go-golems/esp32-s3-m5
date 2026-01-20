---
Title: 'Codebase Analysis: existing patterns for esp_console Wi-Fi selection + embedded Preact/Zustand assets'
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/main/CMakeLists.txt
      Note: Concrete EMBED_TXTFILES usage for assets
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: Concrete static asset serving routes and types
    - Path: 0017-atoms3r-web-ui/web/vite.config.ts
      Note: Concrete deterministic Vite config
    - Path: 0029-mock-zigbee-http-hub/main/wifi_console.c
      Note: Proven esp_console Wi-Fi command UX (scan/set/connect/status)
    - Path: 0029-mock-zigbee-http-hub/main/wifi_sta.c
      Note: Proven Wi-Fi STA manager with NVS persistence + scan helper
    - Path: 0042-xiao-esp32c6-d0-gpio-toggle/sdkconfig.defaults
      Note: ESP32-C6 console defaults (USB Serial/JTAG)
    - Path: docs/playbook-embedded-preact-zustand-webui.md
      Note: Proven Vite bundling + embedding playbook
ExternalSources: []
Summary: Inventory and extract proven patterns from this repo for esp_console-based Wi-Fi selection (scan/join/save) and for bundling/embedding/serving a deterministic Vite (Preact + Zustand) web UI from ESP-IDF firmware.
LastUpdated: 2026-01-20T15:06:21.619471733-05:00
WhatFor: Prevent re-inventing console/Wi-Fi/web-bundle patterns by pointing MO-033 directly at existing working implementations and docs.
WhenToUse: Use before implementing MO-033 to decide what code to reuse (0029 Wi-Fi console, 0017 Vite bundling + embed serving, 0042/0043 ESP32-C6 defaults) and what gaps need new work.
---


# Codebase Analysis: existing patterns for esp_console Wi-Fi selection + embedded Preact/Zustand assets

## Executive summary

This repo already contains almost everything needed to implement MO-033 cleanly:

1) **Wi-Fi selection via esp_console (runtime + NVS persistence)** exists as a console-friendly implementation in `0029-mock-zigbee-http-hub/`:
   - `0029-mock-zigbee-http-hub/main/wifi_console.c` provides a `wifi` REPL command with:
     - `wifi scan [max]` (indexed results)
     - `wifi set <ssid> <pass> [save]` (positional or `--ssid/--pass`)
     - `wifi connect|disconnect|clear|status`
   - `0029-mock-zigbee-http-hub/main/wifi_sta.c` provides a small Wi‑Fi STA manager with:
     - NVS-backed credentials (namespace `wifi`, keys `ssid`/`pass`)
     - an in-memory runtime slot (set-but-not-save)
     - autoconnect and max retry logic
     - blocking scan helper returning records

2) **Bundling + embedding + serving a device-hosted web UI** exists as:
   - Doc: `docs/playbook-embedded-preact-zustand-webui.md`
   - Firmware example: `0017-atoms3r-web-ui/` (Vite+Preact+Zustand served from `esp_http_server` via embedded assets)

3) **ESP32-C6 project scaffolding and console defaults** exist in:
   - `0042-xiao-esp32c6-d0-gpio-toggle/` and `0043-xiao-esp32c6-ws2811-4led-d1/`
   - both ship `sdkconfig.defaults` that prefer USB Serial/JTAG console and disable UART console

MO-033 should be a composition of:

- ESP32-C6 skeleton conventions from `0042`
- Wi‑Fi console workflow from `0029`
- Embedded web bundle workflow from `0017` + `docs/playbook-embedded-preact-zustand-webui.md`

## In-scope behaviors (as requested)

- Provide a “little webserver” on ESP32‑C6.
- Use `esp_console` to select Wi‑Fi networks at runtime.
- Serve a static bundle of “Preact + Zustand counter example”.
- Bundle “nicely” in the build pipeline (deterministic outputs, embedded assets).

## Findings: proven patterns to reuse

### 1) Wi‑Fi STA manager: state + NVS credentials (0029)

**Primary source**

- `0029-mock-zigbee-http-hub/main/wifi_sta.c`

**Key design features worth copying**

- NVS bootstrap that handles common failure cases:
  - `nvs_flash_init()` with erase fallback on `ESP_ERR_NVS_NO_FREE_PAGES` / `ESP_ERR_NVS_NEW_VERSION_FOUND`
- “Runtime credentials” and “Saved credentials” separation:
  - runtime values in RAM (safe to change frequently)
  - save explicitly to NVS only when requested
- Single mutex to protect shared state:
  - current state (`IDLE`/`CONNECTING`/`CONNECTED`)
  - current SSID (but not logging passwords)
  - last disconnect reason
  - current IP (host order u32)
- `wifi scan` helper:
  - uses `esp_wifi_scan_start(&cfg, true /* block */)`
  - fetches records, copies minimal fields, clears AP list

**Why it’s a good fit**

- It’s already designed to be **console-driven** (no UI assumptions).
- It’s already designed to be **runtime-configurable** with persistence.

### 2) esp_console Wi‑Fi command UX (0029)

**Primary source**

- `0029-mock-zigbee-http-hub/main/wifi_console.c`

**What it already provides**

- A single `wifi` command with subcommands.
- A usage message that is good enough for self-serve discovery.
- A scan output format that includes:
  - index
  - channel
  - RSSI
  - auth mode (converted to string)
  - SSID (handles hidden SSIDs)

**Gap for MO-033**

MO-033 explicitly says “select wifi network”. 0029 supports “scan” but “set” requires typing SSID.

Two good options:

- Add `wifi join <index> <pass> [--save]` based on most recent scan results.
- Treat a numeric first argument to `wifi set` as an index (more magical; less clear).

Recommendation: add `wifi join` for clarity.

### 3) Deterministic Vite bundle + embedded serving (0017 + playbook)

**Primary sources**

- `docs/playbook-embedded-preact-zustand-webui.md`
- `0017-atoms3r-web-ui/web/vite.config.ts`
- `0017-atoms3r-web-ui/main/CMakeLists.txt`
- `0017-atoms3r-web-ui/main/http_server.cpp`

**Key pattern**

- Vite config forces:
  - single JS output (`inlineDynamicImports: true`)
  - stable names (`assets/app.js`, `assets/app.css`)
  - output directly into firmware’s assets directory (`outDir: '../main/assets'`)
  - no accidental `public/` files (`publicDir: false`)
- Firmware embeds those assets via ESP-IDF:
  - `EMBED_TXTFILES` in component CMake
- Firmware registers hard-coded routes:
  - `/` (index)
  - `/assets/app.js`
  - `/assets/app.css`

**Why it’s “nice” as a pipeline**

- Frontend build does not require any copy step; it writes directly to the folder that firmware embeds.
- Firmware routing remains constant across rebuilds.

### 4) ESP32-C6 baseline + console defaults (0042 / 0043)

**Primary sources**

- `0042-xiao-esp32c6-d0-gpio-toggle/sdkconfig.defaults`
- `0043-xiao-esp32c6-ws2811-4led-d1/sdkconfig.defaults`

**Relevant baseline config**

- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
- `# CONFIG_ESP_CONSOLE_UART is not set`

This aligns well with a “console-first” Wi‑Fi selection workflow, and avoids pin conflicts.

## Suggested implementation approach (derived from findings)

1) Create a new ESP32-C6 project directory (likely XIAO C6) modeled on `0042`.
2) Port/copy the Wi‑Fi manager logic from `0029` with minimal renaming.
3) Copy the Vite + embed pattern from `0017` but simplify the UI to a counter + optional `/api/status` fetch.
4) Keep asset list tiny (index + one JS + one CSS).

## Reference pointers (most load-bearing)

- `0029-mock-zigbee-http-hub/main/wifi_console.c`
- `0029-mock-zigbee-http-hub/main/wifi_sta.c`
- `docs/playbook-embedded-preact-zustand-webui.md`
- `0017-atoms3r-web-ui/web/vite.config.ts`
- `0017-atoms3r-web-ui/main/CMakeLists.txt`
- `0017-atoms3r-web-ui/main/http_server.cpp`
- `0042-xiao-esp32c6-d0-gpio-toggle/sdkconfig.defaults`

## Additional deep-dive details (to de-risk implementation)

### Wi‑Fi console command structure (0029)

In `0029-mock-zigbee-http-hub/main/wifi_console.c`:

- `cmd_wifi(argc, argv)` is a classic “subcommand switch” that:
  - validates argument count
  - prints usage on unknown inputs
  - uses plain `printf` for REPL-friendly output
- `wifi scan [max]`:
  - allocates an array of scan entries (`calloc(max, sizeof(entry))`)
  - calls `hub_wifi_scan(entries, max, &n)`
  - prints indexes in the output (this is the basis for MO‑033 “select network”)
- `wifi set`:
  - supports both positional args and `--ssid/--pass` flags
  - supports “save” as either `save` or `--save`
  - does *not* print the password (good pattern)

Porting guidance:

- Keep the top-level structure (it’s easy to maintain and test).
- In MO‑033, remove all `hub` commands and only keep `wifi`.
- Add `wifi join <index> ...` by:
  - storing the last scan results in a static array inside the Wi‑Fi module (bounded, e.g. 20 entries)
  - validating `<index>` against the stored list
  - calling `*_set_credentials(ssid, pass, save)` with the selected SSID

### Wi‑Fi STA manager: event-driven state (0029)

In `0029-mock-zigbee-http-hub/main/wifi_sta.c`:

- A single event handler processes:
  - `WIFI_EVENT_STA_START`
  - `WIFI_EVENT_STA_CONNECTED`
  - `WIFI_EVENT_STA_DISCONNECTED`
  - `IP_EVENT_STA_GOT_IP`
  - `IP_EVENT_STA_LOST_IP`
- It keeps the firmware “console friendly” by:
  - logging the browse URL on `GOT_IP`
  - keeping a small, queryable status struct (`wifi_sta_status_t`)
- Credentials persistence:
  - stored under NVS namespace `"wifi"`
  - keys `"ssid"` and `"pass"` (strings)
  - loaded on startup and copied into the runtime slot

Porting guidance:

- Reuse the NVS schema (namespace + keys) unless there’s a reason to change; it’s already a good default.
- Keep the “runtime slot” concept even if “join by index” is added:
  - it makes it easy to try multiple networks without persisting secrets every time

### Deterministic asset bundling + embedding (0017 + playbook)

The important practical details for MO‑033 are already captured (and proven) in:

- `docs/playbook-embedded-preact-zustand-webui.md`
- `0017-atoms3r-web-ui/web/vite.config.ts`

Core Vite knobs to carry forward:

- `outDir: '../main/assets'` so the firmware always embeds current artifacts
- `inlineDynamicImports: true` so you only embed one JS file
- `entryFileNames: 'assets/app.js'` and CSS mapping to `assets/app.css`
- `publicDir: false` to avoid accidental extra embeds (like `vite.svg`)
- `emptyOutDir: true` so stale assets don’t remain and get embedded

### ESP-IDF embedding symbols: basename collisions matter

In the `EMBED_TXTFILES` pattern used by `0017`:

- embedded linker symbols are effectively derived from the **basename**
  - `index.html` → `_binary_index_html_start/_end`
  - `app.js` → `_binary_app_js_start/_end`

Implication for MO‑033:

- Do not embed multiple files with the same basename within the same component, or symbols may collide.
- Keeping only 3 assets (index/app.js/app.css) avoids this entirely.

### HTTP serving pitfalls that recur in embedded UIs

The most common “it doesn’t load” causes (already seen elsewhere in this repo):

- `index.html` references `assets/app.js` instead of `/assets/app.js` (relative path mistakes)
- firmware registers `/assets/app.js` but Vite emitted a different filename (hashes/chunks)
- browser caches aggressively when deterministic names are used

Mitigations recommended for MO‑033:

- enforce deterministic names in Vite
- serve `Cache-Control: no-store` (at least during development)
