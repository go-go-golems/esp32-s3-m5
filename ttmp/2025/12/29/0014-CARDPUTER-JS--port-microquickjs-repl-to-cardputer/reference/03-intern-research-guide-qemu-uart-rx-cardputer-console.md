---
Title: 'Intern Research Guide: QEMU UART RX + Cardputer Console'
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh
      Note: tmux+idf_monitor QEMU harness used to reproduce the missing-input symptom
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_tcp_raw.sh
      Note: raw TCP UART harness to prove issue persists without idf_monitor
    - Path: ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/index.md
      Note: Primary tracking ticket for the QEMU interactive input failure
ExternalSources: []
Summary: Internet research guide for diagnosing missing UART RX under ESP-IDF QEMU (ESP32-S3) and documenting Cardputer console/input options.
LastUpdated: 2026-01-01T00:00:00Z
WhatFor: Help an intern find authoritative sources (docs/issues/PRs) to unblock QEMU interactive input and Cardputer console choices.
WhenToUse: When gathering external info before changing QEMU invocation, console transport, or Cardputer input implementation.
---


# Intern Research Guide: QEMU UART RX + Cardputer Console

## Goal

Help you search the internet for authoritative, up-to-date information to unblock ticket `0014-CARDPUTER-JS`, specifically:

1) Why **ESP-IDF QEMU (ESP32-S3)** prints UART output but does **not** deliver UART RX bytes to firmware (interactive input appears “dead”).
2) The correct ESP-IDF approach for **interactive console input** on ESP32-S3 (UART vs USB Serial JTAG vs USB-CDC), and what applies to **M5Stack Cardputer**.

Deliverables: a short write-up (links + evidence + “try this next”), and any concrete commands/config patches we can validate locally.

## Context

What we observe in this repo:

- Firmware now boots into a REPL-only “repeat” mode and prints `repeat>` reliably under QEMU.
- The REPL echoes each received byte immediately. If UART RX works, typing should visibly echo and `:mode` should print `mode: repeat`.
- Under QEMU, we see the prompt but **cannot get any echoed input** across multiple input paths:
  - `idf.py qemu monitor` (`idf_monitor` path)
  - raw TCP UART client (no monitor)
  - QEMU UART on stdio (no monitor)

This strongly suggests the bug is in **QEMU UART RX or QEMU/ESP-IDF wiring**, not in MicroQuickJS, SPIFFS, or our REPL.

Primary internal tracking:
- Ticket `0015-QEMU-REPL-INPUT` (the QEMU interactive input bug)

## Quick Reference

### Research questions (answer these with citations)

#### A) Is UART RX supported in ESP32-S3 QEMU today?
- Confirm from ESP-IDF docs or Espressif issues/PRs whether UART RX is implemented/working for `-M esp32s3`.

#### B) Which device/transport does ESP-IDF QEMU wire to `-serial`?
- UART0 vs UART1 vs “console” vs USB Serial JTAG emulation.
- If it’s not UART0, does `uart_read_bytes(UART_NUM_0, ...)` necessarily fail to receive bytes?

#### C) Does interactive input require using ESP-IDF console/VFS APIs instead of raw UART driver?
- Any known limitations where `uart_read_bytes()` RX doesn’t work under QEMU, but `esp_console`/VFS console does (or vice-versa).

#### D) Cardputer console/input path under ESP-IDF
- What’s the recommended console input/output transport on Cardputer:
  - USB Serial JTAG
  - USB-CDC
  - external USB-to-UART bridge
- What ESP-IDF drivers/APIs/configs are used for **RX** in that mode.

### Where to search (priority)

1) Espressif docs + GitHub (`espressif/esp-idf`) issues/PRs
2) Espressif-maintained QEMU repo/fork (if separate)
3) Forum posts/discussions that include exact versions + commands

### Search queries (copy/paste)

#### ESP-IDF QEMU UART RX / interactive input
- `ESP-IDF qemu esp32s3 uart rx`
- `qemu-system-xtensa esp32s3 uart input not working`
- `esp-idf idf.py qemu monitor cannot send input`
- `esp-idf qemu socket://localhost:5555 input`
- `ESP32-S3 QEMU uart_read_bytes no input`
- `espressif qemu esp32s3 serial rx`

#### ESP-IDF console transport under QEMU
- `ESP-IDF QEMU esp_console stdin`
- `ESP-IDF QEMU VFS console UART0`
- `idf_monitor qemu send keys stdin`

#### Cardputer / USB Serial JTAG
- `ESP32-S3 USB Serial JTAG interactive console`
- `ESP-IDF CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`
- `M5Stack Cardputer ESP-IDF USB Serial JTAG`
- `Cardputer ttyACM0 ESP-IDF monitor`

### Evidence checklist (what to record for each source)

- URL + date
- ESP-IDF version (e.g. v5.4.1)
- QEMU version/fork used
- target chip (esp32s3)
- exact QEMU invocation / `idf.py` command
- whether they confirm RX works and how they tested it
- if a fix exists: PR link + commit hash + what release contains it

### Local validation scripts (we already have)

tmux + `idf_monitor`:
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh`

UART-direct (bypass `idf_monitor`):
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_stdio.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_tcp_raw.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_uart_raw.py`

### Reporting template (use this format)

#### Finding N: <short title>
- Source: <url>
- Claim: <what they claim works / doesn’t work>
- Versions: IDF=<>, QEMU=<>, chip=<>
- Repro steps from source: <commands / configs>
- Why it matters to us: <tie to our symptoms>
- Proposed action for our repo: <what to try next>
- Confidence: high / medium / low

## Usage Examples

### Example “good outcome” #1: RX not supported (TX-only)

- You find an Espressif issue/PR stating ESP32-S3 QEMU UART RX is missing or broken.
- Proposed action: stop spending time on QEMU interactivity; use hardware (Cardputer) for interactive tests; keep QEMU for boot/output-only.

### Example “good outcome” #2: RX works but needs a different mapping

- You find a doc/issue that says UART RX is wired to a different UART/console device than UART0.
- Proposed action: update firmware console transport for QEMU builds, or change QEMU serial args accordingly.

### Example “good outcome” #3: Cardputer console path is the “real” interactive target

- You find that QEMU is limited, but USB Serial JTAG on ESP32-S3 supports RX properly and is the intended dev loop.
- Proposed action: implement `UsbSerialJtagConsole` and make it selectable for Cardputer builds; use the device test scripts to validate.

## Related

- Ticket index: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/index.md`
- Diary: `ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/01-diary.md`
- QEMU input bug: ticket `0015-QEMU-REPL-INPUT`
