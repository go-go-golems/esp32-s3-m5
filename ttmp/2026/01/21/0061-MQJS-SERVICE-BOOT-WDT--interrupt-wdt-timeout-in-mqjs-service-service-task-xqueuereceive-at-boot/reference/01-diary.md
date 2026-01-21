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
