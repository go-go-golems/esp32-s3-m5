---
Title: Investigation diary
Ticket: ATOMS3R-M12-NATIVE-QUICKJS
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - psram
    - repl
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0103-atoms3r-m12-native-quickjs/main/app_main.cpp
      Note: AtomS3R M12 QuickJS service startup and PSRAM baseline logging
    - Path: 0103-atoms3r-m12-native-quickjs/sdkconfig.defaults
      Note: ESP32-S3 USB Serial/JTAG and PSRAM configuration
    - Path: ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/design-doc/01-atoms3r-m12-native-quickjs-analysis-design-and-implementation-guide.md
      Note: Primary intern-facing design guide
ExternalSources: []
Summary: Chronological diary for the AtomS3R M12 native QuickJS with PSRAM ticket.
LastUpdated: 2026-06-25T22:30:00-07:00
WhatFor: Use to resume AtomS3R M12 QuickJS bring-up and hardware validation.
WhenToUse: Read before flashing 0103, changing serial target, changing PSRAM settings, or extending QuickJS APIs on AtomS3R.
---

# Diary

## Goal

Capture the design, implementation, validation, failures, and follow-up work for running native upstream QuickJS on the AtomS3R M12 / ESP32-S3 with PSRAM. The first target is a USB Serial/JTAG console firmware that proves the QuickJS service and memory profile before adding WiFi, storage, or display APIs.

## Step 1: Create the AtomS3R M12 QuickJS ticket and build the first firmware scaffold

Started a new board-specific QuickJS port for the AtomS3R M12. The workstation also has an ESP32-P4 attached, so the first implementation decision was to identify the serial devices by USB identity and avoid numeric `/dev/ttyACM*` assumptions. The AtomS3R M12 is the Espressif USB Serial/JTAG device; the ESP32-P4 is the CH343 bridge.

The initial firmware target, `0103-atoms3r-m12-native-quickjs`, reuses the proven native QuickJS components from the ESP32-P4 work. It starts `qjs_service` with a conservative 1 MiB QuickJS memory cap, logs PSRAM/heap baselines before and after service startup, and exposes the same `js status`, `js eval`, `js reset`, `js gc`, and `js bench` console commands through USB Serial/JTAG.

### Prompt Context

**User prompt (verbatim):** "I connected a atoms3r M12 (there's also a ESP32P4 connected, so don't mix them up and use device-id to find the serial interface.

I want you to stasrt a new ticket and a new firmware to build and run quickjs on our S3r M12 using PSRAM.

Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a new docmgr ticket and new ESP-IDF firmware target for native QuickJS on AtomS3R M12 / ESP32-S3 with PSRAM, using by-id serial identification to avoid flashing the connected ESP32-P4, and write/upload a detailed intern-facing design guide.

**Inferred user intent:** Validate that the smaller ESP32-S3R8-class AtomS3R M12 can run the native QuickJS service with PSRAM headroom, then use that as a foundation for later WiFi/storage/device API work.

**Commit (code):** 3bfd6cc8670cb9ae756e706dbc065467c56e4d04 — "0103: add AtomS3R M12 native QuickJS scaffold"

### What I did

- Identified connected serial devices:
  - AtomS3R M12: `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00 -> /dev/ttyACM1`.
  - ESP32-P4: `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 -> /dev/ttyACM0`.
- Created docmgr ticket `ATOMS3R-M12-NATIVE-QUICKJS` with topics `atoms3r`, `esp32s3`, `quickjs`, `javascript`, `firmware`, `psram`, and `repl`.
- Added the design guide `design-doc/01-atoms3r-m12-native-quickjs-analysis-design-and-implementation-guide.md`.
- Replaced `tasks.md` with phased implementation tasks.
- Created firmware directory `0103-atoms3r-m12-native-quickjs` with:
  - `CMakeLists.txt`
  - `README.md`
  - `sdkconfig.defaults`
  - `partitions.csv`
  - `main/CMakeLists.txt`
  - `main/app_main.cpp`
  - `main/js_command.cpp`
  - `main/js_command.h`
- Reused existing components:
  - `components/quickjs_native`
  - `components/qjs_service`
- Configured the first AtomS3R build for:
  - ESP32-S3 target.
  - USB Serial/JTAG console.
  - 8 MB flash.
  - Octal PSRAM at 80 MHz.
  - `CONFIG_SPIRAM_USE_MALLOC=y`.
  - 4 MiB app partition plus 3 MiB reserved data partition.
  - 1 MiB QuickJS memory limit.
  - 64 KiB QuickJS stack limit.
  - 32 KiB-class QuickJS owner task stack.
