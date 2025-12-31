---
Title: Diary
Ticket: 0019-BLE-TEST
Status: active
Topics:
    - ble
    - cardputer
    - sensors
    - esp32s3
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Implementation diary tracking research, analysis, and development progress for BLE temperature sensor logger.
LastUpdated: 2025-12-30T11:25:06.619233048-05:00
WhatFor: Tracking the research and implementation journey
WhenToUse: Reference for understanding decisions and progress
---

# Diary

## Goal

This diary tracks the research, analysis, and implementation of a BLE-based temperature sensor logger on the M5Stack Cardputer. The goal is to create a comprehensive analysis document with all relevant files, symbols, and documentation needed to implement the project.

## Step 1: Ticket Creation and Initial Research

Created ticket `0019-BLE-TEST` and began comprehensive codebase search to understand BLE implementation options and temperature sensor availability.

**Commit (docs):** N/A — Initial research phase

### What I did

- Created ticket using `docmgr ticket create-ticket`
- Searched codebase for BLE implementations
- Searched for temperature sensor examples and I2C access patterns
- Searched for Cardputer hardware configuration
- Reviewed existing BLE keyboard implementation in `M5Cardputer-UserDemo`
- Found SHT3x temperature sensor example in `esp32-s3-m5/0004-i2c-rolling-average/`
- Identified ESP-IDF version (v4.4.6) and BLE stack (Bluedroid)

### Why

Need to understand:
1. What BLE stack is used (Bluedroid vs NimBLE)
2. What temperature sensors are available/examples exist
3. How I2C is accessed on Cardputer
4. What existing BLE code patterns can be reused

### What worked

- Found existing BLE implementation using Bluedroid (`M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/`)
- Found complete SHT3x sensor example with I2C master driver (`esp32-s3-m5/0004-i2c-rolling-average/`)
- Identified ESP-IDF version compatibility considerations
- Found Cardputer hardware constraints (512KB SRAM, no PSRAM)

### What didn't work

- No ESP-IDF BLE examples found in codebase (only HID keyboard implementation)
- No built-in temperature sensor on Cardputer (need external I2C sensor)
- Cardputer I2C pin assignments not immediately clear (need verification)

### What I learned

1. **BLE Stack Choice**: Codebase uses Bluedroid, not NimBLE. For consistency, should use Bluedroid even though NimBLE is lighter.

2. **ESP-IDF Version**: Cardputer uses v4.4.6, but example code uses v5.x I2C APIs. Need to verify compatibility or adapt APIs.

3. **Sensor Options**: SHT3x is best choice because:
   - Example code exists
   - High accuracy (±0.3°C)
   - Includes humidity sensor
   - Well-documented protocol

4. **Architecture Pattern**: Existing BLE code uses callback-based event handling with GAP and GATTS callbacks. Should follow same pattern.

### What was tricky to build

- Determining I2C pin assignments for Cardputer (not documented in analyzed files)
- Understanding ESP-IDF version differences (v4.4.6 legacy I2C API vs v5.x new master driver)
- Identifying which BLE stack to use (both available, but existing code uses Bluedroid)

### What warrants a second pair of eyes

- I2C pin assignment verification (critical before implementation)
- ESP-IDF API compatibility (legacy vs new I2C driver)
- Memory budget validation (BLE stack ~100KB + display ~64KB + sensor ~1KB = ~165KB used, ~350KB remaining)

### What should be done in the future

1. **Verify Cardputer I2C pins** - Check schematic, datasheet, or use I2C scanner
2. **Test ESP-IDF API compatibility** - Verify if v5.x I2C master driver works on v4.4.6, or adapt to legacy API
3. **Create minimal BLE GATT server example** - Start with service + one characteristic before sensor integration
4. **Implement SHT3x driver** - Adapt from example code to Cardputer hardware
5. **Test end-to-end** - Sensor → BLE notification → Linux client

### Code review instructions

- Start with analysis documents:
  - `analysis/01-ble-implementation-analysis-esp-idf-bluedroid-gatt-server.md`
  - `analysis/02-temperature-sensor-and-i2c-access-analysis.md`
- Review existing BLE code: `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/`
- Review sensor example: `esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c`
- Verify I2C pin assignments before implementation

