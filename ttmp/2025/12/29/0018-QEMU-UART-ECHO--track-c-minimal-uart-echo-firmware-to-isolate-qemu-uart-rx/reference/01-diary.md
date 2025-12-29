---
Title: Diary
Ticket: 0018-QEMU-UART-ECHO
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - uart
    - serial
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T15:09:28.630658304-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Track building and validating the **minimal UART echo firmware** used to isolate whether ESP32-S3 QEMU can deliver **UART RX** bytes into ESP-IDF at all.

## Step 1: Create Track C ticket and capture upstream context

This step creates a new, focused ticket so a fresh debugging session can start without re-deriving the 0015 history. The core idea is to replace “complex REPL firmware + uncertain console stack” with a tiny echo loop that should work in any sane UART emulation.

### What I did
- Created ticket `0018-QEMU-UART-ECHO`.
- Wrote an analysis doc explaining why Track C is the next isolation step after Track A/Track B results in ticket `0015-QEMU-REPL-INPUT`.
- Added a concrete task list to drive the next session.

### Why
- In `0015`, manual typing appears broken.
- Bypass testing via `imports/test_storage_repl.py` depends on QEMU using a TCP serial backend; our live QEMU run was using `-serial mon:stdio`, so TCP-based bypass is blocked unless we change the backend.
- A minimal echo firmware gives a clear binary outcome: either QEMU UART RX works (then the REPL stack is the issue) or it doesn’t (then QEMU/serial config is the issue).

### What should be done next
- Create the new ESP-IDF firmware directory under `esp32-s3-m5/` next to `0016-atoms3r-grove-gpio-signal-tester/`.
- Implement UART echo loop with finite timeout + heartbeat.
- Run in QEMU and capture transcripts for both serial backends:
  - `-serial mon:stdio` (manual typing)
  - `-serial tcp::5555,server` (scriptable TCP client)

## Quick Reference

### Definition of “UART echo firmware”

Minimal firmware that:
- configures UART0 @ 115200
- reads bytes with `uart_read_bytes(..., timeout)`
- echoes received bytes back via `uart_write_bytes()`
- logs a heartbeat when no bytes are received (proves loop is alive)

### What success looks like

- When you type `abc<enter>`, you see `abc` echoed (RX + TX path validated).
