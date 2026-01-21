---
Title: Bug Report
Ticket: 0061-MQJS-SERVICE-BOOT-WDT
Status: active
Topics:
    - esp-idf
    - freertos
    - javascript
    - microquickjs
    - quickjs
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0048-cardputer-js-web/main/app_main.cpp
      Note: |-
        Calls `js_service_start()` during `app_main`.
        Boot path reaches js_service_start
    - Path: esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp
      Note: |-
        Starts `mqjs_service` during boot; first consumer path.
        Starts mqjs_service and runs bootstrap job
    - Path: esp32-s3-m5/components/mqjs_service/include/mqjs_service.h
      Note: Public API used by 0048 wrapper.
    - Path: esp32-s3-m5/components/mqjs_service/mqjs_service.cpp
      Note: |-
        Worker task (`service_task`) blocks/hangs in `xQueueReceive` at boot.
        service_task xQueueReceive site (line ~134 in current file)
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T18:19:28.524145859-05:00
WhatFor: ""
WhenToUse: ""
---


# Bug Report: Boot-time interrupt watchdog timeout in `mqjs_service`

## Summary

On device boot for firmware `0048-cardputer-js-web`, the system sometimes panics with:

- `Guru Meditation Error: Core  0 panic'ed (Interrupt wdt timeout on CPU0).`
- Backtrace points to `components/mqjs_service/mqjs_service.cpp:134` inside `service_task()` at:
  - `xQueueReceive(s->q, &msg, portMAX_DELAY)`
  - which is stuck inside `spinlock_acquire` / `xPortEnterCriticalTimeout`.

This is suspiciously similar in shape to earlier “boot panic in service_task queue receive”, but the failure mode is now an **interrupt watchdog timeout** (CPU stuck too long with interrupts disabled while spinning for a spinlock).

## Environment

- Target: ESP32-S3 (Cardputer-class device)
- ESP-IDF: v5.4.1 (`/home/manuel/esp/esp-idf-5.4.1`)
- Firmware: `esp32-s3-m5/0048-cardputer-js-web`
- Component under suspicion: `esp32-s3-m5/components/mqjs_service`
- Toolchain: `xtensa-esp-elf` via ESP-IDF

## Timeline / Prior related failures

Work landed under ticket **0055-MQJS-SERVICE-COMPONENT** to extract a reusable VM-owner task component:

- `3e2e671` — added `components/mqjs_service/` and refactored 0048 to use it.
- `1cbd51f` — added a task-start handshake + NULL-queue guard to address an early boot panic.
- `39a30bf` — reworked sync completion to avoid stack pending objects + use `xSemaphoreCreateBinaryStatic()` embedded storage.

Even after the sync semaphore fix, the device can still fail at boot with the WDT timeout below.

## Symptoms / Logs

### Current failing log (boot interrupt WDT)

User report excerpt (confirmed build/flash):

```text
I (310) app_init: Project name:     cardputer_js_web_0048
I (310) app_init: App version:      856f869
I (311) app_init: Compile time:     Jan 21 2026 18:31:08
I (311) app_init: ELF file SHA256:  ff471e8ac...
I (325) main_task: Calling app_main()
I (325) cardputer_js_web_0048: boot
Guru Meditation Error: Core  0 panic'ed (Interrupt wdt timeout on CPU0).

Backtrace: ...
--- (inlined by) xQueueReceive at .../components/freertos/FreeRTOS-Kernel/queue.c:1549
--- 0x42017ec1: (anonymous namespace)::service_task(void*) at .../components/mqjs_service/mqjs_service.cpp:134
```

Key observation: in the current `mqjs_service.cpp`, line 134 is exactly the `xQueueReceive(s->q, &msg, portMAX_DELAY)` call in the worker loop. The `app_init` header above confirms this is not a stale build: the flashed firmware matches git commit `856f869` and ELF SHA `ff471e8ac`.

### Previously observed (related) failure modes

1) Earlier boot panic at the same conceptual location (queue receive path), fixed by task-start handshake:

