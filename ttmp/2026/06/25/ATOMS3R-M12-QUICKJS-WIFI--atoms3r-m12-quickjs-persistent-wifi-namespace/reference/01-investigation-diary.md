---
Title: Investigation Diary
Ticket: ATOMS3R-M12-QUICKJS-WIFI
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - wifi
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Chronological diary for persistent native WiFi and QuickJS WiFi namespace work.
LastUpdated: 2026-06-25T23:30:00-07:00
WhatFor: Use to resume or review WiFi implementation, credential persistence, and JavaScript API work.
WhenToUse: Read before changing WiFi credentials, NVS persistence, JavaScript WiFi bindings, or HTTP prerequisites.
---

# Diary

## Goal

Capture the WiFi work for `0103-atoms3r-m12-native-quickjs`: native ESP32-S3 WiFi, persistent credentials, console recovery, JavaScript `wifi` APIs, and memory validation before HTTP serving.

## Step 1: Create the WiFi ticket and intern guide

This step creates a dedicated WiFi implementation track. The design uses the existing native ESP32-S3 WiFi example in `0095-m5dial-wifi-bench` as the local reference, but adapts it to the AtomS3R QuickJS firmware and the user's requirement to persist guest WiFi credentials.

The WiFi password is treated as operational secret material. The SSID is documented as `Sonic Guest`, but the password is intentionally redacted from committed docs and from the planned reMarkable upload.

### Prompt Context

**User prompt (verbatim):**

> Create a new ticket for each, and Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.
>
> Then add tasks and work on them in sequence, keeping a detailed log as you go.

**Assistant interpretation:** Create a WiFi ticket with a thorough intern-facing guide, tasks, diary, and reMarkable upload, then implement in ordered steps.

**Inferred user intent:** Add persistent networking to the AtomS3R QuickJS firmware in a maintainable way that future developers can understand.

**Commit (code):** N/A — design-only step.

### What I did

- Created ticket `ATOMS3R-M12-QUICKJS-WIFI`.
- Added a design document and diary document.
- Replaced the generated task list with phased WiFi tasks.
- Wrote the WiFi analysis/design/implementation guide.
- Inspected the native ESP32-S3 WiFi reference files:
  - `0095-m5dial-wifi-bench/main/wifi_app.h`
  - `0095-m5dial-wifi-bench/main/wifi_app.c`
  - `0095-m5dial-wifi-bench/main/wifi_console.c`

### Why

- HTTP serving needs network connectivity.
- Credential persistence should be designed carefully before code is added.
- JavaScript should expose request/status APIs, not raw ESP-IDF event callbacks or passwords.

### What worked

- The guide defines the native service responsibilities, console API, JavaScript API, state machine, credential policy, and validation plan.
- The task list splits native WiFi, QuickJS namespace, and memory measurement into separate phases.

### What didn't work

- N/A for this documentation step.

### What I learned

- `0095-m5dial-wifi-bench` is the correct local reference because it uses native ESP32-S3 `esp_wifi`; the ESP32-P4 WiFi6 targets are not the right baseline for AtomS3R.

### What was tricky to build

- The main challenge was recording the real SSID requirement while not leaking the password. The guide states the SSID and redacts the password, and implementation tasks explicitly say not to commit or log it.

### What warrants a second pair of eyes

- Review whether STA-only or APSTA mode should be the first implementation.
- Review whether JavaScript should be allowed to configure credentials or whether provisioning should remain console-only initially.

### What should be done in the future

- Implement the native WiFi service and console commands first.
- Provision and persist the provided credentials on device without writing them to Git.
- Add the JavaScript namespace only after native WiFi is validated.

### Code review instructions

- Start with the WiFi guide's implementation plan.
- Compare the intended API against `0095-m5dial-wifi-bench/main/wifi_app.h`.
- Ensure no password literal appears in committed code or docs.

### Technical details

- Ticket path: `ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-WIFI--atoms3r-m12-quickjs-persistent-wifi-namespace/`.
- Design doc: `design-doc/01-analysis-design-and-implementation-guide.md`.
- Validation SSID: `Sonic Guest`.
- Password: redacted from docs and commits; use only during device provisioning.

## Step 2: Add native STA WiFi service and persist credentials

