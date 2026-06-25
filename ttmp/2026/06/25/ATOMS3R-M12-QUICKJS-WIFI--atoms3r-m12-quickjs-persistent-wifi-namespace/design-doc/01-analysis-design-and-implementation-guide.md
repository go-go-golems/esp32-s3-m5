---
Title: Analysis Design and Implementation Guide
Ticket: ATOMS3R-M12-QUICKJS-WIFI
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - wifi
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0095-m5dial-wifi-bench/main/wifi_app.c
      Note: Native ESP32-S3 WiFi/NVS/event implementation reference
    - Path: 0095-m5dial-wifi-bench/main/wifi_app.h
      Note: Native ESP32-S3 WiFi API reference for 0103
    - Path: 0095-m5dial-wifi-bench/main/wifi_console.c
      Note: Console command reference for WiFi diagnostics and provisioning
    - Path: 0103-atoms3r-m12-native-quickjs/main/app_main.cpp
      Note: Future WiFi startup integration point
    - Path: 0103-atoms3r-m12-native-quickjs/main/wifi_app.c
      Note: Native STA-only ESP32-S3 WiFi service with NVS credential persistence added in commit badc45a
    - Path: 0103-atoms3r-m12-native-quickjs/main/wifi_app.h
      Note: WiFi service API added in commit badc45a
    - Path: 0103-atoms3r-m12-native-quickjs/main/wifi_command.cpp
      Note: WiFi console provisioning and diagnostics added in commit badc45a
    - Path: 0103-atoms3r-m12-native-quickjs/main/wifi_command.h
      Note: WiFi console registration hook added in commit badc45a
    - Path: 0103-atoms3r-m12-native-quickjs/main/wifi_namespace.cpp
      Note: Reset-safe QuickJS wifi status/request namespace added in commit 8ebff61
    - Path: 0103-atoms3r-m12-native-quickjs/main/wifi_namespace.h
      Note: WiFi namespace installer hook added in commit 8ebff61
    - Path: components/qjs_service/include/qjs_service.h
      Note: Owner-task API for future wifi namespace installation
ExternalSources: []
Summary: Intern-facing guide for adding persistent native ESP32-S3 WiFi and a QuickJS WiFi namespace to AtomS3R M12.
LastUpdated: 2026-06-25T23:30:00-07:00
WhatFor: Use when implementing, reviewing, or validating WiFi persistence and JavaScript WiFi APIs in `0103-atoms3r-m12-native-quickjs`.
WhenToUse: Read before changing WiFi credentials, NVS persistence, network startup, JavaScript `wifi` bindings, or HTTP server prerequisites.
---




# AtomS3R M12 QuickJS Persistent WiFi Namespace — Analysis, Design, and Implementation Guide

## Executive summary

This ticket adds native ESP32-S3 WiFi to `0103-atoms3r-m12-native-quickjs` and exposes a small JavaScript `wifi` namespace. The firmware must persist credentials in NVS, connect to the requested network, report connection state without exposing passwords, and leave the USB Serial/JTAG console as the recovery path.

The target network for validation is SSID `Sonic Guest`. The password was provided by the operator for provisioning and must not be committed to source, ticket docs, or uploaded PDFs. Validation logs should state whether credentials were saved and whether the station connected, but not print the password.

This ticket is a prerequisite for HTTP serving. The HTTP server can start in AP-only or STA mode, but the requested workflow is to connect to the existing guest WiFi and serve content from the device over the local network.

## Current system context

The firmware currently has:

- Native QuickJS via `components/quickjs_native`.
- Owner-task runtime integration via `components/qjs_service`.
- USB Serial/JTAG console commands.
- Read-only `system` QuickJS namespace.
- Bounded persistent `storage` QuickJS namespace backed by FatFs on `/storage`.

WiFi must be added beside these pieces without violating QuickJS ownership.

```text
ESP-IDF WiFi events / IP events
        |
        v
wifi_app service state + mutex
        |
        +--> console commands: wifi status/config/connect/disconnect/scan
        |
        +--> QuickJS wifi namespace
                |
                v
          request/status functions only
```

The WiFi service owns the ESP-IDF WiFi handles and state. JavaScript only gets snapshots and requests. JavaScript must not receive raw pointers, callbacks from ESP-IDF event context, or password strings.

## Repository evidence

