---
Title: Diary
Ticket: ATOMLITE-PRINTER-PROV
Status: active
Topics:
    - esp-idf
    - firmware
    - ble
    - provisioning
    - m5stack
    - atom-lite
    - thermal-printer
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/app_button.c
      Note: Factory-reset button implementation from Step 2
    - Path: esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/app_printer.c
      Note: Printer UART implementation from Step 2
    - Path: esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/main.c
      Note: Implemented and built in Step 2
ExternalSources: []
Summary: Chronological implementation diary for the ATOM Lite ESP-IDF BLE provisioning firmware.
LastUpdated: 2026-04-23T16:20:00-04:00
WhatFor: Use to resume or review the ATOM Lite M5 printer ESP-IDF provisioning implementation.
WhenToUse: Read before continuing firmware, flash, provisioning, or printer validation work for ATOMLITE-PRINTER-PROV.
---


# Diary

## Goal

This diary records the implementation of a fresh ESP-IDF BLE provisioning firmware for the M5Stack ATOM Thermal Printer Kit controller: the ATOM Lite / ESP32-PICO-D4.

## Step 1: Correct the target and create the docmgr ticket

The earlier implementation context was for ATOMS3R, which is not the controller in the M5Stack ATOM Thermal Printer Kit. I corrected the scope to ATOM Lite / ESP32-PICO-D4, created a docmgr ticket, and kept the actual firmware source under the requested `0092-m5-printer-esp-idf-provision` directory.

### Prompt Context

**User prompt (verbatim):** "ok so I think I made a mistake prompting: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/22/ATOMS3R-ESPPROV--create-atoms3r-esp-idf-ble-provisioning-firmware/sources/atoms3r-esp-idf 

which is for a atoms3r not a atom-lite. Create a new ticket to build a atomlite esp provisionoing, and implement the firmware there. Create it in a 0092-m5-printer-esp-idf-provision directory. Add implementation guide and tasks to the ticket. Then work task by task, commit at approprivate intervals, keep a detailed diary."

**Assistant interpretation:** Create a new structured ticket and firmware workspace for ATOM Lite, not ATOMS3R, then implement native ESP-IDF BLE provisioning firmware in that new 0092 folder.

**Inferred user intent:** The user wants a clean, reviewable ESP-IDF project targeting the actual M5 printer hardware and wants ticket discipline, tasks, commits, and a diary so the work can continue safely.

### What I did

- Read the `docmgr`, `diary`, and `git-commit-instructions` skills.
- Created docmgr ticket `ATOMLITE-PRINTER-PROV`.
- Added docmgr documents:
  - `design-doc/01-implementation-guide.md`
  - `reference/01-diary.md`