This step implements the first WiFi milestone. The firmware now starts native ESP32-S3 WiFi, loads saved credentials from NVS, exposes USB console WiFi diagnostics/provisioning commands, and autoconnects to the saved SSID after reset.

The implementation initially copied the reference APSTA shape from `0095-m5dial-wifi-bench`. Hardware validation showed an important board/network-specific issue: the guest network gateway used `192.168.4.1`, which conflicts with ESP-IDF's default SoftAP address. Because this firmware has USB Serial/JTAG recovery and no display, I switched the implementation to STA-only mode before committing.

### Prompt Context

**User prompt (verbatim):** "wifi credentials (persist them) are \"Sonic Guest\" password: \"peaceandjoy\""

**Assistant interpretation:** Provision the AtomS3R firmware with the provided WiFi credentials and persist them on-device, while avoiding committing or uploading the password.

**Inferred user intent:** Make the device join the local guest WiFi automatically so HTTP serving can be validated by IP address.

**Commit (code):** badc45a953171ef88bb08d14674bb99ae465253d — "0103: add native WiFi console with persisted credentials"

### What I did

- Copied and adapted `0095-m5dial-wifi-bench/main/wifi_app.h` into `0103-atoms3r-m12-native-quickjs/main/wifi_app.h`.
- Copied and adapted `0095-m5dial-wifi-bench/main/wifi_app.c` into `0103-atoms3r-m12-native-quickjs/main/wifi_app.c`.
- Added `0103-atoms3r-m12-native-quickjs/main/wifi_command.h`.
- Added `0103-atoms3r-m12-native-quickjs/main/wifi_command.cpp`.
- Updated `main/CMakeLists.txt` with WiFi sources and dependencies:
  - `esp_wifi`
  - `esp_netif`
  - `esp_event`
  - `nvs_flash`
- Updated `app_main.cpp` to start WiFi after storage and before QuickJS.
- Added WiFi console commands:
  - `wifi start`
  - `wifi status`
  - `wifi set`
  - `wifi save`
  - `wifi connect`
  - `wifi disconnect`
  - `wifi clear`
  - `wifi scan`
- Built the firmware.
- Flashed to the AtomS3R by-id USB Serial/JTAG path.
- Provisioned SSID `Sonic Guest` with the operator-provided password and `--save`.
- Redacted the password from captured logs.
- Reset/flashed the board again and confirmed NVS autoconnect.

### Why

- HTTP serving requires an IP address.
- Credentials must survive reset because manual provisioning on every boot would slow development and make HTTP testing fragile.
- USB Serial/JTAG remains the control plane, so WiFi can be STA-only without losing recovery access.

### What worked

- Build passed after adding native WiFi:

```text
0103-atoms3r-m12-native-quickjs.bin binary size 0x141ca0 bytes. Smallest app partition is 0x400000 bytes. 0x2be360 bytes (69%) free.
```

- First WiFi boot without saved credentials started normally and reported no saved credentials.
- Provisioning saved credentials to NVS. The password was redacted from logs and was not committed.
- Final STA-only boot loaded saved credentials:

```text
I (...) 0103_wifi: Loaded Wi-Fi credentials from NVS (ssid=Sonic Guest)
I (...) 0103_wifi: STA configured for ssid=Sonic Guest
I (...) wifi:mode : sta (b4:3a:45:be:16:80)
I (...) 0103_wifi: STA start: connecting...
```

- Final connection succeeded:

```text
I (...) wifi:connected with Sonic Guest, aid = 3, channel 11, BW20, bssid = ...
I (...) 0103_wifi: STA browse: http://192.168.4.22/
I (...) esp_netif_handlers: sta ip: 192.168.4.22, mask: 255.255.252.0, gw: 192.168.4.1
```

- Final console status:

```text
wifi status
state=CONNECTED ssid=Sonic Guest saved=yes runtime=yes reason=-1
sta_ip=192.168.4.22
ap_ip=-
```

- QuickJS still initialized with WiFi enabled:

```text
js status
ready=1 busy=0 evals=0 resets=0 last_eval_ms=0
limits: memory=1048576 stack=65536
quickjs: used=51047 malloc=360 atoms=532
esp_heap: internal=74919 8bit=8426579 psram=8351660
```

### What didn't work

