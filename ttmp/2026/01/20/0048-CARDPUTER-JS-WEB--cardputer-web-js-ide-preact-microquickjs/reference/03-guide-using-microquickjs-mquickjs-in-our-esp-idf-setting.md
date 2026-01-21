---
Title: 'Guide: Using MicroQuickJS (mquickjs) in our ESP-IDF setting'
Ticket: 0048-CARDPUTER-JS-WEB
Status: active
Topics:
    - cardputer
    - esp-idf
    - webserver
    - http
    - rest
    - websocket
    - preact
    - zustand
    - javascript
    - quickjs
    - microquickjs
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: Authoritative MicroQuickJS API (JS_NewContext
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h
      Note: Generated stdlib table and definition of js_stdlib
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c
      Note: Runtime JS builtins (print/load/gc/etc) and inclusion point for generated stdlib
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: Canonical in-repo eval+format+exception pattern
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/partitions.csv
      Note: Reference partition table defining storage SPIFFS for load/autoload
    - Path: esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md
      Note: Repo playbook for native extensions
    - Path: esp32-s3-m5/ttmp/2026/01/11/MO-02-ESP32-DOCS--esp32-firmware-diary-synthesis/playbook/15-guide-microquickjs-repl-extensions.md
      Note: Synthesized guide for REPL+extensions patterns
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-20T23:25:48.533124682-05:00
WhatFor: 'A repo-specific guide for embedding and operating MicroQuickJS safely inside ESP-IDF firmware: stdlib wiring, memory/time limits, output capture, native extensions, and concurrency patterns, grounded in our existing code (JsEvaluator) and tickets.'
WhenToUse: Use when implementing any on-device JS evaluation (REPL, REST eval, WS-driven scripts) to avoid common failure modes (OOM, hangs, parser/stdlib mismatches, SPIFFS mount issues).
---


# Guide: Using MicroQuickJS (mquickjs) in our ESP-IDF setting

## Goal

Explain how to use MicroQuickJS correctly and safely *in this repo’s ESP-IDF setting*:

- which files provide the engine and the “standard library” (`js_stdlib`)
- how to create a `JSContext` with a fixed memory pool (no fragmentation surprises)
- how to evaluate code and turn results/exceptions into strings for UI/HTTP
- how to implement timeouts (so user code can’t brick the device)
- how to add native functions (“extensions”) in a maintainable way
- what to watch for (SPIFFS partitions, thread-safety, blocking, memory pressure)

## Context

MicroQuickJS is a small QuickJS-derived interpreter designed for embedded use. In our codebase, the “proper” way to use it is not “call `JS_Eval` and hope”; it is a disciplined pattern:

1) **Use a fixed memory pool** for the VM (predictable, no heap fragmentation).
2) **Use a single VM (or a small fixed number of VMs)** and serialize access with a mutex.
3) **Bound evaluation** by time and input size (otherwise user JS can hang the HTTP task).
4) **Format results and exceptions** in a stable, human-readable way.
5) **Keep extensions explicit**: document the contract and tie it to code locations.

This guide is intentionally grounded in existing working code and past tickets:

- “What to imitate” (reference implementation):
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- “How the engine API looks” (authoritative signatures):
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`
- “Where the stdlib comes from (and why it’s weird)” (generated, included by runtime C):
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c`
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h`

When we say “our setting”, we mean:

- ESP-IDF firmware with FreeRTOS tasks.
- Often a device-hosted UI: browser → REST → JS eval → JSON result.
- Cardputer-style console defaults: prefer USB Serial/JTAG for interactive logs/REPL.

## Quick Reference

### The minimal mental model

- A MicroQuickJS “VM” is represented by `JSContext*`.
- You create it with:
  - `JS_NewContext(mem_start, mem_size, &js_stdlib)`
- You evaluate code with:
  - `JS_Eval(ctx, input, input_len, filename, flags)`
- The result is a `JSValue` tagged union; errors surface as the special “exception” value:
  - check with `JS_IsException(val)`
  - fetch the exception value with `JS_GetException(ctx)`
- You format values by installing a log function and calling `JS_PrintValueF`:
  - `JS_SetLogFunc(ctx, write_to_string)`
  - `JS_PrintValueF(ctx, val, JS_DUMP_LONG)`

### Canonical “evaluate one snippet” pattern (as used in-repo)

See `JsEvaluator::EvalLine(std::string_view)` in:

- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`

Key details worth copying:

- Evaluation flags for REPL-ish behavior:
  - `JS_EVAL_REPL | JS_EVAL_RETVAL`
- Exception formatting:
  - `JSValue exc = JS_GetException(ctx);`
  - prefix with `"error: "` and print the exception value
- Return value formatting:
  - only print if not `undefined`

