---
Title: Diary
Ticket: 0054-MQJS-VM-COMPONENT
Status: active
Topics:
    - esp-idf
    - javascript
    - quickjs
    - microquickjs
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/js_service.cpp
      Note: Consumer migration; removes unstable ctx->opaque swapping and uses MqjsVm helpers
    - Path: components/mqjs_vm/include/mqjs_vm.h
      Note: New reusable MicroQuickJS VM host primitive API and invariants
    - Path: components/mqjs_vm/mqjs_vm.cpp
      Note: Implementation of stable ctx->opaque
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/CMakeLists.txt
      Note: IDF component requirements made explicit for IDF 5.4 (spiffs/uart/usb_serial_jtag/etc)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: Consumer migration; keeps JsEvaluator.h stable while using MqjsVm
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T17:25:04.639169913-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Keep a step-by-step record of implementing ticket 0054 (extracting a reusable MicroQuickJS VM host primitive) with enough detail to reproduce the work and avoid repeating the same integration mistakes.

## Step 1: Add `mqjs_vm` component + migrate `0048` and `mqjs-repl`

I extracted the MicroQuickJS hosting details (ctx creation, stable `ctx->opaque` usage, log routing/capture, and evaluation deadline interrupt) into a reusable component so consumers stop hand-rolling fragile plumbing. The main motivation was preventing the class of crashes we saw when code accidentally swapped `ctx->opaque` to a temporary pointer while the engine still expects it to be stable.

This step also migrated the two immediate consumers (`0048-cardputer-js-web` and `imports/esp32-mqjs-repl/mqjs-repl`) to use the new component, and tightened the imported project's component requirements to build cleanly under ESP-IDF 5.4 (explicit `PRIV_REQUIRES` instead of relying on transitive includes).

**Commit (code):** b426db0 — "0054: add mqjs_vm and migrate 0048 + mqjs-repl"

### What I did
- Added new component `components/mqjs_vm/`:
  - `components/mqjs_vm/include/mqjs_vm.h` (`class MqjsVm`, `struct MqjsVmConfig`)
  - `components/mqjs_vm/mqjs_vm.cpp` (`MqjsVm::Create`, `MqjsVm::WriteFunc`, `MqjsVm::InterruptHandler`, registry for `MqjsVm::From`)
- Migrated `0048-cardputer-js-web/main/js_service.cpp` to:
  - stop managing `JS_SetContextOpaque/JS_SetLogFunc/JS_SetInterruptHandler` directly
  - use `MqjsVm::PrintValue`, `MqjsVm::GetExceptionString`, and `MqjsVm::SetDeadlineMs/ClearDeadline`
- Wired `0048-cardputer-js-web` to find the new component:
  - `0048-cardputer-js-web/CMakeLists.txt` adds `../components/mqjs_vm` to `EXTRA_COMPONENT_DIRS`
  - `0048-cardputer-js-web/main/CMakeLists.txt` adds `mqjs_vm` to `PRIV_REQUIRES`
- Migrated `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp` similarly (no header/API change).
- Fixed `imports/esp32-mqjs-repl/mqjs-repl` build under ESP-IDF 5.4 by making dependencies explicit:
  - `imports/esp32-mqjs-repl/mqjs-repl/CMakeLists.txt` adds `EXTRA_COMPONENT_DIRS` for `mqjs_vm` (and `exercizer_control`, needed by `esp32_stdlib_runtime.c`)
  - `imports/esp32-mqjs-repl/mqjs-repl/main/CMakeLists.txt` adds required `PRIV_REQUIRES` (`esp_driver_uart`, `esp_driver_usb_serial_jtag`, `spiffs`, `esp_timer`, `mquickjs`, `exercizer_control`, `mqjs_vm`)

### Why
- MicroQuickJS passes `ctx->opaque` to the configured `JSWriteFunc` and `JSInterruptHandler`. If any consumer code re-points `ctx->opaque` to a temporary buffer (e.g. a `std::string` on the stack) while JS is running, the next log/interrupt callback will interpret a garbage pointer and can crash.
- Centralizing this logic reduces duplicated, easy-to-get-wrong boilerplate across firmwares and keeps the invariant (“opaque is stable”) enforceable by design.

### What worked
- `0048-cardputer-js-web` built successfully:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && idf.py -B build_esp32s3_mqjs_vm_comp2 build`
- `imports/esp32-mqjs-repl/mqjs-repl` built successfully after requirements fixes:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && idf.py -B build_mqjs_vm_comp2 build`

