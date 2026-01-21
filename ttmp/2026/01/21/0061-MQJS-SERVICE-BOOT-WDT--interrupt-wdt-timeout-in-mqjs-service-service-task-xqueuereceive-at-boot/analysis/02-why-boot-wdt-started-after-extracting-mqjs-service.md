---
Title: Why Boot WDT Started After Extracting mqjs_service
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
    - Path: esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp
      Note: |-
        Sets mqjs_service task stack size and starts service during boot.
        cfg.task_stack_words currently set to 6144/4
    - Path: esp32-s3-m5/components/mqjs_service/include/mqjs_service.h
      Note: |-
        Defines mqjs_service_config_t including task stack size units/defaults.
        task_stack_words field + default semantics
    - Path: esp32-s3-m5/components/mqjs_service/mqjs_service.cpp
      Note: |-
        Creates the FreeRTOS task and queue; where the WDT manifests at xQueueReceive.
        default task_stack_words and xTaskCreate uses words
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T18:41:20.076514248-05:00
WhatFor: ""
WhenToUse: ""
---


# Why Boot WDT Started After Extracting `mqjs_service`

## Question

Why did `0048-cardputer-js-web` start hitting a boot-time **Interrupt WDT timeout** in the JS worker (`mqjs_service`) after we refactored the JS worker code out of the firmware and into a reusable component?

## Executive summary (most likely explanation)

The extraction introduced a configuration layer (`mqjs_service_config_t`) and **quietly changed the FreeRTOS task stack depth by ~4×**, due to a unit mismatch:

- FreeRTOS `xTaskCreate()` stack depth is expressed in **words**, not bytes.
- Pre-extraction `0048` created the JS task with **6144 words** (≈ 24 KiB on ESP32-S3).
- Post-extraction `0048` config sets `cfg.task_stack_words = 6144 / 4` and the service defaults also use `6144 / 4` (1536 words ≈ 6 KiB).

This reduced stack can plausibly lead to **stack overflow / memory corruption** during early boot JS service activity, which then manifests later as:

- queue/lock metadata corruption, and
- a stuck `spinlock_acquire` inside `xQueueReceive`, which spins with interrupts disabled long enough to trip the **interrupt watchdog** on CPU0.

Even when the backtrace *appears* to “die in xQueueReceive”, the root cause can be “queue object corrupted earlier” rather than “xQueueReceive is broken”.

## What changed (from git history)

### Before extraction: JS worker lived inside `0048`

In the last pre-extraction version of `0048-cardputer-js-web/main/js_service.cpp` (commit parent of `3e2e671`), startup was:

- `s_q = xQueueCreate(16, sizeof(Msg));`
- `xTaskCreate(&service_task, "js_svc", 6144, nullptr, 8, &s_task);`

The key line is the stack depth argument:

- `6144` is passed directly to `xTaskCreate`.
- In ESP-IDF FreeRTOS, that argument is **words**.
- So this is **6144 words** ≈ 24 KiB.

### After extraction: JS worker moved into `components/mqjs_service`

Post-extraction, `0048` no longer calls `xTaskCreate` directly. Instead it configures `mqjs_service_config_t`:

- `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`:
  - `cfg.task_stack_words = 6144 / 4;`

And `mqjs_service` itself also defaults to `6144 / 4` words when the field is left at 0:

- `esp32-s3-m5/components/mqjs_service/mqjs_service.cpp`:
  - `if (s->cfg.task_stack_words == 0) s->cfg.task_stack_words = 6144 / 4;`

So, under the new component API, we ended up provisioning:

- **1536 words** ≈ 6 KiB stack

instead of:

- **6144 words** ≈ 24 KiB stack

This is a mechanically provable difference; it does not depend on runtime conditions.

## Why a “stack too small” bug can look like a “queue receive WDT”

The crash symptom is:

- Core 0 interrupt WDT timeout
- PC in `spinlock_acquire`
- call chain includes `xQueueReceive(...)`

