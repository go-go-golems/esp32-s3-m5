---
Title: 'Debugging plan: QEMU REPL input (ESP32-S3, UART RX)'
Ticket: 0015-QEMU-REPL-INPUT
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - uart
    - console
    - serial
    - microquickjs
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/main.c
      Note: The REPL loop blocks in uart_read_bytes(UART_NUM_0, ..., portMAX_DELAY)
    - Path: imports/test_storage_repl.py
      Note: Raw TCP client to localhost:5555 to bypass idf_monitor input path
    - Path: imports/qemu_storage_repl.txt
      Note: Golden boot log showing js> prompt (TX path)
    - Path: ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/analysis/01-bug-report-qemu-monitor-input-not-delivered-to-uart-repl.md
      Note: Bug report + initial hypotheses
ExternalSources:
    - https://nuttx.apache.org/docs/latest/platforms/xtensa/esp32s3/index.html
Summary: ""
LastUpdated: 2025-12-29T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

# Debugging plan: QEMU REPL input (ESP32-S3, UART RX)

## Goal

Determine **where input bytes are getting lost** in the `idf.py qemu monitor` flow:

- Host keyboard → `idf_monitor` → TCP stream (`socket://localhost:5555`) → QEMU `-serial tcp::5555,server` → ESP32-S3 UART peripheral emulation → ESP-IDF UART driver → `uart_read_bytes()` in our app.

And produce a “known-good” workflow (or a documented limitation) for **interactive RX**.

## What we already know (evidence)

- The firmware boots in QEMU and prints the banner + `js>` prompt, so **TX works**.
- `idf_monitor` hotkeys work (Ctrl+T Ctrl+H), so keystrokes reach **idf_monitor**.
- The REPL task blocks in:
  - `uart_read_bytes(UART_NUM_0, ..., portMAX_DELAY)` — meaning **it is waiting for bytes forever**.
- The Manus “golden log” (`imports/qemu_storage_repl.txt`) shows `js>` but does **not** show an actual interactive eval transcript (e.g. no `1+2 → 3`), so “QEMU tested” should be treated as **unverified interactive RX** until we capture a transcript.

## Existing utilities in this repo

- **`imports/test_storage_repl.py`**: bypasses `idf_monitor` and writes directly to the QEMU TCP serial port.
- **`imports/qemu_storage_repl.txt`**: a “golden boot log” baseline (TX path only).
- **`imports/esp32-mqjs-repl/mqjs-repl/build.sh`**: wrapper to run `idf.py qemu monitor` reproducibly.
- **Ticket 0017 playbook**: `ttmp/.../0017.../playbook/01-bring-up-playbook-run-esp-idf-esp32-s3-firmware-in-qemu-monitor-logs-common-pitfalls.md`

## Investigation tracks (decision tree)

### Track A: Confirm baseline manual input (no automation)

**Why**: tmux `send-keys` can mislead (it can trigger monitor hotkeys while still not writing “normal” bytes the way you think).

**Do**
- Start QEMU + monitor the usual way (see “Commands”).
- Manually type in the monitor terminal:
  - `1+2` then Enter

**Record**
- Does the firmware echo characters?
- Does it ever print a result?

**Interpretation**
- **If it works manually**: the issue is likely **tmux automation**, terminal mode, or buffering.
- **If it still fails manually**: proceed to Track B/C.

### Track B: Bypass `idf_monitor` and send raw bytes to TCP:5555

**Why**: isolate whether the problem is in `idf_monitor` input handling vs QEMU UART RX vs firmware.

**Do**
- Run the bypass script while QEMU is running:
  - `python3 /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/test_storage_repl.py`

**Record**
- Full stdout from the script (including “Initial Output” and test command results).

**Interpretation**
- **If bypass works** (echo + results): the core UART RX path in QEMU likely works, and the bug is in **idf_monitor / terminal mode**.
- **If bypass also fails**: either **QEMU UART RX is broken/unsupported** for esp32s3, or the firmware isn’t reading from the serial device we think it is.

### Track C: Minimal UART echo firmware (no JS, no SPIFFS)

**Why**: isolate “UART RX doesn’t work at all” from “REPL/stdio/FreeRTOS interaction is broken”.

**Strategy**
- Create a tiny ESP-IDF app that:
  - installs UART0 driver
  - reads with `uart_read_bytes(..., timeout)`
  - echoes bytes back with `uart_write_bytes()`
  - logs periodic “still alive / no bytes” messages

**Expected outputs**
- When you type `abc<enter>`, you should see `abc` echoed (plus possibly CR/LF).

**Interpretation**
- **Echo works**: QEMU UART RX works; focus on **REPL app interaction** (stdio, console config, line endings).
- **Echo fails**: likely **QEMU RX limitation/bug** or wrong UART mapping.

### Track D: “Storage broke it” A/B (SPIFFS vs no SPIFFS)

**Why**: adding SPIFFS/autoload changed timing, memory, and console behavior; it might have regressed RX (or changed line ending handling).

**Strategy**
- Build two firmware variants:
  - **D1 (current)**: SPIFFS + autoload + REPL
  - **D2 (no storage)**: same UART REPL loop, but skip SPIFFS init + file IO + autoload

**Interpretation**
- **D2 works, D1 fails**: storage integration introduced the regression; focus on which part (SPIFFS init, file IO, malloc pressure, task scheduling).
- **Both fail**: storage is likely not the root cause.

### Track E: Console/UART ownership conflicts

**Why**: ESP-IDF console often uses UART0; re-installing a UART0 driver and reading directly might behave differently under QEMU than on hardware.

**Do**
- Inspect `sdkconfig`:
  - `CONFIG_ESP_CONSOLE_UART_NUM=0`
  - secondary console config (USB-Serial-JTAG)
- Try a firmware variant that reads via **stdin/VFS** instead of raw `uart_read_bytes()` (only as an experiment).

## Commands (copy/paste)

### Run QEMU + monitor (baseline)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl && \
unset ESP_IDF_VERSION && \
./build.sh qemu monitor
```

### Bypass `idf_monitor` (raw TCP test)

```bash
python3 /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/test_storage_repl.py
```

## Firmware instrumentation ideas (if we need more visibility)

These are “if stuck” additions; they should be done in a branch and kept minimal.

- Change `uart_read_bytes(..., portMAX_DELAY)` to a finite timeout (e.g. 200ms) and log:
  - “no bytes” heartbeat
  - `len` when bytes arrive
- Log UART driver state:
  - after `uart_driver_install`
  - `uart_get_buffered_data_len`
- Echo received bytes using `uart_write_bytes` (avoid relying on `putchar` / stdio buffering as a first diagnostic).

## What “done” looks like

- We can **reliably** send `1+2<enter>` and see:
  - input echoed
  - output `3`
- And we have a captured transcript (either from `idf_monitor` or the raw TCP client) stored under `sources/`.