### Technical details

**Key Files Found**:
- `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/ble_keyboard_wrap.cpp` - BLE HID keyboard implementation
- `M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/esp_hid_gap.c` - GAP event handling
- `esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c` - SHT3x sensor example

**Key APIs Identified**:
- Bluedroid: `esp_gatts_api.h`, `esp_gap_ble_api.h`, `esp_bt_main.h`
- I2C (v5.x): `driver/i2c_master.h` - `i2c_new_master_bus()`, `i2c_master_transmit()`
- I2C (v4.4.6): `driver/i2c.h` - `i2c_param_config()`, `i2c_master_cmd_begin()`

**Hardware Constraints**:
- Cardputer: ESP32-S3, 512KB SRAM (no PSRAM)
- Display buffers: ~64KB
- BLE stack: ~100KB
- Available: ~350KB for application

### What I'd do differently next time

- Start with hardware pin verification earlier (I2C pins are critical)
- Check ESP-IDF version compatibility before deep-diving into example code
- Create a hardware verification checklist upfront

## Step 2: Document Creation and Analysis

Created comprehensive analysis documents covering BLE implementation, temperature sensors, debugging playbook, and implementation guide.

**Commit (docs):** N/A — Documentation creation phase

### What I did

- Created 4 documents:
  1. BLE Implementation Analysis (`analysis/01-ble-implementation-analysis-esp-idf-bluedroid-gatt-server.md`)
  2. Temperature Sensor Analysis (`analysis/02-temperature-sensor-and-i2c-access-analysis.md`)
  3. Debugging Playbook (`reference/01-ble-debugging-playbook-and-implementation-guide.md`)
  4. Diary (`reference/02-diary.md`)

- Populated each document with:
  - Executive summaries
  - Technical analysis
  - Code examples and patterns
  - Implementation checklists
  - Debugging tactics
  - External references

### Why

User requested comprehensive analysis with all relevant files, symbols, and documents. The playbook and implementation guide were provided by user and needed to be saved as reference documentation.

### What worked

- Structured analysis documents covering:
  - BLE stack comparison (Bluedroid vs NimBLE)
  - Existing code patterns and APIs
  - Sensor options and I2C access patterns
  - Complete debugging playbook
  - Step-by-step implementation guide

- Documented key decisions:
  - Use Bluedroid (consistency with existing code)
  - Use SHT3x sensor (example exists, high accuracy)
  - Follow existing callback-based architecture

### What didn't work

- Still missing Cardputer I2C pin assignments (noted as research needed)
- ESP-IDF API version compatibility needs verification

### What I learned

- Comprehensive documentation upfront saves time during implementation
- Having both analysis and reference (playbook) documents provides different value:
  - Analysis: Understanding and decision-making
  - Reference: Copy/paste ready code and procedures

### What was tricky to build

- Organizing large amounts of information into coherent documents
- Balancing detail with readability
- Integrating user-provided playbook content with codebase analysis

### What warrants a second pair of eyes

- Technical accuracy of BLE API usage (verify against ESP-IDF v4.4.6 docs)
- Sensor selection rationale (SHT3x vs alternatives)
- Memory budget calculations

### What should be done in the future

1. **Relate files to documents** - Link key source files to analysis documents
2. **Update ticket index** - Add summary and related files
3. **Create implementation tasks** - Break down into actionable steps
4. **Verify hardware** - I2C pins, sensor availability
5. **Create minimal example** - Start with BLE GATT server + one characteristic

### Code review instructions

- Review analysis documents for technical accuracy
- Verify API usage matches ESP-IDF v4.4.6 documentation
- Check that implementation guide aligns with analysis findings

### Technical details

**Documents Created**:
- Analysis: BLE implementation patterns, sensor options
- Reference: Debugging playbook, implementation guide
- Diary: Research process tracking

**Key Decisions Documented**:
- Bluedroid over NimBLE (consistency)
- SHT3x sensor (example exists)
- Task-based architecture (non-blocking)

**Outstanding Research**:
- Cardputer I2C pin assignments
- ESP-IDF API compatibility verification

### What I'd do differently next time

