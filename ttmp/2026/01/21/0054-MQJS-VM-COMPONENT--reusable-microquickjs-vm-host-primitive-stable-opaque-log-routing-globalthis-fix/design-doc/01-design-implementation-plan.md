---
Title: Design + implementation plan
Ticket: 0054-MQJS-VM-COMPONENT
Status: active
Topics:
    - esp-idf
    - javascript
    - quickjs
    - microquickjs
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/js_service.cpp
      Note: Stable ctx->opaque + log routing in callback-driven VM
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.c
      Note: ctx->write_func(ctx->opaque
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: Existing print_value pattern to refactor
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:36:22.1043323-05:00
WhatFor: ""
WhenToUse: ""
---


# Design + implementation plan

## Executive Summary

Extract a reusable MicroQuickJS “VM host primitive” as `components/mqjs_vm`.

This component makes engine invariants explicit and reusable:

- stable `ctx->opaque` type for *all* subsystems (timeouts + printing),
- log routing (`stdout` vs captured string) without pointer swapping,
- C-side `globalThis` fix (stdlib placeholder),
- a safe “print value” helper used by both REPL (`JsEvaluator`) and callback-driven systems (`0048` js_service).

## Problem Statement

We have two different MicroQuickJS embeddings with duplicated logic:

1) REPL evaluator:
   - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
2) Queue-owned service VM:
   - `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`

Both need:

- `JS_NewContext(...)` with fixed arena
- evaluate code and format results/exceptions using `JS_PrintValueF`

But 0048 exposed two critical engine integration pitfalls:

- **Syntax constraints** (ES5-ish; arrow functions fail)
- **Opaque/log coupling**:
  - MicroQuickJS prints via `ctx->write_func(ctx->opaque, ...)`
  - timeouts use `ctx->interrupt_handler(ctx, ctx->opaque)`
  - swapping `ctx->opaque` types is a crash vector (see 0048 postmortem)

Right now, `JsEvaluator.cpp` uses a “swap `ctx->opaque` to `std::string*`” pattern that is not safe as a reusable template.

## Proposed Solution

Create component:

- `esp32-s3-m5/components/mqjs_vm/`
  - `include/mqjs_vm.h` (C++ interface is acceptable; both 0048 and JsEvaluator are C++)
  - `mqjs_vm.cpp`
  - `CMakeLists.txt`

### Core types (proposed)

```c++
struct MqjsVmConfig {
  void* arena = nullptr;
  size_t arena_bytes = 0;
  const JSSTDLibraryDef* stdlib = nullptr; // typically `&js_stdlib`
  bool fix_global_this = true;
};

class MqjsVm {
 public:
  explicit MqjsVm(const MqjsVmConfig& cfg);
  ~MqjsVm();

  JSContext* ctx(); // for advanced/native bindings, used carefully

  void SetDeadlineMs(uint32_t timeout_ms);
  void ClearDeadline();

  std::string PrintValue(JSValue v, int flags = JS_DUMP_LONG);
  std::string GetExceptionString(); // calls JS_GetException + PrintValue

 private:
  struct Opaque {
    int64_t deadline_us = 0;
    std::string* capture = nullptr;
  };
  static int InterruptHandler(JSContext* ctx, void* opaque);
  static void WriteFunc(void* opaque, const void* buf, size_t len);
};
```

### Key invariant (must be baked in)

Never change `ctx->opaque` to point at a different type.
Instead, keep a stable struct and toggle fields:

```text
WriteFunc(opaque, buf, len):
  st = (Opaque*)opaque
  if st.capture != null: st.capture.append(buf,len)
  else fwrite(buf,1,len,stdout)

PrintValue(v):
  st.capture = &out
  JS_PrintValueF(ctx, v, flags)
  st.capture = null
```

This is exactly the fix that prevented `print(ev)` crashes in 0048.

## Design Decisions

1) **C++ component**: both key consumers (0048 js_service and `JsEvaluator`) are C++.
2) **Expose `JSContext* ctx()` but discourage casual use**: advanced bindings need it, but it is a foot-gun.
3) **Keep it “host primitive”, not “service”**: queueing/concurrency belongs in `mqjs_service` (ticket 0055).

## Alternatives Considered

1) Leave embedding logic inside each firmware.
   - Rejected: repeats subtle engine invariants and encourages unsafe patterns.

2) Modify MicroQuickJS itself to separate opaque pointers.
   - Rejected: too invasive; we can fix at the embedding layer.

## Implementation Plan

1) Implement `components/mqjs_vm` with stable opaque + log routing + deadline helper.
2) Migrate 0048:
   - Replace local `print_value`/deadline/log setup in:
     - `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
   - Keep 0048-specific bootstraps in 0048 (encoder, emit) for now.
3) Refactor `JsEvaluator` internals without changing its public API:
   - Keep `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.h` unchanged.
   - Update `JsEvaluator.cpp` to use `MqjsVm` for context creation and printing.
   - This is intended to be **non-breaking** for existing REPL users.

## Open Questions

1) Should `MqjsVm` take ownership of the arena buffer or accept externally-managed storage?
   - Prefer accepting external storage (matches current fixed-buffer patterns).

2) Do we bake in the `globalThis` fix?
   - Yes; default `fix_global_this=true`, because the stdlib placeholder is a recurring trap.

## References

- Engine write path coupling:
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.c` (`ctx->write_func(ctx->opaque, ...)`)
- Current REPL embedding:
  - `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp` (`print_value`)
- Current callback-driven embedding:
  - `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp` (`struct CtxOpaque`, `write_to_log`)
