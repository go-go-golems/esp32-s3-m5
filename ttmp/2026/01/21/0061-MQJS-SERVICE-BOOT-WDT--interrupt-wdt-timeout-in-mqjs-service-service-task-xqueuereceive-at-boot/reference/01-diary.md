---
Title: Diary
Ticket: 0061-MQJS-SERVICE-BOOT-WDT
Status: active
Topics:
    - esp-idf
    - freertos
    - javascript
    - microquickjs
    - quickjs
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T18:19:29.448941315-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Keep a step-by-step record of diagnosing the boot-time `Interrupt wdt timeout` in `mqjs_service` so we can (a) fix it and (b) avoid repeating the same failure pattern in future service components.

## Context

- This is a follow-on from ticket **0055-MQJS-SERVICE-COMPONENT**, which extracted `components/mqjs_service` and migrated `0048-cardputer-js-web` to use it.
- Multiple failure modes have appeared at boot inside `service_task()` around the first `xQueueReceive(...)` call.
- This ticket focuses specifically on the **interrupt watchdog timeout** variant (as opposed to the earlier `LoadProhibited` panic and the `xQueueGenericSend` assert).

## Step 1: Create bug ticket + capture the latest failure evidence

The device is still failing to boot reliably, and the current panic is an **interrupt watchdog timeout on CPU0** with a backtrace that points directly at `service_task()` in `components/mqjs_service/mqjs_service.cpp` at the `xQueueReceive(...)` line. This looks “the same” in the sense that we are still dying in the queue receive path, but the mechanism differs: now we appear to be spinning long enough (with interrupts disabled) to trip the interrupt WDT.

This step creates a dedicated bug ticket, records the observed logs and the exact file/line location, and lays out a controlled next set of experiments to eliminate “wrong binary / stale build dir” uncertainty and to quickly distinguish “invalid queue handle” from “valid queue but corrupted lock”.

### What I did
- Created ticket workspace:
  - `ttmp/2026/01/21/0061-MQJS-SERVICE-BOOT-WDT--interrupt-wdt-timeout-in-mqjs-service-service-task-xqueuereceive-at-boot/`
- Added docs:
  - `ttmp/2026/01/21/0061-MQJS-SERVICE-BOOT-WDT--interrupt-wdt-timeout-in-mqjs-service-service-task-xqueuereceive-at-boot/analysis/01-bug-report.md`
  - `ttmp/2026/01/21/0061-MQJS-SERVICE-BOOT-WDT--interrupt-wdt-timeout-in-mqjs-service-service-task-xqueuereceive-at-boot/reference/01-diary.md`
- Added detailed tasks in:
  - `ttmp/2026/01/21/0061-MQJS-SERVICE-BOOT-WDT--interrupt-wdt-timeout-in-mqjs-service-service-task-xqueuereceive-at-boot/tasks.md`

### Why
- The debugging state had accumulated across multiple commits/tickets; we need a single place to:
  - consolidate all observed failure modes + logs,
  - track attempted fixes and the result of each,
  - and force a disciplined reproduce/verify/instrument cycle.

### What worked
- `docmgr` ticket/doc/task creation succeeded and produced a coherent workspace to track this bug.

### What didn't work
- Firmware boot is still failing with:
  - `Guru Meditation Error: Core  0 panic'ed (Interrupt wdt timeout on CPU0).`
  - backtrace into `service_task()` at `xQueueReceive(...)`.

### What I learned
- When backtraces land in FreeRTOS queue/critical-section machinery, “the same backtrace” can hide multiple distinct root causes:
  - invalid handle (NULL/garbage) vs
  - corrupted queue internal lock vs
  - deadlock where the lock is held forever.

### What was tricky to build
- N/A (documentation and task setup step).

### What warrants a second pair of eyes
- Confirm the hypothesis framing in `analysis/01-bug-report.md`:
  - whether the WDT-in-spinlock acquisition most strongly implicates handle corruption vs a lock being held forever.

### What should be done in the future
- Immediately proceed with tasks 1–3:
  - capture full boot log with App version,
  - ensure we are flashing the intended build dir (fullclean),
  - add pre-`xQueueReceive` instrumentation in `mqjs_service`.

### Code review instructions
- Start with the bug report narrative and reproduction script:
  - `ttmp/2026/01/21/0061-MQJS-SERVICE-BOOT-WDT--interrupt-wdt-timeout-in-mqjs-service-service-task-xqueuereceive-at-boot/analysis/01-bug-report.md`
- Then inspect the current `xQueueReceive` site and surrounding startup logic:
  - `esp32-s3-m5/components/mqjs_service/mqjs_service.cpp`