### What didn't work
- Initial `mqjs-repl` build failures under ESP-IDF 5.4 due to missing component requirements:
  - `fatal error: driver/uart.h: No such file or directory` (needed `esp_driver_uart`)
  - `fatal error: esp_spiffs.h: No such file or directory` (needed `spiffs`)
  - `fatal error: mquickjs.h: No such file or directory` (needed `mquickjs`)
  - `fatal error: exercizer/ControlPlaneC.h: No such file or directory` (needed `exercizer_control` and to add it via `EXTRA_COMPONENT_DIRS`)
  - Each was resolved by adding explicit `PRIV_REQUIRES` and `EXTRA_COMPONENT_DIRS` as noted above.

### What I learned
- MicroQuickJS’s public API is intentionally small; patterns from upstream QuickJS like `JS_GetContextOpaque`, `JS_DupValue`, and `JS_FreeValue` aren’t available here. Rooting and safety is expected to use MicroQuickJS’s GC ref helpers (`JSGCRef` + `JS_PUSH_VALUE/JS_POP_VALUE`).
- If a project’s `main` component directly includes headers from other components (e.g. `driver/uart.h`, `esp_spiffs.h`), ESP-IDF 5.4 requires those components be listed in `PRIV_REQUIRES`; relying on accidental transitive include paths breaks.

### What was tricky to build
- Preserving the “stable ctx->opaque” invariant while still supporting “capture output to string” required the capture pointer to live inside the stable object (`MqjsVm::capture_`), not by replacing `ctx->opaque`.
- Because MicroQuickJS doesn’t expose `JS_GetContextOpaque`, `MqjsVm::From(ctx)` can’t retrieve the VM through the engine API; it uses an internal registry keyed by `JSContext*` to support consumers that only store `JSContext*` (e.g. `JsEvaluator`).

### What warrants a second pair of eyes
- The `MqjsVm::From` registry is a simple linked list. It assumes VMs are created/destroyed in a controlled way and isn’t concurrency-hardened; reviewers should confirm this matches expected usage (single context per firmware, created during init, destroyed at shutdown/reset).
- Confirm the `globalThis` fix approach is correct for all the stdlib variants we ship (`FixGlobalThis()` sets `globalThis` to the global object).

### What should be done in the future
- Validate the regression target on real hardware:
  - `print({a:1})` from the REST eval path (should not crash)
  - `encoder.on('click', function(ev){ print(ev); })` (should not crash)

### Code review instructions
- Start with `components/mqjs_vm/include/mqjs_vm.h` and `components/mqjs_vm/mqjs_vm.cpp` to verify invariants and lifetime.
- Then review consumer migrations:
  - `0048-cardputer-js-web/main/js_service.cpp`
  - `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- Validation:
  - `cd 0048-cardputer-js-web && source /home/manuel/esp/esp-idf-5.4.1/export.sh && idf.py -B build_esp32s3_mqjs_vm_comp2 build`
  - `cd imports/esp32-mqjs-repl/mqjs-repl && source /home/manuel/esp/esp-idf-5.4.1/export.sh && idf.py -B build_mqjs_vm_comp2 build`

### Technical details
- `MqjsVm::WriteFunc(void*, const void*, size_t)` routes JS output to either:
  - a capture buffer (`MqjsVm::capture_`) for helpers like `PrintValue`, or
  - `stdout` for normal prints.
- `MqjsVm::InterruptHandler(JSContext*, void*)` uses `deadline_us_` (set via `SetDeadlineMs`) to interrupt long-running eval/flush/callbacks.

## Step 2: Regression verified on device (no crash)

The remaining work for this ticket was to validate the original crash scenario on real hardware after the refactor. The user verified the key reproductions now behave correctly, which effectively closes the loop on the “unstable ctx->opaque” bug class for this firmware family.

**Validation (user-reported):** 2026-01-21 — `print({a:1})` works; `encoder.on('click', function(ev){ print(ev); })` works and does not panic.

### What I did
- Marked task 10 complete in docmgr.
- Recorded the regression validation result here so we don’t re-test or re-litigate this later.

### Why
- The original StoreProhibited panic was triggered by log/print plumbing interacting with callback execution; this is the minimum safety check that proves the new invariant is correct in practice.

### What worked
- User confirmed both:
  - object printing (`print({a:1})`)
  - callback-event printing (`print(ev)` inside `encoder.on('click', ...)`)
  without a crash.

### What didn't work
- N/A (validation passed).

### What I learned
- The stability invariant (“never repoint `ctx->opaque`”) is the right abstraction boundary: once enforced by `MqjsVm`, consumers can safely use print/exception helpers from any execution path (eval or callback).

### What was tricky to build
- N/A (this step was validation only).

### What warrants a second pair of eyes
- N/A (no new code).

### What should be done in the future
- When adding new “capture output” features, ensure they follow the same pattern (stable opaque + internal capture pointer) and never reintroduce `JS_SetContextOpaque` in consumer code.

### Code review instructions
- N/A (no new code).

### Technical details
- Repro snippets:
  - `print({a:1})`
  - `encoder.on('click', function(ev){ print(ev); })`
