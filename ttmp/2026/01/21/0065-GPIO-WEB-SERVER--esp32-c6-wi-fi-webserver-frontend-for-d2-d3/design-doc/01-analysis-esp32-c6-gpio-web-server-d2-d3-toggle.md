---
Title: 'Analysis: ESP32-C6 GPIO Web Server (D2/D3 toggle)'
Ticket: 0065-GPIO-WEB-SERVER
Status: active
Topics:
    - esp-idf
    - esp32c6
    - wifi
    - webserver
    - http
    - gpio
    - console
    - usb-serial-jtag
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/Kconfig.projbuild
      Note: Menuconfig for D2/D3 pins
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/app_main.c
      Note: Boot + Wi-Fi callback wiring
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/assets/index.html
      Note: Minimal embedded UI
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/gpio_ctl.c
      Note: GPIO state + active-low mapping
    - Path: 0065-xiao-esp32c6-gpio-web-server/main/http_server.c
      Note: HTTP routes + JSON API
    - Path: ttmp/2026/01/21/0065-GPIO-WEB-SERVER--esp32-c6-wi-fi-webserver-frontend-for-d2-d3/reference/01-xiao-esp32c6-pin-mapping.md
      Note: Pin mapping reference (D3=GPIO21)
ExternalSources: []
Summary: 'Design for ESP32-C6: Wi-Fi via esp_console (USB Serial/JTAG) + embedded web UI and JSON API to toggle active-low D2/D3.'
LastUpdated: 2026-01-21T22:36:30.979010227-05:00
WhatFor: ""
WhenToUse: ""
---



# Analysis: ESP32-C6 GPIO Web Server (D2/D3 toggle)

## Executive Summary

We want a small ESP32-C6 firmware that:

1) joins a Wi‑Fi network chosen at runtime via an `esp_console` REPL (USB Serial/JTAG), and
2) serves a tiny web UI that can toggle two GPIO outputs (XIAO “D2” and “D3”, defaulting to GPIO2/GPIO3).

The design follows the same “device-hosted UI” pattern already present in recent firmwares:

- `0045-xiao-esp32c6-preact-web/` (Wi‑Fi selection via console + embedded web UI)
- `0046-xiao-esp32c6-led-patterns-webui/` (same pattern, plus a device control API)
- `components/wifi_mgr/` + `components/wifi_console/` (shared extraction of the Wi‑Fi console pattern)
- `components/httpd_assets_embed/` (tiny helper for embedding/serving static assets)

The implementation is intentionally minimal:

- HTTP API: JSON over `esp_http_server` (GET state, POST set, POST toggle)
- Frontend: a single embedded `index.html` with a small amount of vanilla JS calling the API
- GPIO control: a small module that maps “logical” state (on/off) to “physical” level (active-high/active-low)

This yields a fast feedback loop: connect Wi‑Fi from the console, open `http://<ip>/`, press buttons, watch D2/D3 change. The default electrical model for this ticket is **active-low outputs** (logical “on” drives the pin low), which matches common relay/LED wiring conventions.

## Problem Statement

We need a reliable way to flip two GPIO outputs remotely, without hard-coding Wi‑Fi credentials and without adding large dependencies.

Constraints and desiderata:

- Target: ESP32-C6 (default board: Seeed Studio XIAO ESP32C6).
- Human interaction: via USB Serial/JTAG console (UART pins may be used for peripherals).
- Network: Wi‑Fi STA mode; credentials entered at runtime and optionally persisted.
- Web surface: small and “boring” (one page, two toggles).
- Firmware quality: predictable behavior on boot, clear logs, and an API that is easy to call from any client.

Non-goals (for this ticket):

- provisioning UX (SoftAP captive portal / BLE provisioning / WPS)
- authentication/authorization on the HTTP interface
- perfect hardware abstraction across arbitrary boards/pinouts

## Proposed Solution

We separate the system into three small subsystems with explicit contracts:

1. **Wi‑Fi subsystem**
   - `wifi_mgr`: owns ESP-IDF Wi‑Fi STA init, event handling, NVS persistence, and exposes a “status snapshot”.
   - `wifi_console`: exposes REPL commands (`wifi scan`, `wifi join`, `wifi status`, …) that call `wifi_mgr`.
   - `app_main`: wires them together and prints the “browse URL” after DHCP.

2. **GPIO control subsystem**
   - Configure D2 and D3 as output GPIOs at boot.
   - Maintain a logical state `{d2, d3} ∈ {0,1}²`.
   - Provide `set(pin, logical_level)` and `toggle(pin)` operations.
   - Map logical to physical output level via an “active-low” option (default: enabled for both pins in this ticket):
     - physical = logical XOR active_low

