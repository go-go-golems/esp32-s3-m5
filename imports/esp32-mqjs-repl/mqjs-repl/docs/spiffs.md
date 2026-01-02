---
Title: Cardputer JS REPL: SPIFFS Storage Reference
Slug: mqjs-spiffs-storage-reference
Short: Developer reference for SPIFFS partition wiring, seeded image, and JS load/autoload contracts
Topics:
  - spiffs
  - esp-idf
  - esp32s3
  - cardputer
  - debugging
  - javascript
IsTemplate: false
IsTopLevel: false
ShowPerDefault: false
SectionType: GeneralTopic
---

# Cardputer JS REPL: SPIFFS Storage Reference

## Overview

This firmware uses SPIFFS for on-device JavaScript libraries and scripts:

- JS `load(path)` reads a file from SPIFFS and evaluates it.
- REPL `:autoload` scans `/spiffs/autoload/*.js` and evaluates each file.
- A seeded SPIFFS image is flashed with the firmware so autoload loads at least one real file.

The design goal is “safe by default”:

- SPIFFS is mounted lazily (not at boot).
- Formatting is explicit: only `:autoload --format` will format if mount fails.

## Narrative Walkthrough (Why SPIFFS Exists Here)

The storage layer is what turns the REPL from a “toy evaluator” into something you can actually use on a device. If you can only type one-liners over serial, you’ll never run meaningful programs. SPIFFS gives us a simple way to ship JavaScript files onto the device and then load them at runtime using the same mechanism on both QEMU and hardware.

There are two ways SPIFFS shows up in practice:

1) **Build-time**: we generate a SPIFFS image (`storage.bin`) from a repo directory (`spiffs_image/`) and flash it into the `storage` partition. This is how we ship default libraries and seed files.
2) **Runtime**: when you run `:autoload` or `load(path)`, the firmware mounts the filesystem and reads files from `/spiffs/...`.

The “safe-by-default” contract is important on embedded hardware. Formatting a filesystem can erase user data, so the firmware never formats implicitly: `:autoload` will fail with an error if it can’t mount, and `:autoload --format` is the explicit opt-in path that formats only when required to mount.

## Partition Layout (Offsets Matter)

Partition table: `imports/esp32-mqjs-repl/mqjs-repl/partitions.csv`

Key facts:

- Partition name/label: `storage`
- Subtype: `spiffs`
- Offset: `0x410000`
- Size: `0x100000` (1 MiB)

Any flashing/merging workflow must write the SPIFFS image (`storage.bin`) at **0x410000**.

## How the SPIFFS Image Is Built

Project CMake: `imports/esp32-mqjs-repl/mqjs-repl/CMakeLists.txt`

This line builds the SPIFFS partition image from a directory:

```cmake
spiffs_create_partition_image(storage spiffs_image FLASH_IN_PROJECT)
```

Inputs:
- Directory: `imports/esp32-mqjs-repl/mqjs-repl/spiffs_image/`

Outputs (after `idf.py build` / `./build.sh build`):
- `imports/esp32-mqjs-repl/mqjs-repl/build/storage.bin`

## Seeded Autoload Script (Making Tests “Real”)

Seed file:
- `imports/esp32-mqjs-repl/mqjs-repl/spiffs_image/autoload/00-seed.js`

Contract:
- Sets `globalThis.AUTOLOAD_SEED = 123`
- Prints a line via `print(...)` so you can see it ran in logs/console

The smoke tests assert this value to prove that autoload actually loaded JS, not just mounted SPIFFS.

## Runtime Mount + File Read Helpers

SPIFFS mount helpers:
- `imports/esp32-mqjs-repl/mqjs-repl/main/storage/Spiffs.cpp`
  - `esp_err_t SpiffsEnsureMounted(bool format_if_mount_failed)`
  - `bool SpiffsIsMounted()`
  - `esp_err_t SpiffsReadFile(const char* path, uint8_t** out_buf, size_t* out_len)`

Mount details (current):
- base path: `/spiffs`
- partition label: `storage`
- `max_files = 8`
- format policy is controlled by the caller (`format_if_mount_failed`).

## `:autoload` Semantics

Implementation:
- `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp` (`JsEvaluator::Autoload`)
- Command parsing:
  - `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp` (`ReplLoop::HandleLine`)

Behavior:
- `:autoload` mounts SPIFFS without formatting; errors if mount fails.
- `:autoload --format` formats+mounts SPIFFS if needed.
- The directory `/spiffs/autoload` is scanned for `*.js` and each file is evaluated.

## `load(path)` Semantics

Implementation:
- `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c` (`js_load`)

Behavior:
- Mounts SPIFFS without formatting (errors if mount fails).
- Reads whole file into a heap buffer and `JS_Eval`s it.
- Enforces a 32 KiB max file size (to avoid large allocations on constrained SRAM).

## Validation Commands

### QEMU (UART stdio)

This test merges the SPIFFS image into the emulated flash and then asserts `AUTOLOAD_SEED`:

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/test_js_repl_qemu_uart_stdio.sh --timeout 120
```

### Device (Cardputer)

Stable flash + device autoload smoke test (port auto-detects by-id):

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/set_repl_console.sh usb-serial-jtag
./build.sh build
./tools/flash_device_usb_jtag.sh --port auto
python3 ./tools/test_js_repl_device_uart_raw.py --port auto --timeout 90
```

## Common Pitfalls

- **Port churn**: USB Serial/JTAG can re-enumerate; use `/dev/serial/by-id/...` or `--port auto`.
- **Autoload ordering**: `readdir()` order is not deterministic. If library dependencies require order, collect names and sort before eval.
- **“Mounted” vs “Contains files”**: a successful mount does not imply any `.js` files exist unless you flash a SPIFFS image or otherwise populate the FS.
