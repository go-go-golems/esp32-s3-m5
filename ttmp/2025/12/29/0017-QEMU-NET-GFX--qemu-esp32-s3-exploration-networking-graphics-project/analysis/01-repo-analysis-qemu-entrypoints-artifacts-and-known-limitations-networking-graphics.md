---
Title: 'Repo analysis: QEMU entrypoints, artifacts, and known limitations (networking + graphics)'
Ticket: 0017-QEMU-NET-GFX
Status: active
Topics:
    - esp-idf
    - esp32s3
    - qemu
    - networking
    - graphics
    - emulation
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/imports/qemu_storage_repl.txt
      Note: Baseline log; confirms port 5555 + SPIFFS format behavior
    - Path: esp32-s3-m5/ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/analysis/01-bug-report-qemu-monitor-input-not-delivered-to-uart-repl.md
      Note: Details on idf_monitor input not reaching UART RX
    - Path: esp32-s3-m5/ttmp/2025/12/29/0016-SPIFFS-AUTOLOAD--bug-spiffs-first-boot-format-autoload-js-parse-errors/analysis/01-bug-report-spiffs-mount-format-autoload-js-parse-errors.md
      Note: Details on filesystem/partition quirks under QEMU
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T14:07:24.954826652-05:00
WhatFor: ""
WhenToUse: ""
---


# Repo analysis: QEMU entrypoints, artifacts, and known limitations (networking + graphics)

## Purpose

This document is a repo-specific orientation for “how we currently run ESP32-S3 firmware under QEMU”, what artifacts exist, and what sharp edges we already hit. It’s meant to bootstrap the **new QEMU networking + graphics exploration project** with as little re-derivation as possible.

## Existing QEMU entrypoints in this repo

### Baseline QEMU-friendly firmware project (good starting point)

- Project: `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl`
- Run command (via wrapper): `./build.sh qemu monitor`
- What `idf.py qemu monitor` typically does:
  - builds the project
  - generates `build/qemu_flash.bin`
  - runs QEMU in the background with `-serial tcp::5555,server`
  - attaches `idf_monitor` to `socket://localhost:5555` at 115200 baud

### QEMU artifacts and references we already have

- `esp32-s3-m5/imports/qemu_storage_repl.txt`
  - A captured QEMU boot log (useful as “expected output” reference).
- `esp32-s3-m5/imports/test_storage_repl.py`
  - A simple TCP client that connects to `localhost:5555` and sends commands (useful to bypass `idf_monitor` for RX tests).

## Known issues already observed (general QEMU learnings)

### 1) Tool install + environment drift

Symptom:

- Running `idf.py qemu monitor` errors with:
  - `qemu-system-xtensa is not installed`

Even after:

- `python "$IDF_PATH/tools/idf_tools.py" install qemu-xtensa`

Most likely cause (seen repeatedly):

- Your shell already has `ESP_IDF_VERSION` set, and wrapper scripts skip sourcing `export.sh`, so `PATH` doesn’t include the freshly installed QEMU tool.

Practical fix:

- Use a fresh shell, or run:
  - `unset ESP_IDF_VERSION`
  - then re-run the wrapper.

### 2) Partition table mismatch leads to missing filesystem partitions

Symptom:

- SPIFFS (or other FS) fails to mount because the expected partition isn’t present in the flashed image.

Typical root cause:

- `sdkconfig` is configured for `CONFIG_PARTITION_TABLE_SINGLE_APP=y`, so `partitions.csv` in the project is ignored and QEMU runs with a different partition table.

Fix:

- Switch to project-local custom partition table:
  - `CONFIG_PARTITION_TABLE_CUSTOM=y`
  - `CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"`

### 3) UART RX / interactive input can be unreliable

We have an open investigation that suggests:

- TX works (boot logs print fine),
- but interactive input may not reach the UART consumer that the app reads from.

Reference ticket:

- `0015-QEMU-REPL-INPUT — Bug: QEMU idf_monitor cannot send input ...`

Workaround/diagnostic:

- Bypass `idf_monitor` and send bytes directly to `localhost:5555` using the repo’s `imports/test_storage_repl.py`.

## Networking + graphics feasibility notes (what we need to test)

This ticket’s goal is explicitly to explore QEMU’s support here. At the moment, we should treat the following as **unknown until proven**:

### Networking

Questions to answer empirically:

- Does ESP32-S3 QEMU support any emulated network device, or is networking effectively “not available”?
- If some networking is supported:
  - What IP configuration is available (DHCP? static? host-only)?
  - Can we open sockets reliably?
  - How do timing and retransmissions behave under QEMU?

### Graphics (“graphics in QEMU”)

ESP32 graphics in this repo often means:

- SPI display controllers (`esp_lcd`, ST7789, etc.), and/or
- drawing via libraries (M5GFX).

QEMU may not emulate those peripherals. So for QEMU exploration, “graphics” may need to mean one of:

- **Text-mode rendering**: draw to an in-memory framebuffer and periodically dump it as ASCII art to serial logs.
- **File dump rendering**: write framebuffer snapshots to a file in the emulated filesystem (SPIFFS/FAT) and pull it out for inspection.
- **Host-side viewer via serial/TCP**: encode framebuffer diffs and stream them over the QEMU serial TCP port to a host tool that renders them (lowest dependency on QEMU peripheral emulation).

The first step is to decide which definition we want and then implement the smallest possible proof.

## Recommended starting point for the new project

1. Reuse the known-good QEMU run loop (serial + monitor + log capture).
2. Add an explicit “capabilities probe” app that prints:
   - time functions
   - filesystem mount state
   - attempts to init network stack
   - a small “framebuffer test” (whatever our chosen definition is)

## Related tickets/documents

- `0015-QEMU-REPL-INPUT`: UART RX / interactive input investigation (general QEMU serial IO learning).
- `0016-SPIFFS-AUTOLOAD`: partition/filesystem behavior under QEMU (first-boot format behavior + logs).
