---
Title: 'Context: QEMU REPL input broken; isolate with UART echo'
Ticket: 0018-QEMU-UART-ECHO
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - uart
    - serial
    - debugging
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/analysis/01-bug-report-qemu-monitor-input-not-delivered-to-uart-repl.md
      Note: Original bug report describing “js> prompt appears but input never arrives”
    - Path: ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/playbooks/01-debugging-plan-qemu-repl-input.md
      Note: Broader decision tree; this ticket implements Track C
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/main.c
      Note: REPL task reads UART0 with uart_read_bytes(UART_NUM_0,...)
    - Path: imports/test_storage_repl.py
      Note: TCP client used for bypass testing when QEMU uses -serial tcp::PORT,server
ExternalSources:
    - https://github.com/espressif/esp-idf/issues/9369
    - https://nuttx.apache.org/docs/latest/platforms/xtensa/esp32s3/index.html
Summary: ""
LastUpdated: 2025-12-29T15:09:28.549904941-05:00
WhatFor: ""
WhenToUse: ""
---

# Context: QEMU REPL input broken; isolate with UART echo

## Purpose

This ticket exists to isolate a critical question from the broader `0015-QEMU-REPL-INPUT` investigation:

> **Does ESP32-S3 QEMU deliver any UART RX bytes to ESP-IDF at all?**

The REPL firmware prints `js>` under QEMU (UART TX is working), but manual input does not echo or evaluate. Before we sink time into `idf_monitor`, tmux, MicroQuickJS, SPIFFS, or REPL parsing, we need a minimal “ground truth” firmware that tests UART RX in the smallest possible surface area.

## What we observed so far (from ticket 0015)

### Track A (manual typing): broken

Typing `1+2<enter>` into the QEMU console / monitor does not echo and does not evaluate. This rules out “tmux send-keys only” as the sole cause.

### Track B (bypass idf_monitor): blocked by serial backend mismatch

We attempted to run `imports/test_storage_repl.py` against `localhost:5555`, but the connection was refused. Later we confirmed why: the running QEMU command line was:

- `-serial mon:stdio`

So there is **no** TCP listener on port 5555 in that configuration; the bypass test will fail by design until QEMU is started with `-serial tcp::PORT,server`.

## Why Track C (minimal UART echo) is the right next step

The REPL firmware does multiple complex things (SPIFFS mount/format, file I/O, JS engine init, task creation, stdio echo). Any of these layers can hide the real failure mode. A minimal UART echo firmware lets us answer:

- If echo RX works in QEMU → the issue is higher level (REPL/console/monitor integration).
- If echo RX does not work in QEMU → the issue is likely QEMU UART RX limitation/bug or serial backend configuration, and the REPL app is not the problem.

## Requirements for the minimal firmware

- **Target**: ESP32-S3 (ESP-IDF).
- **UART**: Use UART0 (`UART_NUM_0`) at 115200 baud.
- **RX**: `uart_read_bytes()` with a finite timeout (e.g. 200–1000ms).
- **TX**: Echo bytes back with `uart_write_bytes()` (avoid reliance on stdio buffering).
- **Heartbeat**: Periodic “still alive / no RX” log line so we can tell the loop is running.
- **Console config**: Avoid USB-Serial-JTAG console settings (QEMU doesn’t emulate it).

## Deliverables

- A new ESP-IDF project directory under:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/`
  - **next to** `0016-atoms3r-grove-gpio-signal-tester/`
- A simple README with:
  - build commands
  - run-in-QEMU commands
  - how to test input via both `mon:stdio` and TCP serial