- Panic looked like `LoadProhibited` with EXCVADDR `0x4d`, consistent with an invalid queue handle.

2) Earlier FreeRTOS assert in sync completion path (fixed by 39a30bf):

```text
assert failed: xQueueGenericSend queue.c:937
... (anonymous namespace)::service_task(void*) at .../components/mqjs_service/mqjs_service.cpp:205
```

## Why this is concerning (initial analysis)

`Interrupt wdt timeout` + backtrace inside `spinlock_acquire` / `xPortEnterCriticalTimeout` strongly suggests:

- The worker task is **spinning trying to acquire a spinlock**, and interrupts remain disabled while it spins.
- In this trace, that spinlock acquisition happens as part of `xQueueReceive`, i.e. on the queue’s internal lock.

This can happen if:

1) The queue handle `s->q` is **invalid/corrupted**, pointing at memory that is not a valid queue object (so the lock data is nonsense and appears “permanently held”), or
2) The queue object is valid but its lock is **permanently held** due to some other corruption, or
3) There is a subtle early-boot interaction where the queue/spinlock is used before being safely initialized (less likely after the handshake, but still possible).

## Reproduction notes

We need a precise reproduction script so we stop chasing “maybe stale build” ghosts:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web
source /home/manuel/esp/esp-idf-5.4.1/export.sh
idf.py -B build_esp32s3_mqjs_service_comp fullclean
idf.py -B build_esp32s3_mqjs_service_comp build
idf.py -B build_esp32s3_mqjs_service_comp flash monitor
```

On boot, capture the full `app_init` header including:

- Project name
- App version (git hash)
- Compile time
- ELF SHA256

## What we tried so far

- Added task-start handshake (`ready` semaphore) so `mqjs_service_start()` blocks until the worker task has started (commit `1cbd51f`).
- Reworked sync completion semantics to avoid stack cross-task pointers and heap semaphores (commit `39a30bf`).
- Verified the build/flash pipeline:
  - ran `idf.py -B build_esp32s3_mqjs_service_comp fullclean build flash monitor`
  - captured `app_init` `App version: 856f869` and `ELF file SHA256: ff471e8ac...` from the failing boot.
- Implemented “force internal RAM” allocation for the service’s critical RTOS primitives (commit `0b17809`):
  - `s->q` allocated via `xQueueCreateWithCaps(..., MALLOC_CAP_INTERNAL)`
  - `s->ready` allocated via `xSemaphoreCreateBinaryWithCaps(MALLOC_CAP_INTERNAL)`
  - added a minimal boot log line in `service_task` before the first `xQueueReceive`.

## Next experiments (reasoned approach)

1) **Re-flash after the internal-RAM queue/semaphore change**
   - `idf.py -B build_esp32s3_mqjs_service_comp build flash monitor`
   - Confirm `App version` now includes `0b17809` (or later) and the WDT behavior changes.

2) **If the WDT persists, extend instrumentation without touching the queue**
   - Avoid calling any queue API (if the lock is corrupted, they will hang too).
   - Log pointer provenance:
     - `esp_ptr_internal(s->q)` and `esp_ptr_internal(s)` results
   - Log CPU/core placement:
     - pin worker to core1 (diagnostic), or confirm scheduler placement.

3) **Move worker task to core1 (diagnostic)**
   - If the WDT is CPU0-specific (or interactions with the main task), pinning the JS worker to core1 may change the symptom.
   - This is not necessarily a final fix; it’s a diagnostic to isolate.

4) **Add a startup self-test**
   - Immediately after worker start, send a known message (no-op job) and require an ack.
   - If ack fails/hangs, return a clear error from `mqjs_service_start()` before the system limps into a watchdog.

## Files & symbols to inspect first

- `components/mqjs_service/mqjs_service.cpp`
  - `service_task(void*)`
  - `mqjs_service_start(...)`
- `0048-cardputer-js-web/main/js_service.cpp`
  - `js_service_start()` → `mqjs_service_start(...)` and bootstrap job
- `0048-cardputer-js-web/main/app_main.cpp`
  - the boot sequence that starts the JS service
