---
Ticket: ATOMS3R-M12-QUICKJS-WIFI
Title: Tasks
Status: active
Topics:
  - atoms3r
  - esp32s3
  - quickjs
  - javascript
  - firmware
  - wifi
DocType: tasks
---

# Tasks

## Phase 0 — Ticket and guide

- [x] **W0.1 — Create WiFi ticket workspace.** Create `ATOMS3R-M12-QUICKJS-WIFI`.
- [x] **W0.2 — Write intern-facing analysis/design/implementation guide.** Explain native ESP32-S3 WiFi, NVS persistence, JavaScript API, secret-handling, and validation.
- [x] **W0.3 — Relate files, update changelog, run doctor, upload to reMarkable, and commit docs.** Uploaded `AtomS3R QuickJS WiFi Guide.pdf` to `/ai/2026/06/25/ATOMS3R-M12-QUICKJS-WIFI`.

## Phase 1 — Native WiFi service and console

- [x] **W1.1 — Port native ESP32-S3 WiFi service.** Adapted `0095-m5dial-wifi-bench/main/wifi_app.{h,c}` into `0103` with AtomS3R names in commit `badc45a`.
- [x] **W1.2 — Add component dependencies.** Added `esp_wifi`, `esp_netif`, `esp_event`, and `nvs_flash`.
- [x] **W1.3 — Add console commands.** Implemented `wifi start`, `wifi status`, `wifi set`, `wifi save`, `wifi connect`, `wifi disconnect`, `wifi clear`, and `wifi scan`.
- [x] **W1.4 — Build and boot without credentials.** Built and booted; QuickJS/storage still initialized. The first WiFi boot reported no saved credentials.
- [x] **W1.5 — Provision Sonic Guest credentials.** Persisted SSID `Sonic Guest` with the operator-provided password on device; password was redacted from captured logs and not committed.
- [x] **W1.6 — Validate connection and persistence.** After reset, firmware loaded saved credentials from NVS and connected STA-only with IP `192.168.4.22`; status output did not expose the password.

## Phase 2 — QuickJS WiFi namespace

- [ ] **W2.1 — Add `wifi_namespace.{h,cpp}`.** Install reset-safe JavaScript `wifi` object through `qjs_service_run()`.
- [ ] **W2.2 — Implement `wifi.status()`.** Return state, SSID, credential flags, IPs, and disconnect reason without password.
- [ ] **W2.3 — Implement request functions.** Add `wifi.connect()`, `wifi.disconnect()`, `wifi.clearCredentials()`, and optionally `wifi.configure()`.
- [ ] **W2.4 — Validate from JavaScript.** Confirm namespace survives `js reset` and reports connection state.

## Phase 3 — HTTP prerequisite measurements

- [ ] **W3.1 — Capture memory baselines.** Record `js status` before WiFi start, after WiFi start, connected, disconnected, and after reset.
- [ ] **W3.2 — Decide whether 1 MiB QuickJS cap remains sufficient.** Do not raise without evidence.
