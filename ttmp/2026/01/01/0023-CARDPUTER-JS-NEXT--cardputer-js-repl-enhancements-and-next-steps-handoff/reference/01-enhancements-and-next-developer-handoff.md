---
Title: Enhancements and Next Developer Handoff
Ticket: 0023-CARDPUTER-JS-NEXT
Status: active
Topics:
    - cardputer
    - esp32s3
    - esp-idf
    - console
    - usb-serial-jtag
    - keyboard
    - display
    - javascript
    - debugging
    - uart
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: imports/esp32-mqjs-repl/mqjs-repl/CMakeLists.txt
      Note: Build wiring for SPIFFS partition image
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_atom.h
      Note: Atom header must match generated stdlib tables
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp
      Note: Firmware entrypoint wiring console+REPL+evaluator
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c
      Note: JS runtime stubs including load(path) implementation
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: JS eval
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/eval/ModeSwitchingEvaluator.cpp
      Note: Evaluator selection and routing
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/repl/LineEditor.cpp
      Note: Line discipline; place to add history/arrows/multiline
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp
      Note: REPL command parsing and dispatch (:autoload/:stats/:mode etc)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/storage/Spiffs.cpp
      Note: SPIFFS mount + file read helper
    - Path: imports/esp32-mqjs-repl/mqjs-repl/partitions.csv
      Note: Partition offsets (storage @ 0x410000) used by QEMU merge/flash
    - Path: imports/esp32-mqjs-repl/mqjs-repl/spiffs_image/autoload/00-seed.js
      Note: Seeded autoload script (AUTOLOAD_SEED)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/flash_device_usb_jtag.sh
      Note: Stable USB Serial/JTAG flash helper (usb_reset; by-id)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/gen_esp32_stdlib.sh
      Note: Regenerates ESP32 stdlib + atom header; keep in sync
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_device_uart_raw.py
      Note: Device smoke test that asserts AUTOLOAD_SEED
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_qemu_uart_stdio.sh
      Note: QEMU smoke test (merges storage.bin; asserts AUTOLOAD_SEED)
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T20:29:04.150028649-05:00
WhatFor: ""
WhenToUse: ""
---


# Enhancements and Next Developer Handoff

## Goal

Hand off “what’s next” for running a practical JS interpreter on the M5Stack Cardputer, including:

- the enhancement backlog (REPL UX, keyboard/display, storage/tooling),
- how the current firmware is structured, and
- exactly where to start in code (files + key symbols) with validation commands.

## Context

Ticket `0014-CARDPUTER-JS` brought up a working MicroQuickJS REPL on Cardputer + QEMU with:

- `:mode js` evaluator, `:stats`, `:reset`, `:autoload [--format]`, `:prompt`, `:help`
- SPIFFS-backed `load(path)` and `:autoload` which scans `/spiffs/autoload/*.js`
- a seeded SPIFFS image: `spiffs_image/autoload/00-seed.js` sets `globalThis.AUTOLOAD_SEED = 123`
- device + QEMU smoke tests that assert autoload actually loads JS (not just “SPIFFS mounted”)
- stable flashing helper for USB Serial/JTAG (`--before usb_reset`) + device scripts accept `--port auto`

This ticket (`0023-CARDPUTER-JS-NEXT`) is the “next developer” handoff for enhancements and follow-ups.

## Quick Reference

### Where the system is (entry points)

- Firmware entry: `imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp`
- REPL loop: `ReplLoop::Run`, `ReplLoop::HandleLine` in `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp`
- Line discipline: `LineEditor::FeedByte` in `imports/esp32-mqjs-repl/mqjs-repl/main/repl/LineEditor.cpp`
- Evaluators:
  - `ModeSwitchingEvaluator::SetMode` in `imports/esp32-mqjs-repl/mqjs-repl/main/eval/ModeSwitchingEvaluator.cpp`
  - `JsEvaluator::EvalLine`, `JsEvaluator::Autoload`, `JsEvaluator::Reset` in `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- Storage:
  - `SpiffsEnsureMounted`, `SpiffsReadFile` in `imports/esp32-mqjs-repl/mqjs-repl/main/storage/Spiffs.cpp`
  - JS `load(path)` in `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c` (`js_load`)
- SPIFFS image seeding:
  - `spiffs_create_partition_image(storage spiffs_image FLASH_IN_PROJECT)` in `imports/esp32-mqjs-repl/mqjs-repl/CMakeLists.txt`
  - seed file: `imports/esp32-mqjs-repl/mqjs-repl/spiffs_image/autoload/00-seed.js`

### Golden invariants (do not regress)

- Stdlib + atom header must match: `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h` and `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_atom.h` are generated together via `imports/esp32-mqjs-repl/mqjs-repl/tools/gen_esp32_stdlib.sh`.
- No “backwards compat” fallback stdlib: firmware assumes the generated ESP32-safe tables are in use.
- SPIFFS formatting is explicit: only `:autoload --format` will format on mount failure.

### Device workflows (copy/paste)

Build:
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/set_repl_console.sh usb-serial-jtag
./build.sh build
```

Stable flash on USB Serial/JTAG (recommended):
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/flash_device_usb_jtag.sh --port auto
```

Device smoke test (“real autoload”, asserts `AUTOLOAD_SEED`):
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
python3 ./tools/test_js_repl_device_uart_raw.py --port auto --timeout 90
```

