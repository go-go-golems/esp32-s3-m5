---
Title: Diary
Ticket: 0012-0016-NEWLIB-LOCK-ABORT
Status: active
Topics:
    - atoms3r
    - debugging
    - esp-idf
    - esp32s3
    - gpio
    - usb-serial-jtag
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Frequent diary of investigation and fixes for the 0016 newlib lock_acquire_generic abort."
LastUpdated: 2025-12-26T08:35:26.060234397-05:00
WhatFor: "Capture what we tried, what crashed, and why, so future debugging doesn’t re-derive the same FreeRTOS/newlib constraints."
WhenToUse: "Read when resuming work or when reviewing/validating the fix."
---

# Diary

## Goal

Track the investigation and fix for the `0016` boot-time abort in `lock_acquire_generic()`.

## Context

The crash manifests during `tester_state_init()` when GPIO configuration runs while holding a `portENTER_CRITICAL` mux; newlib locks interpret “cannot yield” as ISR context and abort for recursive locks used by stdio/logging.

## Step 1: Reported crash + initial hypothesis

This step captured the on-device crash log showing `abort()` from `lock_acquire_generic()` and connected it to a specific misuse pattern: calling driver/log/stdio work while holding a critical section (`portENTER_CRITICAL`), which makes `xPortCanYield()` false.

### What I did
- Created ticket `0012-0016-NEWLIB-LOCK-ABORT`.
- Added a bug report + analysis doc with the crash excerpt and a hypothesis for why `lock_acquire_generic()` aborts.
- Located the relevant code paths in:
  - `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp`
  - `/home/manuel/esp/esp-idf-5.4.1/components/newlib/locks.c`

### What I learned
- In ESP-IDF 5.4.1 newlib locks, if `!xPortCanYield()` then lock acquisition goes down an ISR/critical path and can `abort()` for recursive locks (stdio/logging).

### What should be done next
- Refactor `signal_state.cpp` so the critical section protects only state updates/copies, not GPIO apply or printing/logging.

## Step 2: Fix signal_state critical-section misuse (commit da151570)

This step implemented the core fix: avoid calling GPIO driver APIs and stdio/logging while holding the `s_state_mux` critical section. The change is intentionally mechanical and safety-oriented: we keep the “single writer state machine” semantics, but we move all hardware/apply work outside the lock by copying a snapshot first.

The key outcome is that this should prevent `lock_acquire_generic()` aborts caused by newlib recursive locks when `xPortCanYield()` is false in a critical section.

**Commit (code):** da151570ac54d53534efe44b07987361626a8123 — "0016: avoid gpio/log/stdio inside critical sections"

### What I did
- Updated `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp`:
  - snapshot `s_state` under lock
  - run `apply_config_snapshot(snapshot)` outside lock
  - run `print_status_snapshot(snapshot)` outside lock

### What warrants a second pair of eyes
- Confirm no remaining code paths call `printf`/`ESP_LOG*` while holding `s_state_mux`.

## Related

 - Bug report + analysis: `../analysis/01-bug-report-analysis-lock-acquire-generic-abort-on-boot.md`
 - Ticket tasks: `../tasks.md`
