---
Title: Diary
Ticket: 0029a-ADD-WIFI-CONSOLE
Status: active
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - console
    - wifi
    - http
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0029-mock-zigbee-http-hub/README.md
      Note: End-user instructions for configuring WiFi via console.
    - Path: 0029-mock-zigbee-http-hub/main/app_main.c
      Note: Starts WiFi and the console REPL during boot.
    - Path: 0029-mock-zigbee-http-hub/main/wifi_console.c
      Note: esp_console REPL command implementation (wifi status/set/connect/scan/etc).
    - Path: 0029-mock-zigbee-http-hub/main/wifi_sta.c
      Note: WiFi STA wrapper with NVS-backed credentials
    - Path: 0029-mock-zigbee-http-hub/main/wifi_sta.h
      Note: Public API/types for the hub WiFi wrapper (hub_wifi_*).
    - Path: 0029-mock-zigbee-http-hub/sdkconfig.defaults
      Note: Defaults enabling USB-Serial-JTAG console for Cardputer.
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T07:36:07.469283328-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Record what changed for ticket `0029a-ADD-WIFI-CONSOLE`, including the commands run, failures encountered, and how to validate WiFi configuration over `esp_console` on Cardputer (ESP32-S3).

## Step 1: Add `esp_console` WiFi config + scan

Added a USB-Serial-JTAG `esp_console` REPL to `0029-mock-zigbee-http-hub` so WiFi credentials can be set at runtime (persisted to NVS), plus a `wifi scan` command to list nearby APs. This removes the need to hardcode credentials in the repo and makes “grab the device, set WiFi, go” the default workflow.

The WiFi module was refactored into a small stateful STA wrapper (`hub_wifi_*`) that always initializes WiFi even when credentials are absent, auto-loads saved credentials from NVS, and exposes connect/disconnect/scan/status operations for the console.

**Commit (code):** b2650c3 — "0029: add WiFi esp_console config + scan"

### What I did
- Added a new `esp_console` REPL module and registered a `wifi` command with subcommands (`status`, `set`, `connect`, `disconnect`, `clear`, `scan`).
- Reworked the WiFi STA wrapper to:
  - initialize WiFi even if no SSID is configured yet
  - load credentials from NVS namespace `wifi` keys `ssid`/`pass`
  - optionally fall back to Kconfig creds if present
  - expose `hub_wifi_scan()` using `esp_wifi_scan_start()` + `esp_wifi_scan_get_ap_records()`
- Updated `0029-mock-zigbee-http-hub/README.md` with runtime console usage examples.
- Built the firmware: `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0029-mock-zigbee-http-hub build`

### Why
- Avoid committing SSID/password and make WiFi setup fast during demos.
- Provide a “Zigbee-like coordinator” control plane workflow (console + event-driven internals) for experimentation.

### What worked
- `idf.py -C 0029-mock-zigbee-http-hub build` completes cleanly.
- `wifi scan` lists nearby APs; credentials can be saved and will be reloaded on reboot.

### What didn't work
- Linker failure due to a symbol collision with ESP-IDF internals when using `wifi_sta_disconnect` as a public function name:
  - Error: `multiple definition of 'wifi_sta_disconnect' ... libnet80211.a(...)`
  - Fix: rename the module’s public API to `hub_wifi_*` to avoid clashes.

### What I learned
- Avoid exporting generic WiFi symbol names like `wifi_sta_disconnect`; ESP-IDF/WiFi libs may already define them.

### What was tricky to build
- Autoconnect ordering: credentials must be applied before `esp_wifi_start()` so the `WIFI_EVENT_STA_START` handler can reliably autoconnect without races.
- Concurrency: console commands and event handlers run in different tasks; status is protected with a mutex to avoid tearing.

### What warrants a second pair of eyes
- Thread-safety and state transitions in `0029-mock-zigbee-http-hub/main/wifi_sta.c` around disconnect/retry/autoconnect.
- NVS error handling for partial credential presence (SSID present but password missing).

### What should be done in the future
- Optional: add a `wifi set ... connect` convenience flag to make setup one command.
- Optional: add a small “WiFi configured?” banner on the device screen (if/when this firmware gains UI).

### Code review instructions
- Start at `0029-mock-zigbee-http-hub/main/wifi_sta.c` (`hub_wifi_start`, event handler, NVS helpers).
- Then `0029-mock-zigbee-http-hub/main/wifi_console.c` (`wifi` command parsing + scan output).
- Validate:
  - Build: `source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0029-mock-zigbee-http-hub build`
  - On device (Serial/JTAG console):
    - `wifi scan`
    - `wifi set <ssid> <password> save`
    - `wifi connect`
    - `wifi status`
    - reboot and confirm auto-load; `wifi clear` to forget

### Technical details
- NVS namespace: `wifi`
- Keys: `ssid` (string), `pass` (string)
- Console prompt: `hub> `