- Built the firmware successfully with ESP-IDF 5.4.2:

```text
0103-atoms3r-m12-native-quickjs.bin binary size 0xb4d00 bytes. Smallest app partition is 0x400000 bytes. 0x34b300 bytes (82%) free.
```

### Why

- The ESP32-P4 native QuickJS firmware proves the service architecture, but the AtomS3R M12 has a different memory envelope and console path.
- A separate `0103` target keeps the S3R port isolated from the active ESP32-P4 visual REPL work and from the colleague's separate ticket/firmware.
- Starting console-only avoids mixing QuickJS memory bring-up with display, WiFi, or filesystem variables.

### What worked

- Device identity was unambiguous via `/dev/serial/by-id`.
- The new ESP32-S3 firmware scaffold compiled successfully.
- The reusable QuickJS components did not require source changes for the ESP32-S3 build.
- The app partition has substantial headroom: 82% free in the 4 MiB factory partition.

### What didn't work

- No hardware flash or console smoke has been run yet. The firmware is built but not validated on the AtomS3R M12 device.
- The QuickJS component still emits known upstream pointer-type warnings. They are already locally tolerated in `components/quickjs_native` and did not fail the build.

### What I learned

- The existing `quickjs_native` and `qjs_service` boundaries are portable enough to compile for ESP32-S3 without changing the engine or service code.
- The largest immediate risk is not compile-time portability; it is validating the exact PSRAM mode and memory headroom on the AtomS3R M12 hardware.
- By-id serial paths are mandatory in this session because both the AtomS3R and ESP32-P4 expose `/dev/ttyACM*` devices.

### What was tricky to build

- The serial target had to be treated as part of the design, not as an afterthought. Accidentally flashing `/dev/ttyACM0` would hit the ESP32-P4 CH343 bridge, not the AtomS3R.
- The PSRAM configuration needs to be conservative for first boot. The design uses Octal 80 MHz before attempting faster modes.
- The firmware reserves storage partition space now but avoids mounting or using it until QuickJS memory behavior is characterized.

### What warrants a second pair of eyes

- Review `sdkconfig.defaults` against the exact AtomS3R M12 PSRAM electrical configuration; if the board differs from the expected ESP32-S3R8/Octal profile, boot may fail or PSRAM may not initialize.
- Review whether `REQUIRES` pulls in more ESP-IDF networking components than necessary for milestone 1; the binary still fits comfortably, but dependency reduction may help later.
- Review `cfg.task_stack_words = 32768`; it follows the proven P4 service but should be measured on S3 after hardware smoke.

### What should be done in the future

- Flash and monitor from tmux using only the AtomS3R by-id path.
- Capture boot logs to `/tmp/0103-atoms3r-m12-native-quickjs-smoke.log`.
- Run `js status`, `js eval "print(1+2)"`, exception, completion-value, reset, and bench tests.
- Record PSRAM size, internal heap, 8-bit heap, PSRAM free, QuickJS memory used, atom count, and benchmark timings.

### Code review instructions

- Start in `0103-atoms3r-m12-native-quickjs/sdkconfig.defaults` for board configuration.
- Review `0103-atoms3r-m12-native-quickjs/main/app_main.cpp` for service startup, memory logging, and USB Serial/JTAG console creation.
- Review `0103-atoms3r-m12-native-quickjs/main/js_command.cpp` for command parity with the proven ESP32-P4 native console.
- Validate build with:

```bash
source /home/manuel/esp/esp-idf-5.4.2/export.sh
cd 0103-atoms3r-m12-native-quickjs
idf.py build
```

- Validate hardware with a tmux monitor session using:

```bash
PORT=/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00
idf.py -p "$PORT" flash monitor
```

### Technical details

- ESP32-P4 by-id path to avoid: `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`.
- AtomS3R M12 by-id path to use: `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00`.
- Firmware target: `0103-atoms3r-m12-native-quickjs`.
- Build artifact: `0103-atoms3r-m12-native-quickjs/build/0103-atoms3r-m12-native-quickjs.bin`.
- Initial binary size: `0xb4d00`.
- App partition: `4 MiB`.
- Free in app partition after first build: `0x34b300` / 82%.