### Where `js_stdlib` comes from (and why you must not “half-copy” it)

In this repo’s MicroQuickJS REPL implementation:

- `js_stdlib` is a **global `const JSSTDLibraryDef`** defined in the generated header:
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h` (contains `const JSSTDLibraryDef js_stdlib = {...};`)
- That header is included at the end of the runtime C file:
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c` (ends with `#include "esp32_stdlib.h"`)

Implication:

> If you copy `JsEvaluator.cpp` without also providing a matching `js_stdlib`, you will link-fail or (worse) run with a mismatched atom/function table.

Practical rule:

- Treat `esp32_stdlib_runtime.c` + `esp32_stdlib.h` as a **pair**.

### Memory model (how we avoid fragmentation)

The in-repo evaluator uses a fixed buffer:

- `JsEvaluator::kJsMemSize = 64 * 1024`
- `static uint8_t js_mem_buf_[kJsMemSize];`
- `ctx_ = JS_NewContext(js_mem_buf_, kJsMemSize, &js_stdlib);`

This gives:

- predictable upper bound on JS heap
- minimal interaction with ESP-IDF heap allocator (less fragmentation risk)

### Concurrency model (what is safe)

MicroQuickJS contexts are not “FreeRTOS-task-safe” by default. In our setting, use one of:

1) **Single evaluator task model** (recommended when serving REST/WS):
   - a queue of “eval requests”
   - one task owns the VM and processes requests sequentially
2) **Mutex-guarded model** (acceptable for MVP):
   - one global VM
   - one `SemaphoreHandle_t`/mutex around `JS_Eval` and related calls

Avoid:

- calling `JS_Eval` concurrently on the same `JSContext`
- performing evaluation on the HTTP server task if you cannot bound runtime

### Timeouts (prevent infinite loops from bricking the device)

MicroQuickJS exposes an interrupt hook:

- API: `JS_SetInterruptHandler(ctx, handler)`
- contract: handler returns non-zero to interrupt execution

Recommended pattern:

- before eval, set a deadline timestamp in a global/opaque struct
- interrupt handler compares `esp_timer_get_time()` to the deadline

Return contract recommendation (for HTTP):

- `timed_out=true` and an explanatory `error` string

### Output capture (for browser/UI)

MicroQuickJS can print values via `JS_PrintValueF`, but you may want “user stdout”:

- register a JS global `print(...)` that appends to a buffer (instead of `putchar`)
- return the buffer in your HTTP response JSON

Existing reference:

- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c` implements `js_print` (currently writes to stdout).

If you need HTTP-returned logs, reimplement `print` to write to a string/ring buffer owned by your “request context”, not to UART.

### Partitions / SPIFFS (avoid “mount failed” surprises)

If you include SPIFFS-backed helpers like `load()` or autoload:

- ensure the partition table defines a `storage` SPIFFS partition
- see reference partition table:
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/partitions.csv`

Common failure mode:

- `esp_vfs_spiffs_register` fails because `storage` partition doesn’t exist.

### Extensions (native functions) — the “house style”

Canonical guidance lives in:

- `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md`
- `ttmp/2026/01/11/MO-02-ESP32-DOCS--esp32-firmware-diary-synthesis/playbook/15-guide-microquickjs-repl-extensions.md`

House rules:

- prefer a small, explicit API surface (e.g. `gpio.write(pin, val)`), not “bind everything”
- validate types/lengths; return JS exceptions (`JS_ThrowTypeError`, `JS_ThrowRangeError`) on misuse
- do not block inside JS calls; if you need I/O, send an event/queue message to another task

## Usage Examples

### Example: REST handler wants “evaluate code and return JSON”

Conceptual skeleton (not copy/paste complete; tie to the Phase 1 design contract):

1) Read body (bounded size) into a buffer.
2) Lock the VM (or enqueue to the evaluator task).
3) Call `JS_Eval`.
4) If exception, fetch with `JS_GetException`.
5) Format values with `JS_PrintValueF` using a string-backed log func.
6) Return JSON `{ ok, output, error, timed_out }`.

See the Phase 1 design doc for the exact REST schema:

- `esp32-s3-m5/ttmp/.../design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md`

### Example: “I need to add a native function”

Start with the extensions manual:

- `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md`

Then implement:

- argument validation
- conversion helpers (string/int/arrays)
- explicit error returns via `JS_ThrowTypeError` etc.

Use `esp32_stdlib_runtime.c` as a pattern library for how conversions and JS errors are done in practice.

## Related

- `esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md`
- `esp32-s3-m5/ttmp/2026/01/11/MO-02-ESP32-DOCS--esp32-firmware-diary-synthesis/playbook/15-guide-microquickjs-repl-extensions.md`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h`
- `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`