- The copied APSTA design initially brought up a SoftAP at `192.168.4.1`. The guest WiFi also had gateway `192.168.4.1`, creating an address/subnet conflict:

```text
SoftAP browse: http://192.168.4.1/
STA browse: http://192.168.4.22/
sta ip: 192.168.4.22, mask: 255.255.252.0, gw: 192.168.4.1
```

- I fixed this before committing by switching to STA-only mode. `ap_ip=-` is now expected.

### What I learned

- STA-only is the right default for this firmware because there is no display and USB Serial/JTAG is the recovery path.
- Native WiFi consumes substantial internal RAM. After storage+WiFi+QuickJS, internal free heap was about 75 KiB in `js status`, so HTTP work must stay conservative.
- The 4 MiB app partition still has enough room: 69% free after WiFi.

### What was tricky to build

- The tricky part was not the ESP-IDF WiFi bring-up itself; the copied reference was close. The subtle issue was network topology. APSTA looked useful as a fallback, but its default SoftAP subnet conflicted with the actual guest network.
- Credential handling was also sensitive. The password had to be used on-device but not written to docs, commits, or uploaded PDFs. Captured logs were redacted before display.

### What warrants a second pair of eyes

- Review whether STA-only should remain permanent or whether future AP fallback should use a non-conflicting subnet.
- Review internal heap after WiFi before adding HTTP dynamic route dispatch.
- Review whether JavaScript `wifi.configure()` should exist or whether credential provisioning should remain console-only.

### What should be done in the future

- Add the QuickJS `wifi` namespace (`wifi.status()`, `wifi.connect()`, `wifi.disconnect()`, possibly `wifi.clearCredentials()`).
- Avoid JavaScript scan until scan can be bounded or run outside the QuickJS owner task.
- Start HTTP with a static `/healthz` endpoint and no dynamic QuickJS routes first.

### Code review instructions

- Start with `0103-atoms3r-m12-native-quickjs/main/wifi_app.c`.
- Confirm `wifi_app_start()` uses STA-only mode.
- Review `wifi_command.cpp` for password-printing behavior; it should print SSID but not password.
- Build with:

```bash
source /home/manuel/esp/esp-idf-5.4.2/export.sh
cd 0103-atoms3r-m12-native-quickjs
idf.py --no-hints build
```

- Validate with:

```text
wifi status
js status
```

### Technical details

- Code commit: `badc45a`.
- Redacted smoke log: `/tmp/0103-atoms3r-m12-native-quickjs-wifi-sta-smoke.redacted.log`.
- Raw smoke log exists only in `/tmp` and should not be committed because provisioning commands may contain credentials.
- Final STA IP: `192.168.4.22`.
- Final mode: STA-only.

## Step 3: Add the reset-safe QuickJS `wifi` namespace

This step exposes the validated native WiFi service to QuickJS. The namespace is deliberately small: `wifi.status()`, `wifi.connect()`, `wifi.disconnect()`, and `wifi.clearCredentials()`. It does not expose password-setting from JavaScript yet, because provisioning through the console avoids putting passwords into scripts stored on flash.

The namespace follows the same owner-task rule as `system` and `storage`. It is installed with `qjs_service_run()` after QuickJS startup and reinstalled after `js reset`.

### Prompt Context

**User prompt (verbatim):** (see Step 2)

**Assistant interpretation:** Continue the WiFi implementation by exposing safe request/status functions to JavaScript.

**Inferred user intent:** Make WiFi usable from scripts while preserving credential safety and QuickJS runtime ownership.

**Commit (code):** 8ebff61e7b04ab6a85e77a2fe74d4d5b80db1787 — "0103: add QuickJS WiFi namespace"

### What I did

- Added `0103-atoms3r-m12-native-quickjs/main/wifi_namespace.h`.
- Added `0103-atoms3r-m12-native-quickjs/main/wifi_namespace.cpp`.
- Updated `main/CMakeLists.txt` to compile `wifi_namespace.cpp`.
- Updated `app_main.cpp` to install `wifi` after QuickJS startup.
- Updated `js_command.cpp` to reinstall `wifi` after `js reset`.
- Built and flashed the firmware.
- Validated JavaScript status, disconnect, reconnect, connected IP, and reset persistence.

### Why

