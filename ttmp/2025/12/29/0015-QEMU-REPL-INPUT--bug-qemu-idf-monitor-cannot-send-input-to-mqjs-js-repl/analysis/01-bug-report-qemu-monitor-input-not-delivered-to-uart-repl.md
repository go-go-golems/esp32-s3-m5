---
Title: 'Bug report: QEMU monitor input not delivered to UART REPL'
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/build.sh
      Note: Reference build wrapper pattern used for mqjs build.sh
    - Path: imports/esp32-mqjs-repl/mqjs-repl/build.sh
      Note: Wrapper used to run idf.py qemu monitor reproducibly
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/main.c
      Note: REPL reads UART0 via uart_read_bytes and echoes chars; suspected RX path issue
    - Path: imports/esp32-mqjs-repl/mqjs-repl/partitions.csv
      Note: Defines SPIFFS storage partition needed to reach js>
    - Path: imports/qemu_storage_repl.txt
      Note: Original reference log showing expected boot-to-js> behavior
    - Path: imports/test_storage_repl.py
      Note: Potential bypass test (raw TCP to 5555) to validate RX without idf_monitor
    - Path: ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/reference/01-diary.md
      Note: Prior diary with QEMU bring-up + partition mismatch discovery
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T14:00:34.662304116-05:00
WhatFor: ""
WhenToUse: ""
---


# Bug report: QEMU monitor input not delivered to UART REPL

## Summary

The `mqjs-repl` firmware boots under ESP-IDF QEMU and prints the REPL prompt (`js>`), but **interactive input does not reach the firmware**: characters sent via `idf_monitor` (running under tmux) do not show up in the REPL and do not evaluate.

We can confirm input reaches **idf_monitor itself** (Ctrl+T Ctrl+H prints the monitor help), but normal characters do not appear to be forwarded to the emulated UART RX path that `uart_read_bytes()` is reading from.

## Scope / impact

- Blocks validating the “JS REPL works under QEMU” workflow.
- Blocks using QEMU as a fast dev loop while porting to Cardputer (and while iterating on REPL UX).

## Environment

- **Host**: Linux (manuel)
- **ESP-IDF**: 5.4.1 (`~/esp/esp-idf-5.4.1`)
- **idf_monitor**: 1.8.0 (as printed by the tool)
- **QEMU**: `qemu-system-xtensa` installed via `idf_tools.py install qemu-xtensa`
- **Firmware**: `imports/esp32-mqjs-repl/mqjs-repl`

## Repro steps (current best known)

1) Start monitor in tmux:

```bash
tmux new-session -s mqjs-qemu -c /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl \
  "unset ESP_IDF_VERSION; ./build.sh qemu monitor"
```

2) Wait for the firmware to print the banner and `js>` prompt.

3) Type `1+2` then Enter in the monitor (or programmatically via tmux send-keys). **Expected**: REPL echoes input and prints `3`. **Actual**: nothing visible; no echo, no result.

## Expected behavior

- Input typed into `idf_monitor` should be forwarded to the emulated UART RX.
- Firmware should echo characters (it calls `putchar(c)` for printable chars) and evaluate on CR/LF.

## Actual behavior

- Firmware reaches `js>` prompt (so UART TX path is working).
- Input does not appear to be received by `uart_read_bytes(UART_NUM_0, ...)` in `repl_task()`.
- `idf_monitor` hotkeys work (Ctrl+T Ctrl+H prints help), proving tmux keystrokes reach `idf_monitor` at least.

## Evidence / logs

- Captured QEMU monitor transcript (trimmed) in:
  - `sources/mqjs-qemu-monitor.txt`

Notably, it reaches:

- `mqjs-repl: Starting ESP32-S3 MicroQuickJS REPL with Storage`
- SPIFFS mounts/formats
- prints banner
- prints `js>`

but we have not captured any echoed input or evaluation output.

## Important prerequisite discovered during validation (SPIFFS partition mismatch)

During validation we initially saw:

- `SPIFFS: spiffs partition could not be found`

Root cause: the generated partition table binary did not include the `storage` partition, because the project’s `sdkconfig` was set to single-app partitions (`partitions_singleapp.csv`).

Once the generated partition table included the `storage` entry, the firmware booted to `js>` reliably under QEMU.

## Code points involved

### Firmware REPL loop

- `imports/esp32-mqjs-repl/mqjs-repl/main/main.c`
  - `repl_task()` reads via `uart_read_bytes(UART_NUM_0, ...)`
  - `process_char()` echoes via `putchar(c)` and triggers eval on `\\n` / `\\r`

### Build wrapper

- `imports/esp32-mqjs-repl/mqjs-repl/build.sh`
  - ensures ESP-IDF env is sourced when needed

### QEMU entrypoint

- `./build.sh qemu monitor`
  - QEMU uses `-serial tcp::5555,server`
  - `idf_monitor` connects to `socket://localhost:5555`

## Hypotheses (ranked)

1. **UART0 RX path isn’t wired in QEMU the way we assume**: TX works (logs), but RX might not reach the UART peripheral that `uart_read_bytes()` reads.
2. **Conflict between ESP-IDF console UART config and app’s UART0 driver usage**: although we use raw UART APIs, ESP-IDF console may configure UART0 in a way that interferes with RX (or the QEMU uart model only supports one “consumer”).
3. **Our tmux send-keys approach isn’t actually sending normal chars to the serial stream** (despite sending Ctrl+T/Ctrl+H working). We should validate by manually attaching to tmux and typing, and/or by using a raw TCP client (`test_storage_repl.py`) to send bytes directly to `localhost:5555`.

## Next debugging steps (fresh session)

1. **Manual interactive confirmation**:
   - `tmux attach -t mqjs-qemu`
   - type `1+2` Enter and see whether it echoes.

2. **Bypass idf_monitor**:
   - run `imports/test_storage_repl.py` against `localhost:5555` and confirm if RX works at all.

3. **Instrument RX**:
   - add logging around `uart_read_bytes()` return values and/or set a timeout to see if RX is stuck at 0.

4. **Try alternate transport**:
   - switch to `esp_console_new_repl_uart()` or use `esp_vfs_dev_uart_use_driver()` / VFS stdin approach to see if that changes RX behavior under QEMU.

5. **Check QEMU serial wiring**:
   - confirm which UART instance QEMU’s `-serial tcp::5555,server` maps to on esp32s3 machine model.

## Related tickets

- Prior work (now closed/blocked): `0014-CARDPUTER-JS` — port effort blocked until this QEMU/serial issue is understood.

