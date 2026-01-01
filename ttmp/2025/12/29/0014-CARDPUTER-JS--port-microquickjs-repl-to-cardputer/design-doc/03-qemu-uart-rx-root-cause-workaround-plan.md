---
Title: QEMU UART RX root cause + workaround plan
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Root cause analysis and plan to restore interactive REPL input under ESP-IDF QEMU (ESP32-S3), plus a parallel device-first plan using Cardputer USB Serial/JTAG.
LastUpdated: 2026-01-01T00:00:00Z
WhatFor: Convert intern research into a concrete fix plan and validation steps for QEMU REPL input and Cardputer console.
WhenToUse: Before implementing more REPL features that assume interactive input works under QEMU.
---

# QEMU UART RX root cause + workaround plan

## Executive Summary

We have a REPL-only firmware (RepeatEvaluator) that prints `repeat>` under QEMU but does not receive UART RX bytes, across multiple input paths (`idf_monitor`, raw TCP, and stdio). Intern research points to a known Espressif-QEMU failure mode: **UART RXFIFO timeout interrupt is not implemented**, so the driver only surfaces RX when the RX FIFO “full” threshold is reached (often ~120 bytes).

Plan:

1) Validate the hypothesis quickly (send a ≥200-byte burst; then force the RX FIFO full threshold down to **1 byte**).
2) Implement a minimal workaround in our console transport (`UartConsole`): after `uart_driver_install()`, set `uart_set_rx_full_threshold(uart_num, 1)` (and optionally `uart_set_rx_timeout(...)` for real hardware responsiveness).
3) Keep QEMU as a fast dev loop once interactive input is unblocked, but prioritize Cardputer hardware for “real” interactive work via **USB Serial/JTAG** (`/dev/ttyACM*`).

## Problem Statement

### Symptom

Under ESP-IDF QEMU (ESP32-S3):

- Firmware prints the REPL prompt (`repeat>`) reliably.
- But *no interactive input* reaches `uart_read_bytes()`:
  - `idf.py qemu monitor` (`idf_monitor` over socket://localhost:5555): no echo, no command processing.
  - Raw TCP UART connection: prompt appears, but commands don’t echo / process.
  - QEMU UART on stdio: prompt appears, but commands don’t echo / process.

This blocks the REPL-only milestone from being interactively usable under QEMU, and blocks JS bring-up steps that depend on a working REPL harness.

### Constraints

- The fix must be minimal and safe on real hardware (Cardputer), and must not depend on MicroQuickJS/stdlib/storage.
- The fix must be automatable (scripts) to prevent regressions.

## Proposed Solution

### Root-cause hypothesis (from intern research)

The most likely cause is a QEMU UART RX interrupt implementation gap:

- QEMU lacks the **UART_RXFIFO_TOUT interrupt** (RX timeout), so the driver isn’t notified for short bursts.
- If the driver only gets an interrupt when “RX FIFO full” triggers, and the default full threshold is high (commonly ~120 bytes), then typing short commands never wakes the driver up.

This matches our observation: TX/logs work, prompt prints, but short inputs never arrive.

### Fix strategy

Avoid relying on RX timeout IRQ by ensuring **RX FIFO full interrupt triggers on 1 byte**.

Concrete firmware change (console layer):

- After `uart_driver_install()` for the UART used for REPL input:
  - `uart_set_rx_full_threshold(uart_num, 1)`
  - optionally `uart_set_rx_timeout(uart_num, 1)` (QEMU may ignore it; useful for real UART responsiveness)

### Device-first parallel path (Cardputer)

Even with QEMU fixed, Cardputer should remain the “real” interactive dev loop:

- Prefer ESP-IDF USB Serial/JTAG console for interactive input/output on `/dev/ttyACM*`.
- Use the existing device scripts to validate repeat REPL on hardware.

## Design Decisions

### D1: Prefer “RX full threshold = 1” over deeper QEMU changes

- It’s a small, self-contained mitigation in our firmware.
- It aligns with the intern-researched failure mode (missing RX timeout IRQ).
- It should be safe for our REPL workload.

### D2: Keep fix scoped to the transport layer

- The fix belongs in `UartConsole` (transport), not in the line editor or evaluator.
- This keeps the REPL core transport-agnostic and matches the architecture intent.

## Alternatives Considered

### A1: Blast ≥120 bytes to “unstick” RX

- Useful diagnostic (no firmware changes).
- Not an acceptable dev loop.

### A2: Switch to `esp_console` / VFS console framework under QEMU

- Might work around UART driver behavior, but adds significant integration surface and complexity.
- Doesn’t align with our goal of a JS REPL with custom line discipline.

### A3: Ignore QEMU interactivity entirely

- Possible fallback if QEMU truly cannot support RX.
- But we already rely on QEMU for rapid iteration; keeping interactive REPL in QEMU is still valuable.

## Implementation Plan

### Step 0: Confirm the threshold hypothesis (fast)

- Send ≥200-byte bursts via the raw TCP UART harness.
- If a large burst suddenly echoes but short inputs don’t, that strongly supports the “no RX timeout IRQ + high full threshold” hypothesis.

### Step 1: Implement the console workaround

- Update `imports/esp32-mqjs-repl/mqjs-repl/main/console/UartConsole.cpp`:
  - after `uart_driver_install()`, call `uart_set_rx_full_threshold(..., 1)`
  - optionally set `uart_set_rx_timeout(..., 1)`

### Step 2: Validate with scripts

Re-run:
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_tcp_raw.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_stdio.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_tcp_raw.sh` (raw TCP)

Expected: `:mode` yields `mode: repeat`, and `hello-*` echoes.

### Step 3: Validate on Cardputer (device loop)

- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh --port /dev/ttyACM0 --flash`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_uart_raw.py --port /dev/ttyACM0`

### Step 4: Keep as a regression contract

- Keep these scripts around as regression tests when we swap transports (UART → USB Serial/JTAG) and when we reintroduce JS evaluation.

## Open Questions

- If “RX FIFO full threshold = 1” still doesn’t deliver bytes under QEMU, is UART RX fundamentally unimplemented for the ESP32-S3 machine?
- If QEMU’s `-serial` is not wired to UART0 RX, which UART/transport is actually wired, and should we move REPL input accordingly for QEMU-only builds?

## References

- Intern research write-up: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/sources/intern-research-qemu-uart-results.md`
- Intern research guide: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/03-intern-research-guide-qemu-uart-rx-cardputer-console.md`
- Design for REPL split: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/design-doc/02-split-firmware-main-into-c-components-pluggable-evaluators-repeat-js.md`
- QEMU input bug tracking: `ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/index.md`
