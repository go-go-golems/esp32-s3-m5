---
Title: Diary
Ticket: 0055-MQJS-SERVICE-COMPONENT
Status: active
Topics:
    - esp-idf
    - javascript
    - quickjs
    - microquickjs
    - freertos
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/js_service.cpp
      Note: 0048 bindings + wrappers layered on mqjs_service
    - Path: components/mqjs_service/include/mqjs_service.h
      Note: Public service API (eval + run + post)
    - Path: components/mqjs_service/mqjs_service.cpp
      Note: Queue-owned VM task implementation
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T17:52:48.174912472-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Keep a step-by-step record of extracting the “queue-owned MicroQuickJS VM task” pattern into `components/mqjs_service`, including build errors and the exact fixes.

## Context

- Ticket 0054 extracted `components/mqjs_vm` to enforce the “stable ctx->opaque” invariant.
- Ticket 0055 builds on that by extracting the single-owner VM task + inbound queue pattern from `0048-cardputer-js-web` into a reusable service component.

## Step 1: Implement `mqjs_service` + migrate `0048` wrapper layer

This step creates a reusable FreeRTOS task that owns a `MqjsVm` instance and provides two entry points: (1) synchronous eval and (2) running arbitrary jobs on the VM thread (sync or enqueue-only). Then `0048` is refactored to keep its public `js_service_*` API while using `mqjs_service` underneath.

**Commit (code):** 3e2e671 — "0055: add mqjs_service and refactor 0048 js_service"

### What I did
- Added component `components/mqjs_service/`:
  - `components/mqjs_service/include/mqjs_service.h`
    - `mqjs_service_start`, `mqjs_service_eval`, `mqjs_service_run`, `mqjs_service_post`
    - `mqjs_eval_result_t` (+ `mqjs_eval_result_free`)
  - `components/mqjs_service/mqjs_service.cpp`
    - VM owner task + queue
    - lazy `MqjsVm` creation (arena malloc on first message)
    - eval path formats output/error via `MqjsVm::PrintValue` / `MqjsVm::GetExceptionString`
    - job path for application bindings/callback dispatch
- Refactored `0048-cardputer-js-web/main/js_service.cpp` to become:
  - a “start once” wrapper around `mqjs_service_start`
  - 0048 bootstraps (`encoder.on`, `emit`, `__0048_take_lines`) executed via a `mqjs_service_run` bootstrap job
  - callback jobs posted via `mqjs_service_post`:
    - `job_handle_encoder_delta`
    - `job_handle_encoder_click`
  - REST eval wrapper uses `mqjs_service_eval` and then posts a flush job (`job_flush_eval`)
- Wired component discovery/requirements in 0048:
  - `0048-cardputer-js-web/CMakeLists.txt` adds `../components/mqjs_service` to `EXTRA_COMPONENT_DIRS`
  - `0048-cardputer-js-web/main/CMakeLists.txt` adds `mqjs_service` to `PRIV_REQUIRES`

### Why
- Multiple firmwares need the same concurrency-safe pattern: “one JSContext, one owner task”.
- Separating generic service mechanics (queue + execution) from app-specific bindings (encoder/events/ws) makes reuse feasible without copy/paste.

### What worked
- Build succeeded for 0048 after fixes:
  - `cd esp32-s3-m5/0048-cardputer-js-web`
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `idf.py -B build_esp32s3_mqjs_service_comp build`

### What didn't work
- First build failed with a type mismatch:
  - `conflicting declaration 'typedef struct JSSTDLibraryDef JSSTDLibraryDef'`
  - Root cause: MicroQuickJS defines `JSSTDLibraryDef` as an anonymous `typedef struct { ... } JSSTDLibraryDef;` so it cannot be forward-declared as `struct JSSTDLibraryDef`.
  - Fix: include `mquickjs.h` in `components/mqjs_service/include/mqjs_service.h`.