- HTTP scripts need to inspect network state.
- JavaScript should request WiFi actions but not receive passwords or ESP-IDF handles.
- Reset-safe namespace installation keeps `js reset` useful without losing firmware APIs.

### What worked

- Build passed:

```text
0103-atoms3r-m12-native-quickjs.bin binary size 0x1422e0 bytes. Smallest app partition is 0x400000 bytes. 0x2bdd20 bytes (69%) free.
```

- Initial JavaScript status during connection:

```text
js eval "JSON.stringify(wifi.status())"
[atoms3r-eval] ok=1 timed_out=0 elapsed=3ms
{"state":"connecting","ssid":"Sonic Guest","hasSavedCredentials":true,"hasRuntimeCredentials":true,"staIp":"","apIp":"","lastDisconnectReason":-1}
```

- JavaScript disconnect:

```text
js eval "JSON.stringify(wifi.disconnect())"
[atoms3r-eval] ok=1 timed_out=0 elapsed=10ms
{"ok":true,"requested":"disconnect","state":"idle"}
```

- JavaScript reconnect:

```text
js eval "JSON.stringify(wifi.connect())"
[atoms3r-eval] ok=1 timed_out=0 elapsed=8ms
{"ok":true,"requested":"connect","state":"connecting"}
```

- Final JavaScript connected status:

```text
js eval "JSON.stringify(wifi.status())"
[atoms3r-eval] ok=1 timed_out=0 elapsed=3ms
{"state":"connected","ssid":"Sonic Guest","hasSavedCredentials":true,"hasRuntimeCredentials":true,"staIp":"192.168.4.22","apIp":"","lastDisconnectReason":8}
```

- Reset persistence:

```text
js reset
reset: ESP_OK
js eval "wifi.status().ssid"
[atoms3r-eval] ok=1 timed_out=0 elapsed=2ms
Sonic Guest
```

### What didn't work

- The first build failed because `wifi_namespace.cpp` used `IPSTR` and `IP2STR` without including `esp_netif_ip_addr.h`:

```text
error: 'IPSTR' was not declared in this scope
error: 'IP2STR' was not declared in this scope
```

- Adding `#include "esp_netif_ip_addr.h"` fixed the build.

### What I learned

- The WiFi namespace can safely expose state snapshots and request functions without giving JavaScript access to credentials.
- `wifi.disconnect()` updates `lastDisconnectReason` to `8` on this path, which then remains visible after reconnect. That is useful diagnostic state but should be explained to users.
- The namespace install pattern is now repeated across `system`, `storage`, and `wifi`, suggesting a future shared binding-registration helper may be worthwhile.

### What was tricky to build

- The key design decision was to omit `wifi.configure()` for now. It would be easy to implement, but it would encourage putting passwords into JavaScript snippets under `/scripts`. Console provisioning is safer for this milestone.
- `wifi.clearCredentials()` exists for recovery, but I did not smoke-test it because it would erase the validated on-device credentials.

### What warrants a second pair of eyes

- Review whether `wifi.clearCredentials()` should be exposed to JavaScript or kept console-only.
- Review whether `lastDisconnectReason` should reset after a successful reconnect.
- Review the repeated namespace-install boilerplate across firmware APIs.

### What should be done in the future

- Add HTTP `/healthz` and static serving now that WiFi can be inspected from JavaScript.
- Consider an async scan worker before exposing `wifi.scan()` to JavaScript.
- Keep credential provisioning console-only unless there is a strong reason to script it.

### Code review instructions

- Start with `0103-atoms3r-m12-native-quickjs/main/wifi_namespace.cpp`.
- Confirm `wifi.status()` does not include passwords.
- Confirm `wifi` is installed in `app_main.cpp` and reinstalled in `js_command.cpp`.
- Validate with:

```text
js eval "JSON.stringify(wifi.status())"
js eval "JSON.stringify(wifi.disconnect())"
js eval "JSON.stringify(wifi.connect())"
js reset
js eval "wifi.status().ssid"
```

### Technical details

- Code commit: `8ebff61`.
- Redacted smoke log: `/tmp/0103-atoms3r-m12-native-quickjs-wifi-ns-smoke.redacted.log`.
- Final STA IP during smoke: `192.168.4.22`.
