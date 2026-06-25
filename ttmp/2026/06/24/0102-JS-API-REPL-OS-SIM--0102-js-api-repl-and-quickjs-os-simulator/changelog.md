# Changelog

## 2026-06-24

- Initial workspace created


## 2026-06-24

Created intern-facing JS API REPL and QuickJS OS simulator design package; recorded smoke-test environment failure and firmware evidence.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/0102-JS-API-REPL-OS-SIM--0102-js-api-repl-and-quickjs-os-simulator/design-doc/01-js-api-repl-and-os-simulator-design-and-implementation-guide.md — Primary design and implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/24/0102-JS-API-REPL-OS-SIM--0102-js-api-repl-and-quickjs-os-simulator/reference/01-investigation-diary.md — Chronological investigation diary


## 2026-06-24

Added QuickJS as an explicit submodule dependency and fixed the JS smoke runner to use qjs -I; smoke test now passes.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/.gitmodules — New QuickJS submodule mapping
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh — Smoke runner fix


## 2026-06-24

Committed QuickJS submodule and corrected smoke runner invocation (commit 69873159b3186bcdcd577c4616e88051386e45f3).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/.gitmodules — Tracks upstream QuickJS submodule (commit 69873159)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh — Smoke runner now preloads host shim with qjs -I


## 2026-06-24

Implemented portable picoOS QuickJS runtime through Phase 5: core/screen, OS simulator, TUI runtime, examples, and bundle workflow (commits c4ffe35, be1285d, 713f19e, bf8faab, c91c593).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/lib/30-ui-runtime.js — Main fluent TUI runtime
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/tests/run-bundle-smoke.sh — Final bundle validation


## 2026-06-25

Added host-only interactive QuickJS emulator for loading examples, sending keys, and stepping frames (commit 4e0979f).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/tests/run-interactive.sh — Interactive launcher
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/tools/interactive-host.js — Interactive command loop


## 2026-06-25

Added native C++ QuickJS host prototype with C++-implemented picoOS API surface and host-only terminal loop (commit 48b6e0a).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/src/main.cpp — Desktop event loop
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/src/pico_native_api.cpp — Firmware-portable native API prototype


## 2026-06-25

Continued native host hardening: added RAII JSValue cleanup, native smoke runner, and native layout binding/example (commits 6a6d7b7, 267e113).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh — Native smoke validation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/src/pico_native_api.cpp — Native value ownership and layout binding

