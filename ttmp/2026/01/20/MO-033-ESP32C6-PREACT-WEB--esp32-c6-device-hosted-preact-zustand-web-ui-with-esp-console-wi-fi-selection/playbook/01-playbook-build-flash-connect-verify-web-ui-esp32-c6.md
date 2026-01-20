---
Title: 'Playbook: build + flash + connect + verify web UI (ESP32-C6)'
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
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Repeatable steps to build and run the MO-033 ESP32-C6 firmware and verify esp_console Wi-Fi selection + embedded Preact/Zustand UI serving."
LastUpdated: 2026-01-20T15:06:23.953611819-05:00
WhatFor: "Provide a concrete, repeatable verification flow for the final MO-033 deliverable on real ESP32-C6 hardware."
WhenToUse: "Use after any change to Wi-Fi console, Wi-Fi STA manager, HTTP server, or web bundle embedding."
---

# Playbook: build + flash + connect + verify web UI (ESP32-C6)

## Purpose

Verify the MO-033 demo end-to-end:

- `esp_console` REPL is interactive (USB Serial/JTAG preferred)
- user can scan/select/join Wi‑Fi and optionally save credentials to NVS
- device obtains STA IP via DHCP and prints a browse URL
- browser loads `/` and the embedded `/assets/app.js` + `/assets/app.css`
- optional `/api/status` returns expected JSON
- saved credentials (if used) persist across reboot

## Environment Assumptions

- ESP-IDF 5.4.x installed and working.
- Hardware: ESP32-C6 board connected over USB (assumed: Seeed XIAO ESP32C6).
- Host has Node.js + npm (to build the web bundle).
- Console transport is USB Serial/JTAG (recommended; avoids UART pin conflicts).

## Commands

Note: the project directory name below is illustrative; update to the actual implemented directory (e.g. `0045-xiao-esp32c6-preact-web`).

### 1) Build web assets (Vite → firmware assets dir)

```bash
cd 0045-xiao-esp32c6-preact-web/web
npm ci
npm run build
cd ..
```

Expected outputs exist at deterministic paths:

- `main/assets/index.html`
- `main/assets/assets/app.js`
- `main/assets/assets/app.css`

### 2) Build firmware

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd 0045-xiao-esp32c6-preact-web

idf.py set-target esp32c6
idf.py build
```

### 3) Flash + monitor

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Expected:

- boot logs followed by an interactive prompt (e.g. `c6> `).

### 4) Configure Wi‑Fi from the REPL

At the prompt:

```text
wifi status
wifi scan
wifi join 0 mypassword --save
wifi status
```

Expected:

- logs show a DHCP IP acquisition (e.g. `STA IP: x.x.x.x`)
- `wifi status` indicates `CONNECTED` and prints the IP.

### 5) Verify HTTP + embedded UI

In a browser:

- open `http://<sta-ip>/`

Optional checks:

- `http://<sta-ip>/assets/app.js`
- `http://<sta-ip>/assets/app.css`
- `http://<sta-ip>/api/status`

Expected:

- the UI loads and the counter works
- assets return 200 with correct `Content-Type`

### 6) Persistence test (if `--save` implemented)

1) Reboot device (reset/power cycle).
2) Observe it auto-connects with saved credentials (if designed to).
3) Confirm UI loads again at the printed IP.

## Exit Criteria

- `wifi scan` lists networks.
- `wifi join` (or `wifi set` + `wifi connect`) results in a valid STA IP.
- Browser loads `/` and the page renders.
- Assets `/assets/app.js` and `/assets/app.css` are served successfully.
- Saved credentials persist across reboot (if applicable).

## Notes

- If the UI seems “stuck” while iterating:
  - deterministic filenames can lead to aggressive browser caching
  - hard-refresh, clear cache, or ensure firmware serves `Cache-Control: no-store`.