- Added docmgr tasks for scope, scaffold, provisioning, printer UART, button reset, build validation, and flash/monitor documentation.
- Created the requested 0092 directory as the firmware home:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision`

### Why

The ATOM Lite uses ESP32-PICO-D4 and FTDI UART, while ATOMS3R uses ESP32-S3. Reusing the ATOMS3R project would risk wrong target settings, wrong USB assumptions, and wrong peripheral assumptions.

### What worked

- `docmgr ticket create-ticket` created the structured ticket workspace.
- `docmgr doc add` created the implementation guide and diary.
- `docmgr task add` created task bookkeeping.

### What didn't work

- I initially started writing ad-hoc docs directly in 0092 before switching fully to docmgr after the user corrected the workflow. Those local docs were treated as scratch and the docmgr ticket is now the authoritative task/guide/diary location.

### What I learned

For this repo, docmgr ticket documents live under `esp32-s3-m5/ttmp/...`, while implementation source can live elsewhere and should be related to the ticket with `docmgr doc relate`.

### What was tricky to build

The tricky part was reconciling two requirements: use docmgr for ticket/work tracking, but keep firmware in the 0092 folder. The solution is to use docmgr as the authoritative ticket record and relate the 0092 firmware files into that ticket.

### What warrants a second pair of eyes

Review whether the 0092 local `index.md` should remain as a lightweight source-directory README or be reduced further now that docmgr owns the implementation guide and tasks.

### What should be done in the future

Keep all task/changelog/diary updates in docmgr. Keep only source-adjacent README material in 0092.

### Code review instructions

- Start with the docmgr ticket index and implementation guide.
- Verify related files point at the 0092 firmware source.

### Technical details

Docmgr ticket path:

```text
esp32-s3-m5/ttmp/2026/04/23/ATOMLITE-PRINTER-PROV--atom-lite-esp-idf-ble-provisioning-firmware-for-m5-printer
```

Firmware path:

```text
esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov
```

## Step 2: Implement and build the ATOM Lite ESP-IDF firmware

I implemented a standalone ESP-IDF project under the requested 0092 folder. The firmware targets `esp32`, uses BLE provisioning through Espressif's provisioning manager, initializes the ATOM printer UART, and implements a five-second button-hold NVS erase/reboot factory reset.

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Work task-by-task after creating the ticket: scaffold, implement, build, document, and record details.

**Inferred user intent:** The user wants a working native ESP-IDF firmware that can be flashed and monitored on the ATOM Lite via `/dev/ttyUSB0`.

**Commit (code):** 2b74d824e25c9ab59ecaf8cab7bfcdb6a14589e7 — "Add ATOM Lite printer ESP-IDF provisioning firmware"

### What I did

- Wrote ESP-IDF project files:
  - `source/atomlite-printer-prov/CMakeLists.txt`
  - `source/atomlite-printer-prov/sdkconfig.defaults`
  - `source/atomlite-printer-prov/partitions.csv`
  - `source/atomlite-printer-prov/main/CMakeLists.txt`
- Wrote application source:
  - `main/main.c` — app lifecycle, event handlers, provisioning, WiFi status receipt, reset task.
  - `main/app_button.c/.h` — GPIO39 active-low hold detection.
  - `main/app_printer.c/.h` — UART2 printer setup and text/status receipt helpers.
- Built with:

```bash
cd esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc >/tmp/atomlite-idf-export.log
idf.py set-target esp32
idf.py build
```

### Why

The firmware needs to replace the current Arduino/M5 printer firmware with an ESP-IDF-native app that supports iPhone-friendly BLE WiFi provisioning while preserving the printer UART path.

### What worked

The build completed successfully with ESP-IDF 5.4.1:

```text
atomlite-printer-prov.bin binary size 0x109840 bytes. Smallest app partition is 0x180000 bytes. 0x767c0 bytes (31%) free.
Project build complete.
```

### What didn't work

No compile failures occurred. Build output was long because it was the first full ESP-IDF build for this project.

### What I learned

- ESP-IDF's provisioning example for ESP32 uses NimBLE with `CONFIG_BT_NIMBLE_ENABLED=y` and `WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM`.
- The BLE/WiFi provisioning binary needs a custom partition table because the app is larger than a minimal factory partition.
- ATOM Lite should use UART console settings, not USB Serial/JTAG settings.

### What was tricky to build

The main subtlety was avoiding a redundant custom NVS credential store. `wifi_prov_mgr` already uses the WiFi NVS namespace and can report provisioning state with `wifi_prov_mgr_is_provisioned()`. The firmware therefore lets the provisioning manager own credentials and only erases NVS on factory reset.

Another subtlety was serial ownership: UART0 is used for `idf.py monitor`, while UART2 is used for the thermal printer. The firmware deliberately avoids reusing UART0 for printer traffic.

### What warrants a second pair of eyes

- The fixed development PoP `12345678` is acceptable for bring-up but not production.
- `app_printer_init()` currently installs UART2 unconditionally and uses no CTS flow control. Large bitmap printing may need CTS GPIO19 support.
- `WIFI_EVENT_STA_DISCONNECTED` always reconnects; this is simple but may be noisy if credentials are wrong after provisioning.

### What should be done in the future

- Flash and validate on hardware.
- Verify the ESP BLE Provisioning phone app sees the `M5PRN_XXXXXX` service.
- Verify the printer receipt output after WiFi connection.
- Add SK6812/RMT LED status after the basic provisioning and printing path is proven.

### Code review instructions

Review these files first:

```text
0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/main.c
0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/app_printer.c
0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/sdkconfig.defaults
0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/partitions.csv
```

Validate with:

```bash
cd esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
idf.py set-target esp32
idf.py build
```

### Technical details

Flash and monitor command for hardware validation:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Expected provisioning log shape:

```text
Provision with Espressif's 'ESP BLE Provisioning' app:
  Transport : BLE
  Device    : M5PRN_XXXXXX
  Security  : Security 1
  PoP       : 12345678