### Technical details
- The current crash report points to:
  - `esp32-s3-m5/components/mqjs_service/mqjs_service.cpp:134` (worker loop `xQueueReceive`)
  - FreeRTOS critical section / spinlock acquisition path (`xPortEnterCriticalTimeout` → `spinlock_acquire`).

## Step 2: Confirm flashed binary + force internal allocation for queue/semaphore

The first order of business was eliminating “wrong build directory / stale binary” uncertainty. The captured `app_init` header shows `App version: 856f869` and `ELF file SHA256: ff471e8ac...`, so the device is running a deterministic, reproducible build that still fails with the interrupt watchdog timeout at `service_task()` → `xQueueReceive(...)`.

Given the failure is inside `spinlock_acquire` during queue receive, a high-probability explanation is that the queue (or its embedded lock) was allocated in a memory region where ESP-IDF/FreeRTOS spinlocks are unsafe or unreliable (e.g. external RAM / PSRAM). ESP-IDF provides `xQueueCreateWithCaps(..., MALLOC_CAP_INTERNAL)` and `xSemaphoreCreateBinaryWithCaps(MALLOC_CAP_INTERNAL)` specifically for this class of problem, so we switched the service’s queue and startup handshake semaphore to internal-RAM-only allocations and added a minimal “task start” log before the first receive.

**Commit (code):** 0b17809 — "0061: allocate mqjs_service queue in internal RAM"

### What I did
- Confirmed reproduction evidence includes the build identity:
  - `App version: 856f869`
  - `ELF file SHA256: ff471e8ac...`
- Updated `components/mqjs_service/mqjs_service.cpp`:
  - `s->q = xQueueCreateWithCaps(..., MALLOC_CAP_INTERNAL)`
  - `s->ready = xSemaphoreCreateBinaryWithCaps(MALLOC_CAP_INTERNAL)`
  - `mqjs_service_stop()` uses `vQueueDeleteWithCaps`
  - Added a one-line boot log in `service_task` before the first `xQueueReceive`:
    - prints `s`, `s->q`, `s->ready`, core id
  - Added a guard that exits early if `s->q` is not internal (`esp_ptr_internal(s->q)`).
- Rebuilt:
  - `cd esp32-s3-m5/0048-cardputer-js-web`
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `idf.py -B build_esp32s3_mqjs_service_comp build`

### Why
- If a FreeRTOS queue is allocated outside internal RAM, its lock/metadata can behave pathologically under `spinlock_acquire`, leading to exactly the observed “interrupt WDT timeout while acquiring a lock”.
- Using the `*WithCaps(MALLOC_CAP_INTERNAL)` APIs lets us force the queue/semaphore into internal RAM regardless of global malloc/PSRAM policy.

### What worked
- Build succeeded after switching to `*WithCaps` APIs.

### What didn't work
- Not yet validated on device; still pending a flash/monitor run with the new commit hash visible in `app_init`.

### What I learned
- On ESP32-class targets with PSRAM, it’s not enough to “assume FreeRTOS objects are internal” when the project’s malloc policy can be configured to prefer external RAM.

### What was tricky to build
- Cleanup correctness: queues created with `xQueueCreateWithCaps()` must be deleted with `vQueueDeleteWithCaps()`; mixing the APIs silently risks heap corruption.

### What warrants a second pair of eyes
- Verify that every failure cleanup path in `mqjs_service_start()` uses the matching `WithCaps` deleters and that we never call `vQueueDelete()` on a `WithCaps` queue.

### What should be done in the future
- Flash and re-test immediately:
  - confirm `App version` includes `0b17809` (or later)
  - confirm whether the boot WDT disappears or changes character.

### Code review instructions
- Review the allocation/cleanup changes in:
  - `esp32-s3-m5/components/mqjs_service/mqjs_service.cpp`

## Quick Reference

### “Are we flashing the right thing?” verification snippet

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web
source /home/manuel/esp/esp-idf-5.4.1/export.sh
idf.py -B build_esp32s3_mqjs_service_comp fullclean
idf.py -B build_esp32s3_mqjs_service_comp build
idf.py -B build_esp32s3_mqjs_service_comp flash monitor
```

Capture the `app_init` section including `App version:` and `ELF file SHA256:`.

## Usage Examples

N/A (this is an implementation diary).

## Related

- `ttmp/2026/01/21/0061-MQJS-SERVICE-BOOT-WDT--interrupt-wdt-timeout-in-mqjs-service-service-task-xqueuereceive-at-boot/analysis/01-bug-report.md`
- `ttmp/2026/01/21/0055-MQJS-SERVICE-COMPONENT--reusable-queue-owned-microquickjs-service-eval-callbacks-event-flush/reference/01-diary.md`