3. **HTTP server + frontend subsystem**
   - `esp_http_server` serves:
     - `GET /` → embedded HTML UI
     - `GET /api/gpio` → JSON state
     - `POST /api/gpio` → JSON set (one or both pins)
     - `POST /api/gpio/d2/toggle` and `/api/gpio/d3/toggle` → toggle
   - Frontend uses `fetch()` to call the API and render the current state.

### Data model and invariants

Let `L2` and `L3` be logical levels for D2 and D3.

Invariant: the API reports logical levels; hardware sees `P = L XOR active_low`.

This is the simplest rule that:
- keeps API semantics stable (a logical “1” always means “on”), and
- supports boards where an attached LED/relay is wired active-low.

### Concurrency model

The HTTP server runs request handlers on its worker threads. GPIO operations are short and can be performed directly in handlers.

To avoid “read-modify-write” races on toggles, we keep the logical state in a small critical section (mutex/spinlock) or use an atomic variable, then perform:

1) read current logical
2) compute next logical
3) write logical + `gpio_set_level()`

In practice, the critical section is small, and GPIO operations are fast.

## Design Decisions

### Why Wi‑Fi selection via console?

Console-driven Wi‑Fi selection is a good fit for development and lab tooling:

- avoids baking secrets into firmware images
- makes it easy to move the device between networks
- leverages existing, tested code (`components/wifi_mgr`, `components/wifi_console`)
- keeps the provisioning surface off the network (no “setup AP” to attack)

This matches the pattern in `0045-xiao-esp32c6-preact-web/` and `0046-xiao-esp32c6-led-patterns-webui/`.

### Why embed static assets instead of a filesystem?

For a tiny UI, embedding wins:

- fewer moving parts (no SPIFFS partition sizing, formatting, mounting, etc.)
- deterministic firmware image contents (one binary)
- simplest deploy story (flash once)

If the UI grows, SPIFFS or an external build pipeline (Vite) becomes more attractive; this repo already contains both patterns.

### Why REST-ish JSON endpoints?

We want the smallest API that is:

- debuggable with `curl`
- easy to call from browser JS without extra libraries
- stable and extensible

WebSockets are excellent for streaming telemetry, but unnecessary for two toggles.

### Why configure D2/D3 with menuconfig?

Pin naming (D2, D3) is board-specific. The safest default is XIAO ESP32C6:

- D0 = GPIO0 (seen in `0042-xiao-esp32c6-d0-gpio-toggle/`)
- D1 = GPIO1 (seen in `0043-xiao-esp32c6-ws2811-4led-d1/`)
- by the board’s published pin map, D2 = GPIO2 and D3 = GPIO21

But making pins configurable keeps the firmware reusable across variants and avoids forcing a hardware assumption.

## Alternatives Considered

### SoftAP captive portal provisioning

Pros: no serial console needed; user-friendly for consumers.

Cons: more code, more state machines, more security surface; for this ticket we prioritize a simple developer workflow.

### BLE provisioning

Pros: avoids temporary open AP.

Cons: higher complexity (pairing UX, BLE stack, phone tooling); not needed for the demo.

### WebSockets for state sync

Pros: nice for real-time dashboards.

Cons: extra protocol surface and lifecycle; polling is fine for two toggles.

### Preact/Vite frontend

Pros: better ergonomics as UI grows; code splitting; type safety; dev server.

Cons: Node toolchain dependency and larger build artifacts. For a two-button UI, plain HTML/JS is the simplest.

## Implementation Plan

1) Scaffold a new ESP32-C6 project in this repo using the shared components:
   - `wifi_mgr`, `wifi_console`, `httpd_assets_embed`
2) Add a small GPIO control module with menuconfig for:
   - D2 GPIO number, D3 GPIO number
   - active-low option (per pin or shared)
3) Implement HTTP server:
   - embed `index.html`
   - implement `/api/gpio` and toggle endpoints
4) Add README with build/flash/use steps:
   - connect via USB Serial/JTAG REPL
   - `wifi scan`, `wifi join`, open browser
5) Relate code files to this design doc and update the ticket changelog.

## Open Questions

1) Pin mapping confirmation: are “D2” and “D3” guaranteed to be GPIO2/GPIO3 for the target board, or is a different board assumed?
2) Security: intentionally none for this ticket (assume trusted LAN/dev environment).

## References

- Similar firmwares in this repo:
  - `0045-xiao-esp32c6-preact-web/` (Wi‑Fi console + embedded UI)
  - `0046-xiao-esp32c6-led-patterns-webui/` (same pattern + control API)
  - `0042-xiao-esp32c6-d0-gpio-toggle/` (XIAO pin mapping example)
- Shared components:
  - `components/wifi_console/`
  - `components/wifi_mgr/`
  - `components/httpd_assets_embed/`