- Relate files immediately after creating documents (don't wait)
- Create implementation tasks in parallel with analysis
- Verify hardware constraints earlier

## Step 3: Scaffold firmware + hit ESP-IDF 5.4.1 Bluedroid advertising link error

This step created the first concrete firmware project for ticket 0019 so we can iterate toward a working BLE GATT server + notify loop on Cardputer. The core bring-up code is in place (Bluedroid init, GATT attribute table, mock temperature updates, counters/logging), but the build currently fails at link time due to missing GAP advertising symbols.

**Commit (code):** N/A — build not yet green

### What I did

- Created a new ESP-IDF app under `esp32-s3-m5/0019-cardputer-ble-temp-logger/` using the same CMake/partition patterns as other Cardputer tutorials (ESP-IDF 5.4.1).
- Implemented a Bluedroid GATT server with:
  - Service `0xFFF0`
  - Temp characteristic `0xFFF1` (READ + NOTIFY) int16 centi-degC
  - Control characteristic `0xFFF2` (WRITE) uint16 notify period ms
- Added a mock temperature generator + periodic notify loop + heap/counter logging.
- Added a minimal Python `bleak` client script to scan/connect/read/notify.
- Attempted to build with:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd .../0019-cardputer-ble-temp-logger && idf.py set-target esp32s3 && idf.py build`

### Why

- We needed a real project location and compilation feedback loop to move from analysis to implementation.
- Starting with a mock sensor de-risks the BLE/GATT layer before touching I2C hardware.

### What worked

- The firmware compiles through the C compilation phase and builds most components.
- The GATT table + notify loop code compiles cleanly under ESP-IDF 5.4.1.

### What didn't work

- Link failure:

  - `undefined reference to esp_ble_gap_start_advertising`
  - `undefined reference to esp_ble_gap_config_adv_data`

### What I learned

- In ESP-IDF 5.4.1, some Bluedroid GAP advertising helpers appear to be gated behind BLE 4.2 feature support, while our project config currently enables BLE 5.0 features and disables BLE 4.2 features.
- Editing `sdkconfig.defaults` does not override an already-generated `sdkconfig`; config changes must be applied deliberately.

### What was tricky to build

- ESP-IDF BLE feature gating: compile success can mask link-time missing symbol issues when headers are present but implementation is excluded by config.

### What warrants a second pair of eyes

- The correct, forward-looking choice for advertising APIs in ESP-IDF 5.4.x (keep BLE 5.0 and use extended adv APIs vs enable BLE 4.2 helper APIs).
- The cleanest project-level approach to enforcing the intended BLE feature set without repeatedly hand-editing `sdkconfig`.

### What should be done in the future

- Resolve the advertising API mismatch by either:
  - enabling the Kconfig needed for the helper APIs, or
  - switching to the correct BLE 5.0 extended advertising APIs for Bluedroid.
- Once build is green: commit firmware code, then record a known-good host command sequence for `bleak` validation.

### Code review instructions

- Start in:
  - `esp32-s3-m5/0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c`
  - `esp32-s3-m5/0019-cardputer-ble-temp-logger/sdkconfig.defaults`
- Validate by building:
  - `idf.py build`

### Technical details

- Research query document created to resolve the link issue:
  - `reference/03-research-query--esp-idf-5-4-1-bluedroid-adv-gap-link-error.md`

### What I'd do differently next time

- Confirm ESP-IDF advertising API availability and feature gating (BLE 4.2 vs 5.0) before hardcoding the first advertising flow.

## Step 4: Confirm BLE 5.0 default disables legacy advertising APIs in ESP-IDF 5.4.1 (source inspection)

This step validated the root cause of the link error by reading the ESP-IDF 5.4.1 Bluedroid source directly. The outcome is a clear fork in approach: either disable BLE 5.0 in Kconfig to keep the legacy `esp_ble_gap_start_advertising()` / `esp_ble_gap_config_adv_data()` APIs, or keep BLE 5.0 and switch the firmware to the extended advertising APIs (`esp_ble_gap_ext_adv_*`).

**Commit (code):** N/A — decision pending

### What I did

- Read ESP-IDF 5.4.1 Bluedroid config and feature gating code under `/home/manuel/esp/esp-idf-5.4.1/`.

### What I found (why the link error happens)

- In `components/bt/host/bluedroid/Kconfig.in`, BLE 5.0 is **enabled by default** and the prompt explicitly warns BLE 4.2 + BLE 5.0 are mutually exclusive.
- In `components/bt/host/bluedroid/common/include/common/bt_target.h`, the macro `BLE_42_FEATURE_SUPPORT` is only enabled when either:
  - `UC_BT_BLE_42_FEATURES_SUPPORTED == TRUE`, or
  - BLE 5.0 support is disabled (`BLE_50_FEATURE_SUPPORT == FALSE`).
- The legacy advertising structs + helper APIs (including `esp_ble_gap_config_adv_data()` and `esp_ble_gap_start_advertising()`) are guarded by `#if (BLE_42_FEATURE_SUPPORT == TRUE)` in Bluedroid GAP API headers/source, so when BLE 5.0 is enabled and BLE 4.2 is disabled, these symbols are not built into the library — resulting in undefined references at link time.

### What warrants a second pair of eyes

- Whether we should prefer:
  - **Option A**: disable BLE 5.0 and use legacy advertising (simpler, matches many older examples), or
  - **Option B**: keep BLE 5.0 (default in 5.4.1) and use extended advertising APIs (more future-proof).

### What should be done in the future

- Decide Option A vs Option B, then implement and re-run `idf.py build` to get a green build and commit the firmware baseline.

## Step 5: Document BLE 4.2 vs 5.0 advertising APIs (cross-validated with intern research)

This step turned the linker root cause into a durable, ticket-local guide: a comparison between BLE 4.2 and BLE 5.0 *as implemented in ESP-IDF 5.4.1 Bluedroid*, and a practical “how to advertise” recipe for each. It also cross-validated the intern research writeup, keeping the good parts while correcting the few project-breaking details (notably a non-existent Kconfig symbol and some struct-field naming).

**Commit (docs):** N/A — documentation update

### What I did

- Wrote a new analysis doc focused on advertising API differences and exact API sequencing for legacy vs extended advertising in ESP-IDF 5.4.1.
- Cross-validated the intern’s `sources/research-ble-advertising-error.md` against:
  - ESP-IDF 5.4.1 Bluedroid Kconfig (`BT_BLE_42_FEATURES_SUPPORTED` vs `BT_BLE_50_FEATURES_SUPPORTED`)
  - `bt_target.h` feature gating (`BLE_42_FEATURE_SUPPORT` / `BLE_50_FEATURE_SUPPORT`)
  - in-tree examples that demonstrate both advertising flows.

### What worked

- Found authoritative, up-to-date (5.4.1) examples for both flows:
  - Legacy advertising: `advanced_https_ota` Bluedroid helper
  - Extended advertising: `ble_50/periodic_adv` and `ble50_throughput` server

### What didn't work

- Intern doc proposed `CONFIG_BT_BLE_42_ADV_EN=y`, but that symbol does not exist in the ESP-IDF 5.4.1 tree for Bluedroid; relying on it would stall bring-up.

### What I learned

- The most reliable “truth” for this problem is ESP-IDF’s own:
  - Kconfig text (mutual exclusivity)
  - header/source guards (what actually compiles)
  - working examples (what sequences/events are expected).

### What warrants a second pair of eyes

- Decide whether we should standardize this repo’s 0019 firmware on:
  - BLE 4.2 + legacy adv (fastest bring-up), or
  - BLE 5.0 + extended adv (align with 5.4.1 defaults).

## Step 6: Ship BLE 5.0 extended advertising + validate full E2E (firmware ↔ host) with tmux + btmon

This step completed the first working end-to-end baseline for the ticket: the Cardputer firmware advertises via **BLE 5.0 extended advertising**, exposes a custom GATT service for temperature streaming, and a Linux host can scan/connect/read/subscribe successfully. The key outcome is that we now have a reproducible “known-good” setup (including scripts and a tmux workflow) that we can stress-test and later swap from mock temperature to a real I2C sensor.

**Commit (code):** 1c57e6fb5aa9cce1f13a95721422d0417648b011 — "0019: add BLE5 temp logger (GATTS + ext adv)"

### What I did

- Implemented a BLE GATT server (Bluedroid) with:
  - service `0xFFF0`
  - temperature characteristic `0xFFF1` (READ + NOTIFY), encoded as int16 centi-degC little-endian
  - control characteristic `0xFFF2` (WRITE), encoded as uint16 notify period ms (0 disables loop)
- Switched advertising to **BLE 5.0 extended advertising** to match ESP-IDF 5.4.1 defaults and avoid legacy advertising APIs being gated out:
  - configured ext adv params + raw adv payload
  - started ext advertising after the correct GAP completion events
- Added two “pane scripts” to make the workflow reliable in tmux:
  - `tools/run_fw_flash_monitor.sh`: sources ESP-IDF 5.4.1 and runs `idf.py flash monitor` using the **IDF venv python** to avoid tmux/direnv PATH mismatches
  - `tools/run_host_ble_client.sh`: runs the host `bleak` client, normalizing argparse global flags (fixes `scan --timeout 5` vs `--timeout 5 scan`)
- Validated the whole loop in tmux:
  - firmware flash + serial monitor in one pane
  - host scan/connect/read/notify/control in the other pane
  - `sudo btmon` in a separate window to capture HCI traffic
- Documented the exact workflow in a playbook:
  - `playbook/02-e2e-test-cardputer-ble-temp-logger--tmux.md`

### Why

- ESP-IDF 5.4.1 Bluedroid gates legacy advertising APIs behind BLE 4.2 feature support; using BLE 5.0 means we must use the extended advertising APIs.
- A stable E2E workflow (scripts + playbook) avoids losing time on environment issues (tmux shells, direnv, python venv mismatch) while iterating on BLE + sensors.

### What worked

- Firmware compiles and links under ESP-IDF 5.4.1 (BLE 5.0 enabled).
- Device is discoverable as `CP_TEMP_LOGGER` and accepts connections.
- Host can:
  - scan and find the device
  - connect
  - read temperature
  - subscribe to notifications and receive periodic values
  - adjust notify period via the control characteristic
- `btmon` captures expected advertising and connection traffic for debugging.

### What didn't work

- In tmux, running `idf.py flash monitor` directly could fail with:
  - `No module named 'esp_idf_monitor'`
  This was caused by the wrong python environment being used inside the pane (PATH/direnv quirks).
- The first version of the host pane suggested:
  - `ble_client.py scan --timeout 5`
  but argparse defines `--timeout` as a global option, so it must appear before the subcommand.

### What I learned

- ESP-IDF tool stability in tmux comes from being explicit:
  - source the IDF export script, then run `IDF_PATH/tools/idf.py` using the **IDF python venv**, not whatever `idf.py` resolves to on PATH.
- For CLI design, global argparse flags should be documented in the correct order; wrapper scripts can normalize user-friendly ordering.

### What was tricky to build

- Extended advertising is callback/event sequenced; it’s easy to “start too early” unless you gate `ext_adv_start()` on:
  - `ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT`
  - `ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT`
- Ensuring the development ergonomics stay sane (tmux + monitor + btmon + host client) without accidental environment breakage.

### What warrants a second pair of eyes

- Validate the extended advertising payload and parameters are appropriate for common scanners (Android/iOS/Linux), and confirm we don’t need scan response data for our discovery UX.
- Review the temperature encoding contract (int16 centi-degC) and whether we should add an explicit descriptor/format char later.

### What should be done in the future

- Run stress scenarios (reconnect loops, long streaming) and confirm:
  - `notifies_fail` stays at 0
  - heap minimum free remains stable over time
- Add a small “log capture bundle” routine (save serial log + btmon log) for reproducible debugging artifacts.
- Proceed with hardware work (SHT3x I2C pins + driver) once BLE baseline is stable.

## Step 4: "Connected but keyboard not working" — ClassicBondedOnly blocks HID

After successfully pairing and connecting the keyboard (`bluetoothctl info` showed `Connected: yes`), typing on the keyboard did nothing. Investigation revealed BlueZ was rejecting HID profile connections because the device was paired but not bonded (`Bonded: no`), and `/etc/bluetooth/input.conf` has `ClassicBondedOnly=true` (default) which blocks HID from unbonded devices.

**Commit (docs):** N/A — Troubleshooting documentation

### What I did

- Verified keyboard state: `bluetoothctl info DC:2C:26:FD:03:B7` showed:
  - `Paired: yes`
  - `Bonded: no` ← **this was the problem**
  - `Connected: yes`
  - `UUID: Human Interface Device... (00001124-0000-1000-8000-00805f9b34fb)`
- Checked `/proc/bus/input/devices` — keyboard did not appear (no input device created)
- Checked BlueZ logs: `journalctl -u bluetooth` showed:
  - `profiles/input/device.c:hidp_add_connection() Rejected connection from !bonded device /org/bluez/hci0/dev_DC_2C_26_FD_03_B7`
- Found `/etc/bluetooth/input.conf` contains `#ClassicBondedOnly=true` (commented, so defaults to true)
- Documented fix: disable `ClassicBondedOnly` or force bonding

### Why

This is a common gotcha: BlueZ distinguishes between "paired" (keys exchanged) and "bonded" (keys persisted). The HID profile can be configured to reject connections from devices that are paired but not bonded, which is a security feature but can block legitimate keyboards.

### What worked

- Identifying the exact log message (`Rejected connection from !bonded device`)
- Finding the config file (`/etc/bluetooth/input.conf`) and the setting (`ClassicBondedOnly`)
- Documenting the fix in the playbook

### What didn't work

- Couldn't run `sudo` commands directly to test the fix (requires password)
- Device still shows `Bonded: no` even after pairing (may require additional steps or different pairing method)

### What I learned

1. **Paired ≠ Bonded**: BlueZ has two states:
   - **Paired**: keys exchanged during pairing session
   - **Bonded**: keys persisted to `/var/lib/bluetooth/.../` and device can reconnect automatically

2. **ClassicBondedOnly**: Security setting that blocks HID connections from unbonded devices. Defaults to `true` for security, but can block legitimate keyboards.

3. **HID profile rejection is silent**: The keyboard connects at the GATT level (`Connected: yes`) but HID profile connection is rejected, so no `/dev/input/eventX` device is created.

### What was tricky to build

- Understanding why `Connected: yes` doesn't mean the keyboard works (HID profile is separate from base connection)
- Finding the right config file (`input.conf` not `main.conf`)
- Distinguishing between pairing/bonding/trusting states

### What warrants a second pair of eyes

- Whether forcing bonding (vs disabling ClassicBondedOnly) is the better approach
- Whether some keyboards simply can't achieve `Bonded: yes` state (legacy pairing limitations)

### What should be done in the future

1. **Test the fix**: Run `sudo sed -i 's/^#ClassicBondedOnly=true/ClassicBondedOnly=false/' /etc/bluetooth/input.conf && sudo systemctl restart bluetooth` and verify keyboard works
2. **Document bonding vs pairing**: Add to playbook explaining when `Bonded: no` is acceptable vs when it blocks functionality
3. **Add verification steps**: Include commands to check `/proc/bus/input/devices` and test actual keyboard input

### Code review instructions

- Review playbook troubleshooting section: `playbook/01-linux-scan-pair-trust-and-connect-a-bluetooth-keyboard-bluez-bluetoothctl.md`
- Verify ClassicBondedOnly fix is correct (check BlueZ docs for current behavior)
- Test on a real keyboard to confirm fix works

### Technical details

**Key Files**:
- `/etc/bluetooth/input.conf` — HID profile configuration
- `/var/lib/bluetooth/<adapter>/<device>/` — Bonding keys storage (if bonded)

**Key Log Messages**:
- `hidp_add_connection() Rejected connection from !bonded device` — HID profile rejection
- `ioctl_is_connected() Can't get HIDP connection info` — HIDP connection check failure

**Fix**:
```bash
# Disable ClassicBondedOnly (allows HID from paired but unbonded devices)
sudo sed -i 's/^#ClassicBondedOnly=true/ClassicBondedOnly=false/' /etc/bluetooth/input.conf
sudo systemctl restart bluetooth
bluetoothctl disconnect "$MAC" && bluetoothctl connect "$MAC"
```

### What I'd do differently next time

- Check `Bonded: yes/no` status immediately after pairing (not just `Paired: yes`)
- Look for HID rejection messages in logs right after connecting
- Test keyboard input immediately after connection (don't assume `Connected: yes` means it works)

## Step 5: Understanding bonding - why some keyboards never achieve "Bonded: yes"

After successfully pairing and connecting the keyboard (no PIN prompt appeared), it still showed `Bonded: no` and no bonding directory was created. This revealed that **some legacy keyboards pair successfully but never complete bonding**, which is normal behavior and requires disabling `ClassicBondedOnly` as a workaround.

**Commit (docs):** N/A — Documentation update

### What I did

- Verified keyboard state after successful connection:
  - `Paired: yes` ✓
  - `Bonded: no` ✗ (no bonding directory exists)
  - `Connected: yes` ✓
  - `LegacyPairing: yes` ✓
- Checked `/var/lib/bluetooth/10:A5:1D:00:C6:6F/DC:2C:26:FD:03:B7/` — directory doesn't exist
- Reviewed BlueZ logs — still shows `Rejected connection from !bonded device`
- Added detailed bonding explanation to playbook

### Why

Understanding the difference between pairing and bonding is critical for troubleshooting Bluetooth keyboards. Many users (and documentation) conflate these terms, leading to confusion when `Paired: yes` but `Bonded: no` blocks functionality.

### What worked

- Identifying that no PIN prompt appeared (keyboard uses "Just Works" pairing)
- Confirming no bonding directory exists (keys weren't persisted)
- Documenting that this is **normal behavior** for some legacy keyboards

### What didn't work

- Attempting to force bonding — no mechanism found to force key persistence
- PIN entry — keyboard doesn't require it (uses "Just Works" pairing)

### What I learned

1. **Pairing vs Bonding**:
   - **Pairing**: Keys exchanged in memory during pairing session (`Paired: yes`)
   - **Bonding**: Keys persisted to `/var/lib/bluetooth/.../` (`Bonded: yes`)
   - Bonding allows automatic reconnection without re-pairing

2. **Legacy keyboards and bonding**:
   - Some keyboards use "Just Works" pairing (no PIN required)
   - These keyboards may pair successfully but **never achieve `Bonded: yes`**
   - This is **normal behavior** - not a bug or misconfiguration
   - The practical solution is to disable `ClassicBondedOnly` if HID is blocked

3. **When bonding matters**:
   - `ClassicBondedOnly=true` (default) blocks HID from unbonded devices
   - If bonding doesn't complete, you must either:
     - Disable `ClassicBondedOnly` (allows HID from paired but unbonded devices)
     - Accept that re-pairing may be needed after reboots

### What was tricky to build

- Understanding why some keyboards never bond (legacy pairing limitations)
- Finding authoritative sources explaining when bonding completes vs when it doesn't
- Distinguishing between "this is broken" vs "this is normal for legacy devices"

### What warrants a second pair of eyes

- Whether there's a way to force bonding for legacy keyboards (doesn't appear to be)
- Whether disabling `ClassicBondedOnly` has security implications (documented as security feature)

### What should be done in the future

1. **Test ClassicBondedOnly fix**: Verify keyboard works after disabling it
2. **Document this pattern**: Add to playbook that "some keyboards never bond" is normal
3. **Consider security implications**: Document whether disabling ClassicBondedOnly is acceptable

### Code review instructions

- Review playbook bonding explanation section
- Verify that "some keyboards never achieve Bonded: yes" is accurately documented
- Confirm ClassicBondedOnly workaround is clearly explained

### Technical details

**Key Insight**: 
- Pairing ≠ Bonding
- Some legacy keyboards pair but never bond
- This is **normal**, not a bug

**Bonding Directory**:
- Location: `/var/lib/bluetooth/<adapter>/<device>/`
- Contains: `info`, `attributes`, and possibly link keys
- Created only when bonding completes (`Bonded: yes`)

**Verification**:
```bash
# Check if bonding completed
ls -la /var/lib/bluetooth/10:A5:1D:00:C6:6F/DC:2C:26:FD:03:B7/
# If directory doesn't exist, bonding didn't complete

# Check device state
bluetoothctl info DC:2C:26:FD:03:B7 | grep Bonded
# Shows: Bonded: no (for legacy keyboards that don't complete bonding)
```

**Practical Solution**:
- Disable `ClassicBondedOnly` if keyboard pairs but never bonds
- This allows HID to work from paired but unbonded devices
- Acceptable workaround for legacy keyboards

### What I'd do differently next time

- Check for bonding directory immediately after pairing (not just `Bonded: yes/no` status)
- Document that "no PIN prompt" + "Bonded: no" is normal for some keyboards
- Explain bonding vs pairing distinction upfront in playbook

## Step 3: Linux Bluetooth keyboard pairing + connection playbook (validated commands)

This step captured a real-world pairing session on Linux with a Bluetooth keyboard in pairing mode. The main outcomes were: (1) we hit a host-side “adapter not ready” issue caused by rfkill + Powered off, (2) we discovered that agent handling matters a lot for keyboards (legacy pairing needs a PIN/passkey typed on the keyboard), and (3) we wrote a playbook that documents the stable command sequence and common failure modes.

**Commit (docs):** N/A

### What I did
- Observed initial failure: `bluetoothctl scan on` returning `org.bluez.Error.NotReady`
- Verified root cause: `rfkill` showed soft block and `bluetoothctl show` reported `Powered: no`
- Unblocked and powered the adapter; verified `Powered: yes`
- Scanned and discovered the keyboard MAC and name:
  - `DC:2C:26:FD:03:B7` — `ZAGG Slim Book`
- Attempted pairing in non-interactive mode and saw agent-related failures (no agent registered / agent object registration failing)
- Switched to `bluetoothctl --agent KeyboardDisplay` to ensure an agent exists for PIN/passkey prompts
- Wrote the playbook: `playbook/01-linux-scan-pair-trust-and-connect-a-bluetooth-keyboard-bluez-bluetoothctl.md`

### Why
- BLE projects often need a reliable host-side workflow (scan/pair/connect) to validate peripherals and to debug device-side issues.
- Keyboards are a special case because pairing frequently involves a “type this PIN on the keyboard and press Enter” interaction.

### What worked
- `rfkill unblock bluetooth` reliably cleared the “NotReady” scan failures once the adapter was powered.
- `bluetoothctl --timeout 40 scan on` produced consistent discovery results.
- `bluetoothctl --agent KeyboardDisplay ...` is the right shape for keyboard pairing because it provides an agent for PIN/passkey prompts.

### What didn't work
- Non-interactive `bluetoothctl` sessions sometimes failed to register an agent:
  - “No agent is registered”
  - “Failed to register agent object”
- After an apparent pairing success, the device state sometimes flipped back to `Paired: no` and HID input could be rejected as “!bonded”.

### What I learned
- If BlueZ reports `Powered: no` or rfkill is blocking, you’ll see `org.bluez.Error.NotReady` on scan—fix is to unblock + power on.
- For keyboards, pairing often requires **typing a PIN/passkey on the keyboard**; if you don’t do it, pairing can “half succeed” and then not persist/bond.
- BlueZ may evict “temporary” devices quickly (`TemporaryTimeout`), so pairing needs to happen right after discovery.

### What was tricky to build
- Avoiding “interactive hang” while still capturing a real pairing flow: we used `--timeout` and documented the manual steps explicitly.

### What warrants a second pair of eyes
- If the keyboard continues to “pair but not bond”, review host policy/config and logs (BlueZ input profile vs bonding persistence) and confirm whether the keyboard requires a specific key sequence / PIN workflow.

### What should be done in the future
- Run the playbook fully in an interactive terminal session (so PIN/passkey entry is handled cleanly) and confirm a stable `Bonded: yes` state.
- If problems persist, capture:
  - `journalctl -u bluetooth -f`
  - `sudo btmon`
  and compare a “good” vs “bad” attempt.

### Code review instructions
- Read: `playbook/01-linux-scan-pair-trust-and-connect-a-bluetooth-keyboard-bluez-bluetoothctl.md`

### Technical details
- Relevant config files inspected:
  - `/etc/bluetooth/main.conf` (TemporaryTimeout default 30s)
  - `/etc/bluetooth/input.conf` (ClassicBondedOnly documented; default security behavior is to require bonding for classic HID)