The best local reference is `0095-m5dial-wifi-bench`, because it uses native ESP32-S3 WiFi rather than ESP32-P4 remote WiFi:

- `0095-m5dial-wifi-bench/main/wifi_app.h`
- `0095-m5dial-wifi-bench/main/wifi_app.c`
- `0095-m5dial-wifi-bench/main/wifi_console.c`

The relevant API shape is:

```c
typedef enum {
    WIFI_APP_STATE_UNINIT = 0,
    WIFI_APP_STATE_IDLE,
    WIFI_APP_STATE_CONNECTING,
    WIFI_APP_STATE_CONNECTED,
} wifi_app_state_t;

esp_err_t wifi_app_start(void);
esp_err_t wifi_app_get_status(wifi_app_status_t *out);
esp_err_t wifi_app_set_credentials(const char *ssid, const char *password, bool save_to_nvs);
esp_err_t wifi_app_save_credentials(void);
esp_err_t wifi_app_clear_credentials(void);
esp_err_t wifi_app_connect(void);
esp_err_t wifi_app_disconnect(void);
esp_err_t wifi_app_scan(wifi_scan_entry_t *out, size_t max_out, size_t *out_n);
```

For `0103`, keep the shape but change the product names, hostnames, AP defaults, and JavaScript binding behavior.

## WiFi service responsibilities

The native service is responsible for:

- Initializing NVS.
- Initializing `esp_netif`.
- Creating the default event loop if needed.
- Creating AP and STA netifs if APSTA mode is used.
- Starting ESP-IDF WiFi with `WIFI_STORAGE_RAM`.
- Loading saved credentials from NVS.
- Applying runtime credentials to STA config.
- Connecting, disconnecting, and retrying under a bounded retry policy.
- Tracking current status under a mutex.
- Avoiding password output in logs and JavaScript objects.

Suggested product constants:

```c
#define ATOMS3R_WIFI_AP_SSID "AtomS3R-QJS"
#define ATOMS3R_WIFI_AP_PASSWORD "atoms3rqjs"
#define ATOMS3R_WIFI_AP_CHANNEL 1
#define ATOMS3R_WIFI_AP_MAX_CONN 2
#define ATOMS3R_WIFI_HOSTNAME "atoms3r-qjs"
#define ATOMS3R_WIFI_NVS_NAMESPACE "wifi"
#define ATOMS3R_WIFI_NVS_KEY_SSID "ssid"
#define ATOMS3R_WIFI_NVS_KEY_PASS "pass"
#define ATOMS3R_WIFI_MAX_RETRY 10
```

Implementation status as of commit `badc45a`: the first WiFi milestone uses STA-only mode, not APSTA. The copied reference initially brought up APSTA, but hardware validation showed that the guest network gateway also used `192.168.4.1`, which conflicts with the ESP-IDF default SoftAP network. Because this firmware has USB Serial/JTAG recovery and no display, STA-only is the safer default and also saves memory.

APSTA remains a possible future feature, but it should use a non-conflicting SoftAP subnet and should only be added if there is a concrete recovery or provisioning need.

## Credential policy

The operator requested persistent credentials for:

```text
ssid: Sonic Guest
password: [redacted]
```

Implementation rules:

- The password may be sent to the device through a console command during validation.
- The device may store it in NVS.
- Do not commit it to source code.
- Do not include it in design docs, diaries, changelogs, or reMarkable uploads.
- Do not print it in logs.
- Do not return it from `wifi.status()`.

Console provisioning command:

```text
wifi set --ssid "Sonic Guest" --pass "<operator-provided password>" --save
wifi connect
```

After saving, later boots should load the SSID from NVS and autoconnect.

## JavaScript API

The JavaScript namespace should be request/status oriented.

```js
wifi.status()
wifi.configure({ ssid, password, save })
wifi.connect()
wifi.disconnect()
wifi.clearCredentials()
wifi.scan({ maxResults })
```

### `wifi.status()`

Returns a snapshot. It must never include the password.

```js
wifi.status()
// {
//   started: true,
//   state: "idle" | "connecting" | "connected" | "uninit",
//   ssid: "Sonic Guest",
//   hasSavedCredentials: true,
//   hasRuntimeCredentials: true,
//   staIp: "192.168.1.23" | "",
//   apIp: "192.168.4.1" | "",
//   lastDisconnectReason: 0
// }
```