QEMU smoke test (merges `storage.bin` so autoload is real):
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/test_js_repl_qemu_uart_stdio.sh --timeout 120
```

### Enhancement backlog (prioritized)

Below, each item includes: **where**, **key symbols**, and **how to validate**.

#### P0 — Make the REPL usable as an “interpreter”

1) Command history + arrow key navigation
- Where: `imports/esp32-mqjs-repl/mqjs-repl/main/repl/LineEditor.cpp`
- Key symbols: `LineEditor::FeedByte`, `LineEditor::PrintPrompt`
- Notes: current behavior intentionally ignores ANSI escapes; to support arrows, parse escape sequences and maintain a history ring buffer.
- Validate: interactive device session; keep existing smoke tests (they should still pass).

2) Multiline input / continuation prompt for JS
- Where: `imports/esp32-mqjs-repl/mqjs-repl/main/repl/LineEditor.cpp`, `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- Key symbols: `LineEditor::FeedByte`, `JsEvaluator::EvalLine`
- Notes: simplest approach is bracket/quote tracking; if MicroQuickJS exposes a “compile-only” flag, prefer that.
- Validate: type an unterminated block (`function f(){`) and continue until it evaluates.

3) Improve exception reporting (include stack if available)
- Where: `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- Key symbols: `print_value(...)` helper, `JsEvaluator::EvalLine`
- Validate: run `throw new Error("x")` and ensure output contains meaningful stack/trace info.

#### P0 — Storage primitives (beyond `load()`)

4) Add `:ls`, `:cat`, and `:rm` commands (SPIFFS)
- Where: `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp`, `imports/esp32-mqjs-repl/mqjs-repl/main/storage/Spiffs.cpp`
- Key symbols: `ReplLoop::HandleLine`, `SpiffsEnsureMounted`
- Validate: run on device with seeded SPIFFS; list `/spiffs/autoload/00-seed.js` and print its contents.

5) Sort autoload order deterministically
- Where: `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- Key symbols: `JsEvaluator::Autoload`
- Notes: `readdir()` ordering is not stable; collect filenames, sort, then load.
- Validate: add multiple `spiffs_image/autoload/*.js` and verify load order matches filenames.

6) Make `load(path)` streaming / larger-file-friendly
- Where: `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c`
- Key symbols: `js_load`
- Notes: current implementation reads whole file into RAM and caps at 32 KiB.
- Validate: add a larger script (e.g. 64–128 KiB) and ensure it loads without large allocations (or fail gracefully with a clear error).

#### P1 — Cardputer “device-first” integration

7) Keyboard → REPL input (no PC required)
- Where: new module(s) under `imports/esp32-mqjs-repl/mqjs-repl/main/` (suggested: `input/`), wiring in `imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp`
- Key idea: push key events into a queue; create an `IConsole` implementation that reads from that queue (or refactor REPL loop to accept an `IInput` + `IOutput`).
- Validate: type on the Cardputer keyboard and confirm line editing works (including backspace and enter).

8) Display output (mirror console to screen)
- Where: new output sink; consider a `TeeConsole` that writes to USB + screen, or split console into input/output interfaces.
- Validate: prompt and evaluated output appear on display; USB console still works.

9) Speaker feedback (optional)
- Where: UI integration layer; add a “beep on error” or “beep on autoload complete”.
- Validate: audible feedback without blocking the REPL.

#### P1 — Memory budget + diagnostics

10) Measure and document peak memory use (Cardputer)
- Where: `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp` (extend `:stats`), `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp` (`JS_DumpMemory`)
- Key symbols: `ReplLoop::HandleLine` (stats command), `JsEvaluator::GetStats`
- Validate: run typical workloads (autoload + evaluate scripts) and record min-free heap + JS memory dump.

#### P2 — Tooling robustness

11) Make all device scripts default to by-id ports
- Where: `imports/esp32-mqjs-repl/mqjs-repl/tools/*.py`, `imports/esp32-mqjs-repl/mqjs-repl/tools/*.sh`
- Key symbols/scripts: `flash_device_usb_jtag.sh`, `test_js_repl_device_uart_raw.py`, `test_repeat_repl_device_uart_raw.py`
- Validate: unplug/replug device and confirm scripts still succeed without editing port strings.

## Usage Examples

### “I want a one-command device check that proves autoload loads real JS”

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/set_repl_console.sh usb-serial-jtag
./build.sh build
./tools/flash_device_usb_jtag.sh --port auto
python3 ./tools/test_js_repl_device_uart_raw.py --port auto --timeout 90
```

### “I’m adding another library to ship on-device by default”

1) Add file(s) to `imports/esp32-mqjs-repl/mqjs-repl/spiffs_image/autoload/NN-*.js`
2) Rebuild: `./build.sh build` (regenerates `build/storage.bin`)
3) Flash: `./tools/flash_device_usb_jtag.sh --port auto`
4) In REPL: `:mode js` then `:autoload --format` then verify expected globals exist

### “I’m debugging autoload order issues”

- Start in `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp` (`JsEvaluator::Autoload`)
- Ensure deterministic sort before evaluating scripts

## Related

- Previous bring-up ticket: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/index.md`
- QEMU input bug: `ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/index.md`
- SPIFFS/autoload bug notes: `ttmp/2025/12/29/0016-SPIFFS-AUTOLOAD--bug-spiffs-first-boot-format-autoload-js-parse-errors/index.md`
