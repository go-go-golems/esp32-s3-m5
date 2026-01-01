---
Title: Device-First Developer Handoff Guide
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp
      Note: Entrypoint and wiring for REPL-only firmware
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/console/UsbSerialJtagConsole.cpp
      Note: Cardputer interactive console transport
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp
      Note: REPL command handling and evaluator dispatch
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh
      Note: Fast device-first smoke test
ExternalSources: []
Summary: Device-first handoff guide for continuing 0014-CARDPUTER-JS (REPL → JS → storage) with concrete entry points, symbols, scripts, and task-by-task notes.
LastUpdated: 2026-01-01T00:00:00Z
WhatFor: Onboard the next developer quickly while preserving deep context (why the current architecture exists and how to validate changes).
WhenToUse: Use at the start of any new work session on this ticket.
---


# Device-First Developer Handoff Guide

## Goal

Get a new developer productive on `0014-CARDPUTER-JS` in <30 minutes using a device-first workflow (Cardputer on `/dev/ttyACM*`), while preserving the deep context needed to make correct architecture and debugging decisions.

## Context

This ticket ports a MicroQuickJS REPL firmware to Cardputer and then grows it into a usable on-device JS REPL.

Current state (important):

- REPL core has been split into C++ components with a transport-agnostic `IConsole` and a pluggable evaluator model (RepeatEvaluator-first bring-up).
- QEMU interactive input is unblocked via a UART FIFO polling fallback (QEMU-specific UART RX interrupt gaps).
- Cardputer interactive REPL works over **USB Serial/JTAG** (appears as `/dev/ttyACM0` on this machine).

Open work (high-level):

- Finish the “split” by moving remaining legacy features (SPIFFS/autoload + MicroQuickJS) behind optional components.
- Fix stdlib/atom-table correctness for ESP32 (generate 32-bit stdlib header).
- Add a `JsEvaluator` and mode switching (`:mode js`) to enable actual JavaScript evaluation.
- Reintroduce storage/autoload without blocking the REPL.

## Quick Reference

### Where to start (fast “marks”)

Ticket home:
- `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/index.md`
- `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/tasks.md`
- `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/01-diary.md`

Firmware project (the actual code you build/flash):
- `imports/esp32-mqjs-repl/mqjs-repl/`

Entrypoint + wiring (read this first):
- `imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp:42` (`app_main`, selects console + evaluator + REPL loop)

REPL plumbing:
- `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp:25` (`Run`, calls `HandleLine`)
- `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp:46` (`HandleLine`, meta-commands + evaluator dispatch)
- `imports/esp32-mqjs-repl/mqjs-repl/main/repl/LineEditor.cpp:1` (byte→line, backspace, prompt)

Console transports (device-first):
- `imports/esp32-mqjs-repl/mqjs-repl/main/console/UsbSerialJtagConsole.cpp:1` (Cardputer `/dev/ttyACM*`)
- `imports/esp32-mqjs-repl/mqjs-repl/main/console/UartConsole.cpp:1` (QEMU/UART0; includes FIFO polling fallback)

Evaluator interface + current evaluator:
- `imports/esp32-mqjs-repl/mqjs-repl/main/eval/IEvaluator.h:1`
- `imports/esp32-mqjs-repl/mqjs-repl/main/eval/RepeatEvaluator.cpp:1`

Kconfig “knobs”:
- `imports/esp32-mqjs-repl/mqjs-repl/main/Kconfig.projbuild:1` (choose REPL console transport)

### Device-first workflows (recommended)

Build:
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./build.sh build
```

Flash + monitor + smoke test (tmux-driven):
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/test_repeat_repl_device_tmux.sh --port /dev/ttyACM0 --flash --timeout 220
```

Device raw UART smoke test (no idf_monitor; requires pyserial):
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
python3 -m pip install pyserial
./tools/test_repeat_repl_device_uart_raw.py --port /dev/ttyACM0 --timeout 20
```

Switch REPL console transport (local, edits gitignored `sdkconfig`):
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/set_repl_console.sh usb-serial-jtag
./build.sh build
```

### QEMU (secondary, still useful)

tmux + `idf_monitor`:
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/test_repeat_repl_qemu_tmux.sh --timeout 120
```

Raw TCP UART:
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/test_repeat_repl_qemu_uart_tcp_raw.sh --timeout 120
```

### Task map (what each open task really means)

These are the open tasks as of now; use this as a “what to do next” explainer.

- Task 2 (implementation plan doc): Create a short “what’s left” plan now that console + QEMU input are solved; include evaluator + stdlib + storage sequencing.
- Task 6 (memory budget): Determine peak heap use on Cardputer once JS is enabled (JS heap + buffers + SPIFFS + any caches). The Cardputer’s effective SRAM budget is tight; document measured numbers and constraints.
- Task 7 (hardware test): Extend the existing device smoke tests beyond RepeatEvaluator:
  - prove JS evaluation works (`:mode js`, run a script containing `var`)
  - prove storage mounts and autoload doesn’t brick the REPL
- Task 8 (optional enhancements): Only after core REPL is stable; keep separate from correctness work.
- Task 14 (finish split): Move remaining legacy concerns into components (Storage/Autoload/JsEvaluator) while keeping behavior unchanged where possible.
- Task 18 (meta-commands/modes): Expand meta-commands so they can switch evaluators (e.g. `:mode repeat|js`) and provide help/output that makes debugging easy.
- Tasks 22–26 (stdlib): Generate a 32-bit stdlib header for ESP32 and wire it correctly so keywords like `var` parse and basic scripts run.

## Usage Examples

### Example: “I need to verify the device REPL works in 2 minutes”

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/test_repeat_repl_device_tmux.sh --port /dev/ttyACM0 --flash --timeout 220
```

### Example: “I’m about to touch REPL parsing; what tests should I run?”

- Device (primary):
  - `./tools/test_repeat_repl_device_tmux.sh --port /dev/ttyACM0 --timeout 220`
- QEMU (secondary regression):
  - `./tools/test_repeat_repl_qemu_tmux.sh --timeout 120`

## Related

Deep context docs (read as needed):

- Architecture (split plan): `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/design-doc/02-split-firmware-main-into-c-components-pluggable-evaluators-repeat-js.md`
- QEMU UART RX investigation + plan: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/design-doc/03-qemu-uart-rx-root-cause-workaround-plan.md`
- Stdlib/atom-table analysis: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/analysis/03-microquickjs-stdlib-atom-table-split-why-var-should-parse-current-state-ideal-structure.md`
- Intern research notes: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/sources/intern-research-qemu-uart-results.md`
- MicroQuickJS native extensions reference: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md`