```

## Step 3: Switch validation docs and build flow to repo `.envrc` / ESP-IDF 5.4.1

The repository already has an `esp32-s3-m5/.envrc` that sources ESP-IDF 5.4.1. I updated the ticket and source-adjacent documentation to use that `.envrc` instead of directly sourcing ESP-IDF 5.3.4, then rebuilt the firmware after a full clean with the 5.4.1 environment.

### Prompt Context

**User prompt (verbatim):** "use the .envrc in /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5 which:

source ~/esp/esp-idf-5.4.1/export.sh"

**Assistant interpretation:** Update the project to rely on the repository-local `.envrc` and validate against ESP-IDF 5.4.1 rather than the previously used 5.3.4 install.

**Inferred user intent:** Keep build instructions consistent with the repository's standard ESP-IDF environment and avoid split-version confusion.

**Commit (docs):** 4c17231c34d3dd09af5504c7f99785fca6227d2c — "Use repo ESP-IDF 5.4.1 env for ATOM Lite provisioning"

### What I did

- Read `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc` and confirmed it contains:

```bash
source ~/esp/esp-idf-5.4.1/export.sh
```

- Replaced explicit `esp-idf-5.3.4/export.sh` commands in:
  - `0092-m5-printer-esp-idf-provision/index.md`
  - `0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/README.md`
  - docmgr ticket `index.md`
  - docmgr implementation guide
  - this diary
- Ran a clean rebuild using the repo `.envrc`:

```bash
cd esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc >/tmp/atomlite-idf541-export.log
idf.py fullclean
idf.py set-target esp32
idf.py build
```

### Why

The repo-level `.envrc` is the canonical environment setup for work under `esp32-s3-m5`. Using it keeps manual commands, docs, and future agent work aligned on ESP-IDF 5.4.1.

### What worked

The ESP-IDF 5.4.1 rebuild succeeded:

```text
atomlite-printer-prov.bin binary size 0x1093f0 bytes. Smallest app partition is 0x180000 bytes. 0x76c10 bytes (31%) free.
Project build complete.
```

### What didn't work

No build failure occurred. ESP-IDF emitted one Kconfig warning during link generation:

```text
warning: config BTDM_CTRL_CONTROLLER_DEBUG_MODE_1 (defined at /home/manuel/esp/esp-idf-5.4.1/components/bt/controller/esp32/Kconfig.in:537) has a "visible if" option, which is not supported for configs
```

The warning came from ESP-IDF's Bluetooth Kconfig and did not stop the build.

### What I learned

The firmware remains compatible with ESP-IDF 5.4.1 using the same source and `sdkconfig.defaults`. The binary is slightly smaller under 5.4.1 than the earlier 5.3.4 build recorded in Step 2.

### What was tricky to build

The build directory had previously been generated under ESP-IDF 5.3.4, so I ran `idf.py fullclean` before rebuilding with 5.4.1. This avoids stale CMake cache paths pointing at the old ESP-IDF installation.

### What warrants a second pair of eyes

Check whether the Bluetooth Kconfig warning is acceptable for this repository's CI/noise standards. It appears benign, but it may be worth tracking if future builds run with warnings-as-errors wrappers.

### What should be done in the future

Use the repo `.envrc` for all 0092 commands, including hardware flash/monitor validation.

### Code review instructions

Review the command snippets in:

```text
0092-m5-printer-esp-idf-provision/index.md
0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/README.md
ttmp/2026/04/23/ATOMLITE-PRINTER-PROV--atom-lite-esp-idf-ble-provisioning-firmware-for-m5-printer/design-doc/01-implementation-guide.md
```

Validate with:

```bash
cd esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
idf.py build
```

### Technical details

The `.envrc` is intentionally sourced from the repo root path rather than relying on shell-specific direnv activation, making the command copy/paste safe in plain bash sessions.
