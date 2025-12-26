---
Title: 'Bug report + analysis: lock_acquire_generic abort on boot'
Ticket: 0012-0016-NEWLIB-LOCK-ABORT
Status: active
Topics:
    - atoms3r
    - debugging
    - esp-idf
    - esp32s3
    - gpio
    - usb-serial-jtag
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-26T08:35:25.918382346-05:00
WhatFor: ""
WhenToUse: ""
---

# Bug report + analysis: `lock_acquire_generic()` abort on boot

## Summary

AtomS3R firmware `0016-atoms3r-grove-gpio-signal-tester` can abort during boot with:

- `abort() was called at PC ... lock_acquire_generic (newlib/locks.c:133)`

The backtrace shows this happens during `tester_state_init()` when we reset/configure GPIO pins while holding a `portENTER_CRITICAL` mux.

## Environment

- **Board**: AtomS3R (ESP32-S3)
- **ESP-IDF**: v5.4.1
- **Project**: `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester`
- **Observed version string**: `App version: 9423e44-dirty` (note: `-dirty` indicates uncommitted local changes at build time)

## Reproduction

1. Build and flash `0016-atoms3r-grove-gpio-signal-tester` to AtomS3R.
2. Boot device and observe logs over USB Serial/JTAG monitor.
3. Crash occurs shortly after display init, when `tester_state_init()` runs.

## Expected

Boot completes, LCD status UI renders, and the REPL prompt appears (or at least the system remains running).

## Actual

Device aborts during boot with a backtrace pointing into newlib lock acquisition.

## Crash log (excerpt)

```text
I (340) atoms3r_gpio_sig_tester: canvas ok: 32768 bytes; free_heap=329780 dma_free=322276

abort() was called at PC 0x40376c53 on core 0
--- 0x40376c53: lock_acquire_generic at .../components/newlib/locks.c:133

Backtrace: ...
--- lock_acquire_generic -> _lock_acquire_recursive -> __retarget_lock_acquire_recursive
--- _vfprintf_r -> vprintf -> esp_log_writev -> esp_log_write
--- gpio_config -> gpio_reset_pin
--- safe_reset_pin -> apply_config_locked -> tester_state_init
--- app_main
```

## Analysis: why this happens

ESP-IDF’s newlib locks treat “cannot yield” as ISR context:

In `components/newlib/locks.c`, `lock_acquire_generic()` does:

- If `!xPortCanYield()`, it assumes ISR/critical context.
- In that context, it **aborts** for recursive mutex locks:
  - `abort(); /* recursive mutexes make no sense in ISR context */`

In our firmware, we call driver and stdio/log functions while holding a critical section:

- `tester_state_init()` does `portENTER_CRITICAL(&s_state_mux)` and calls `apply_config_locked()`.
- `apply_config_locked()` calls `safe_reset_pin()`.
- `safe_reset_pin()` calls `gpio_reset_pin()`, which can call into `gpio_config()` and log via `esp_log_writev()`.
- Logging ultimately calls `vprintf()`, which uses newlib recursive locks.

Because we are inside a critical section (cannot yield), the newlib lock code takes the “ISR context” path and aborts.

This is not specific to `gpio_reset_pin()`; **any** libc/stdio/log path inside `portENTER_CRITICAL` is risky.

## Proposed fix

Refactor `0016/main/signal_state.cpp` to ensure:

- the `s_state_mux` critical section only protects copying/updating `s_state`,
- any “apply hardware config” (GPIO driver calls) happens **after** leaving the critical section,
- any `printf()` / `ESP_LOG*` also happens **after** leaving the critical section.

Implementation note:

- Fix implemented in commit `da151570` (see `0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp`).

## Validation plan

- Rebuild and flash.
- Confirm boot proceeds past `tester_state_init()`.
- Confirm `status` command works repeatedly without abort.
- Confirm mode changes (`mode idle|tx|rx`) work without abort.