On ESP-IDF SMP FreeRTOS:

- entering a critical section involves acquiring a spinlock (portMUX) and disabling interrupts on that core.
- if the spinlock appears “held”, the core will spin.
- if it spins long enough with interrupts disabled, the **interrupt watchdog** fires.

If the queue’s internal spinlock/owner fields are corrupted (e.g. overwritten by stack overflow scribble), `xQueueReceive` may spin forever trying to acquire a lock that can never become “free” according to the corrupted metadata.

Why would the extraction make corruption more likely?

- With the worker moved into a component, we introduced new heap objects (`new Service`) and new message structs. Heap layout differences can make a latent overflow clobber more “sensitive” data (like a queue’s lock) instead of unused padding.
- The stack depth reduction increases probability of overflow dramatically.

## Secondary differences introduced by the extraction (less likely but relevant)

Even if the stack-depth issue is the primary cause, these are real behavioral differences that can amplify or change failure modes:

1) **Queue handle storage moved from a global to a heap object**
   - Before: queue handle was a global `static QueueHandle_t s_q`.
   - After: queue handle is a field `Service::q` in a heap-allocated `Service`.
   - If anything corrupts `Service`, the queue handle becomes garbage and `xQueueReceive(s->q, ...)` will operate on an invalid object.

2) **Startup handshake semaphore introduced**
   - Before: 0048 created the task and immediately returned; first use would send messages later.
   - After: `mqjs_service_start()` blocks on a “ready” semaphore.
   - This changes timing (still should be correct, but it changes early-boot scheduling).

3) **C++ compilation boundary moved**
   - `mqjs_service` is now a separate component in C++.
   - That can change inlining, stack frame sizes, and the shape of early boot code paths (small, but not zero).

## How to validate this hypothesis (cheap, high-signal checks)

1) Restore stack depth to the pre-extraction value (words) and re-test boot:
   - Set `cfg.task_stack_words = 6144;` in `0048-cardputer-js-web/main/js_service.cpp`, OR
   - Fix `mqjs_service` defaults to `6144` (and treat the field as words everywhere).

2) Enable stack overflow detection (if not already):
   - `CONFIG_FREERTOS_CHECK_STACKOVERFLOW` (ESP-IDF menuconfig)
   - Provide a `vApplicationStackOverflowHook` implementation that logs the task name.

3) Log stack high-water mark from the worker task:
   - `uxTaskGetStackHighWaterMark(nullptr)` early in `service_task`
   - This is a very direct “are we close to the edge?” measurement.

If increasing the stack to ~24 KiB makes the WDT disappear, the unit mismatch is the root cause (or at least a necessary contributing factor).

## What the service API should be (design correction)

This bug exists because the config field name and default suggest “bytes”, but FreeRTOS wants “words”.

Two clean options:

**Option A (keep FreeRTOS semantics):**
- Keep the field as words.
- Rename to `task_stack_depth_words` (or similar).
- Default to `6144` (words) to match the known-good value from pre-extraction 0048.
- In firmware config, set `cfg.task_stack_words = 6144;` (no division).

**Option B (offer byte-oriented configuration):**
- Rename field to `task_stack_bytes`.
- In `mqjs_service_start`, convert: `stack_words = task_stack_bytes / sizeof(StackType_t)`.
- This is harder to misuse, but requires an API change.

Given we want minimal churn across firmwares, Option A is likely the safest immediate correction.

## Conclusion

The strongest evidence-backed “difference caused by extraction” is the **4× reduction in worker task stack depth** caused by interpreting a FreeRTOS “words” parameter as “bytes” (or vice versa).

This is consistent with:

- the fact that the firmware previously worked with an in-firmware worker task,
- the fact that the WDT is deep in FreeRTOS critical-section/spinlock code,
- and the fact that memory corruption often surfaces as “queue receive spins forever”.

Next action should be: fix stack depth units to match the pre-extraction known-good configuration, then re-test boot.