### `wifi.configure(options)`

Sets runtime credentials and optionally persists them.

```js
wifi.configure({ ssid: 'Sonic Guest', password: '...', save: true })
// { ok: true, saved: true, ssid: 'Sonic Guest' }
```

For normal operation, prefer console provisioning so the password does not appear in JavaScript snippets stored in `/storage/scripts`.

### `wifi.connect()` and `wifi.disconnect()`

These should request an operation and return quickly.

```js
wifi.connect()
// { ok: true, requested: 'connect', state: 'connecting' }

wifi.disconnect()
// { ok: true, requested: 'disconnect', state: 'idle' }
```

### `wifi.scan()`

A blocking scan helper exists in the reference code, but JavaScript should not block the QuickJS owner task indefinitely. For the first implementation, either:

1. expose scan only as a console command, or
2. implement a bounded scan worker and return the last result snapshot from JavaScript.

Do not run an unbounded blocking scan directly inside a QuickJS C function.

## Console API

Add console commands before JavaScript bindings. They are easier to debug and are the recovery path.

```text
wifi start
wifi status
wifi set --ssid "Sonic Guest" --pass "<redacted>" --save
wifi connect
wifi disconnect
wifi clear
wifi scan [max]
```

Console output should show:

- state
- SSID
- whether saved credentials exist
- whether runtime credentials exist
- STA IP
- AP IP if AP mode is enabled
- last disconnect reason

It should not show the password.

## State machine

```text
UNINIT
  |
  | wifi_app_start()
  v
IDLE  <--------------------+
  |                        |
  | wifi_app_connect()     | disconnect / failure without retry
  v                        |
CONNECTING                 |
  |                        |
  | IP_EVENT_STA_GOT_IP    |
  v                        |
CONNECTED -----------------+
```

Retry behavior is firmware-owned. JavaScript does not loop and call `connect()` repeatedly; it requests connect once and polls `wifi.status()`.

## Event handling rules

ESP-IDF events happen outside the QuickJS owner task. Therefore:

- Event handlers may update firmware-owned state under a mutex.
- Event handlers may log non-secret status.
- Event handlers must not call `JS_Eval`.
- Event handlers must not call QuickJS C API functions.
- If future scripts need events, use a firmware queue that JavaScript polls.

Pseudocode:

```c
wifi_event_handler(event):
    lock(state)
    switch event:
        STA_CONNECTED: state = CONNECTING
        GOT_IP: state = CONNECTED; sta_ip = ip
        DISCONNECTED:
            sta_ip = 0
            state = IDLE
            if autoconnect and retry_count < max_retry:
                retry_count++
                request esp_wifi_connect()
    unlock(state)
```

## Implementation plan

### Phase 1: Native service and console

Implementation status: complete in commit `badc45a`.

1. Copied/adapted `0095-m5dial-wifi-bench/main/wifi_app.h` to `0103/main/wifi_app.h`.
2. Copied/adapted `0095-m5dial-wifi-bench/main/wifi_app.c` to `0103/main/wifi_app.c`.
3. Renamed constants and log tags to `0103_wifi` / AtomS3R names.
4. Added `wifi_command.{h,cpp}` console commands.
5. Added ESP-IDF component dependencies: `esp_wifi`, `esp_netif`, `esp_event`, `nvs_flash`.
6. Built and booted without credentials.
7. Provisioned SSID `Sonic Guest` with the operator-provided password using the console; the password was redacted from captured logs and not committed.
8. Saved credentials in NVS and connected.
9. Reset the board and confirmed autoconnect.

Validated final status:

```text
wifi status
state=CONNECTED ssid=Sonic Guest saved=yes runtime=yes reason=-1
sta_ip=192.168.4.22
ap_ip=-
```

Memory baseline after STA-only WiFi and QuickJS:

```text
after_wifi: internal_free=261907 8bit_free=8614103 psram_free=8352196
after_qjs:  internal_free=77647  8bit_free=8429843 psram_free=8352196
js status:  internal=74919      8bit=8426579 psram=8351660
```

### Phase 2: QuickJS namespace

Implementation status: complete for status/connect/disconnect/clear in commit `8ebff61`.