- Then link failed with a mangled C++ symbol:
  - `undefined reference to _Z7JS_EvalP9JSContextPKcjS2_i`
  - Root cause: including `mquickjs.h` in a C++ TU without `extern "C"` gave C++ linkage for `JS_Eval`.
  - Fix: wrap `#include "mquickjs.h"` in `extern "C"` in `mqjs_service.h`.

### What I learned
- For MicroQuickJS headers in C++:
  - Always include them under `extern "C"` to avoid name-mangling link errors.
  - Don’t forward-declare `JSSTDLibraryDef`; include the header.
- Asynchronous jobs should be freed by the VM owner task, not by spawning helper tasks (too expensive for encoder/event rates).

### What was tricky to build
- Keeping `0048` behavior identical while moving the queue/task out:
  - encoder delta coalescing stays in 0048 (critical section + “pending” flag),
  - but the actual callback execution + WS flush is now a posted VM job.

### What warrants a second pair of eyes
- `mqjs_service_post` frees the posted job payload on the VM task; reviewers should confirm all posted `user` payloads are either:
  - immutable (string literals), or
  - heap-allocated and freed inside the job (as done for click events).

### What should be done in the future
- Smoke-test on device (to check off task 9):
  - REST eval still works from browser
  - encoder callbacks still run
  - WS event flushing still broadcasts

### Code review instructions
- Start with service API + execution model:
  - `components/mqjs_service/include/mqjs_service.h`
  - `components/mqjs_service/mqjs_service.cpp`
- Then verify 0048 wrapper behavior stayed stable:
  - `0048-cardputer-js-web/main/js_service.cpp`

### Technical details
- Key symbols:
  - `mqjs_service_start`, `mqjs_service_eval`, `mqjs_service_run`, `mqjs_service_post`
  - `job_bootstrap`, `job_handle_encoder_delta`, `job_handle_encoder_click`, `flush_js_events_to_ws`

## Step 2: Fix early-boot panic in service task (NULL queue / task start race)

After flashing the `mqjs_service` refactor, the device immediately panicked during boot inside the service task’s first `xQueueReceive`. The backtrace pointed at `vPortEnterCritical`/`spinlock_acquire` with a very small invalid address (EXCVADDR `0x4d`), consistent with attempting to enter a critical section using a queue handle that was `NULL` (field-offset address) or otherwise not safely initialized for the running task.

**Commit (code):** 1cbd51f — "0055: handshake-start mqjs_service task"

### What I did
- Added a task-start handshake so `mqjs_service_start` only returns after the created task has actually started running.
- Added an early guard in the task: if `s->q` is `NULL`, the task exits instead of calling `xQueueReceive` and panicking.

### Why
- Without an explicit “task is alive and has its queue pointer” handshake, a failure mode can become a hard-to-debug boot panic (or a deadlock if the creator blocks waiting for a completion that can never arrive).

### What worked
- `idf.py -B build_esp32s3_mqjs_service_comp build` still succeeds after the change.

### What didn't work
- Boot on device previously panicked with:
  - `Guru Meditation Error: Core  0 panic'ed (LoadProhibited)`
  - Backtrace into `components/mqjs_service/mqjs_service.cpp` at the `xQueueReceive` line.

### What I learned
- For “single-owner task services” that expose synchronous APIs, it’s worth paying a tiny one-time cost to ensure the worker task is running before returning from `*_start()`.

### What was tricky to build
- N/A (small control-plane change).

### What warrants a second pair of eyes
- Confirm that the 1-second handshake timeout is appropriate for all targets and that failure cleanup is correct.

### What should be done in the future
- Re-flash and re-run the smoke test (task 9) to confirm:
  - boot is stable
  - REST eval works
  - encoder callbacks and WS event flush still work

### Code review instructions
- Review `components/mqjs_service/mqjs_service.cpp`:
  - `mqjs_service_start` readiness semaphore logic
  - `service_task` early signal + `NULL` queue guard

### Technical details
- The panic location was `service_task` → `xQueueReceive` → `vPortEnterCritical` → `spinlock_acquire`, with EXCVADDR `0x4d` (consistent with a `NULL` queue handle leading to an invalid field-offset address).

## Related

<!-- Link to related documents or resources -->
