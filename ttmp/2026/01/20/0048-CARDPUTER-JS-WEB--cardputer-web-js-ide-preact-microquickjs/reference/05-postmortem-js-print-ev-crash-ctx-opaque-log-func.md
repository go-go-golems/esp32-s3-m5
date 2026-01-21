---
Title: 'Postmortem: JS print(ev) crash (ctx->opaque/log_func)'
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
    - Path: 0048-cardputer-js-web/main/js_service.cpp
      Note: CtxOpaque + write_to_log fix for print(ev) crash
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.c
      Note: JS_PrintValueF/js_dump_object/log_func call chain
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c
      Note: js_print uses JS_PrintValueF
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:08:50.91645847-05:00
WhatFor: ""
WhenToUse: ""
---


# Postmortem: JS print(ev) crash (ctx->opaque/log_func)

## Goal

Prevent and quickly diagnose the class of crashes where MicroQuickJS/QuickJS logging (used by `print()` and `JS_PrintValueF`) writes through a mismatched `ctx->opaque`, leading to hard faults (e.g. ESP32 StoreProhibited in `memcpy`/`std::string::append`).

## Context

In ticket `0048-CARDPUTER-JS-WEB`, the firmware embeds **MicroQuickJS** and exposes:

- REST evaluation (`/api/js/eval`) from the web IDE.
- Encoder callbacks (`encoder.on('click'| 'delta', fn)`) invoked from a single “JS service” task.
- A JS-side event queue (`emit(...)`) flushed to the browser via WebSocket.

MicroQuickJS in this repo has a non-obvious coupling:

- The VM **interrupt handler** is installed via `JS_SetInterruptHandler(ctx, fn)` and is invoked with an `opaque` pointer derived from the context.
- Many **printing / formatting** paths (e.g. `print()` in the stdlib runtime, `JS_PrintValueF`, and object dumping) write bytes via `ctx->log_func`, which also receives an `opaque` pointer derived from the same place.

If you treat `ctx->opaque` as “free real estate” for one subsystem (timeouts) and later repurpose printing by installing a log sink that assumes a different opaque type (e.g. `std::string*`), then any JS `print(x)` (especially `print(object)`) can become a type-confusion crash.

## Quick Reference

### Symptom

- `Guru Meditation Error: Core 1 panic'ed (StoreProhibited)` with a backtrace involving:
  - `memcpy` (ROM)
  - `std::string::append(...)`
  - `js_vprintf` / `js_printf`
  - `js_dump_object` / `JS_PrintValueF`
  - `js_print` (stdlib runtime)
  - application callback trampoline (e.g. encoder click callback)

### Reproduction (minimal)

In the web editor (or `esp_console`), register:

```js
encoder.on("click", function (ev) {
  print(ev); // crashes before fix
});
```

### Root Cause (mechanical)

In `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`, the helper `print_value(...)` previously did:

1. `JS_SetLogFunc(ctx, write_to_string)` where `write_to_string(void* opaque, ...)` assumes `opaque` is a `std::string*`.
2. Temporarily swapped `ctx->opaque` to point to a local `std::string out`, then restored it to an `EvalDeadline`.
3. **Did not restore** the log function back to a “safe” default.

Later, when user JS executed `print(ev)`:

- The stdlib `print()` path invoked `JS_PrintValueF`, which used `ctx->log_func == write_to_string`.
- But `ctx->opaque` was an `EvalDeadline*` (or similar), not a `std::string*`.
- `write_to_string` casted the deadline pointer to `std::string*` and called `append(...)`, producing an invalid write and a hard fault.

### Correct Pattern (single opaque, multiplexed)

Use one stable context-opaque structure for *all* subsystems that need it, and never install a log sink that assumes a different opaque type than the context is configured with.

Implemented fix (see symbols below):

- Stable `struct CtxOpaque { EvalDeadline deadline; std::string* log_out; };`
- Set once at init:
  - `JS_SetContextOpaque(s_ctx, &s_opaque);`
  - `JS_SetLogFunc(s_ctx, write_to_log);`
  - `JS_SetInterruptHandler(s_ctx, interrupt_handler);`
- Make `write_to_log(...)` route bytes:
  - to `s_opaque.log_out` when capturing
  - otherwise to `stdout` (UART console)
- Make `print_value(...)` toggle `s_opaque.log_out` with RAII while calling `JS_PrintValueF(...)`.

### Where To Look (files + symbols)

- Firmware integration:
  - `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
    - `struct CtxOpaque`
    - `write_to_log(void* opaque, const void* buf, size_t buf_len)`
    - `interrupt_handler(JSContext* ctx, void* opaque)`
    - `print_value(JSContext* ctx, JSValue v)`
    - `ensure_ctx()`
- MicroQuickJS internals (call chain for object printing):
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.c`
    - `js_vprintf`, `js_printf`
    - `js_dump_object`
    - `JS_PrintValueF`
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c`
    - `js_print`

## Usage Examples

### Validate the fix (manual)

1. Flash firmware, open the web UI, then run:

```js
encoder.on("click", function (ev) {
  print(ev);           // should print a structured dump to the serial console
  emit("click", ev);   // should appear in the WS event history panel
});
```

2. Press the encoder button.
3. Confirm:
   - No crash / reboot.
   - Serial console shows a dumped object (or at least no fault).
   - The browser shows a `js_events` entry with `topic:"click"`.

### Developer rule-of-thumb

If you ever need “capture printing into a string” in MicroQuickJS:

- Do **not** swap `ctx->opaque` between unrelated types.
- Do **not** install a capture-only `log_func` and leave it installed.
- Do implement a “log router” where `ctx->opaque` is a struct containing all needed state.

## Related

- `esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/reference/04-postmortem-microquickjs-bootstrap-failures-arrow-functions-globalthis-null.md`
- `esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/playbook/03-playbook-queue-based-js-service-microquickjs-rest-eval-ws-events-encoder-bindings.md`