1. Added `wifi_namespace.h/cpp`.
2. Installed `wifi` after QuickJS startup and after `js reset`.
3. Implemented `wifi.status()`.
4. Implemented `wifi.connect()` and `wifi.disconnect()` as request functions.
5. Implemented `wifi.clearCredentials()` for recovery, but did not smoke-test it because it would erase the validated credentials.
6. Deliberately did not expose `wifi.configure()` yet; credential provisioning remains console-only to avoid storing passwords in JavaScript files.
7. Deferred JavaScript scan until worker behavior is safe.

Validated JavaScript behavior:

```text
js eval "JSON.stringify(wifi.status())"
{"state":"connecting","ssid":"Sonic Guest","hasSavedCredentials":true,"hasRuntimeCredentials":true,"staIp":"","apIp":"","lastDisconnectReason":-1}

js eval "JSON.stringify(wifi.disconnect())"
{"ok":true,"requested":"disconnect","state":"idle"}

js eval "JSON.stringify(wifi.connect())"
{"ok":true,"requested":"connect","state":"connecting"}

js eval "JSON.stringify(wifi.status())"
{"state":"connected","ssid":"Sonic Guest","hasSavedCredentials":true,"hasRuntimeCredentials":true,"staIp":"192.168.4.22","apIp":"","lastDisconnectReason":8}

js reset
js eval "wifi.status().ssid"
Sonic Guest
```

### Phase 3: HTTP prerequisite validation

1. Record `js status` before WiFi start.
2. Record heap after WiFi start.
3. Record heap while connected.
4. Decide whether HTTP can start with the 1 MiB QuickJS cap unchanged.

## Security and privacy constraints

This firmware is not an auth gateway. The requested HTTP server later is simple serving, not user authentication. Even so, WiFi credentials are sensitive operational state.

Rules:

- No password in Git.
- No password in docs.
- No password in reMarkable PDFs.
- No password in normal logs.
- NVS persistence is allowed because the user explicitly requested persisted credentials.
- A console `wifi clear` command must exist.

## Validation checklist

Build:

```bash
source /home/manuel/esp/esp-idf-5.4.2/export.sh
cd 0103-atoms3r-m12-native-quickjs
idf.py --no-hints build
```

Flash/monitor:

```bash
PORT=/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00
idf.py --no-hints -p "$PORT" flash monitor
```

Provision and connect:

```text
wifi status
wifi set --ssid "Sonic Guest" --pass "<operator-provided password>" --save
wifi connect
wifi status
```

Persistence test:

```text
# reset board
wifi status
js eval "JSON.stringify(wifi.status())"
```

Expected result: saved credentials are present, SSID is visible, password is not visible, and `state` eventually becomes `connected` with a non-empty STA IP.

## Decision records

### Decision: Use native ESP32-S3 WiFi

- **Context:** AtomS3R M12 is ESP32-S3; WiFi is on-chip.
- **Decision:** Use `esp_wifi`, not ESP32-P4 `esp_wifi_remote`.
- **Rationale:** The existing `0095` target already demonstrates native ESP32-S3 WiFi.
- **Status:** accepted.

### Decision: Persist credentials in NVS

- **Context:** The operator requested persisted WiFi credentials.
- **Decision:** Store SSID/password in NVS namespace `wifi`.
- **Rationale:** NVS is the ESP-IDF standard for small persistent config.
- **Status:** accepted.

### Decision: Redact password from docs and logs

- **Context:** The password is operational secret material.
- **Decision:** Use it only for provisioning; do not commit or upload it.
- **Rationale:** Prevents accidental credential disclosure.
- **Status:** accepted.

### Decision: JavaScript gets request/status APIs

- **Context:** QuickJS owner-task functions must remain bounded.
- **Decision:** JavaScript asks for snapshots and requests operations; firmware owns event callbacks and retries.
- **Rationale:** Avoids calling QuickJS from WiFi callbacks and avoids blocking the runtime.
- **Status:** accepted.

## References

- `0095-m5dial-wifi-bench/main/wifi_app.h`
- `0095-m5dial-wifi-bench/main/wifi_app.c`
- `0095-m5dial-wifi-bench/main/wifi_console.c`
- `0103-atoms3r-m12-native-quickjs/main/app_main.cpp`
- `0103-atoms3r-m12-native-quickjs/main/js_command.cpp`
- `components/qjs_service/include/qjs_service.h`
- `components/qjs_service/qjs_service.cpp`
