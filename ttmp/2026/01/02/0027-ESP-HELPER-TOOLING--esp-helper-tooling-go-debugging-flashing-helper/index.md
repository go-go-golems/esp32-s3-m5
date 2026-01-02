---
Title: 'ESP helper tooling: Go debugging + flashing helper'
Ticket: 0027-ESP-HELPER-TOOLING
Status: active
Topics:
    - esp-idf
    - esp32
    - esp32s3
    - tooling
    - debugging
    - flashing
    - serial
    - tmux
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0013-atoms3r-gif-console/flash_storage.sh
      Note: parttool.py write storage partition
    - Path: 0019-cardputer-ble-temp-logger/tools/run_fw_flash_monitor.sh
      Note: explicit IDF python runner; checks esp_idf_monitor
    - Path: 0021-atoms3-memo-website/build.sh
      Note: older tmux split-pane flash+monitor pattern
    - Path: 0022-cardputer-m5gfx-demo-suite/build.sh
      Note: tmux + port auto-pick + explicit IDF python wrapper
    - Path: 0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py
      Note: serial framing parser for on-device PNG screenshots
    - Path: 0023-cardputer-kb-scancode-calibrator/build.sh
      Note: tmux + port auto-pick + explicit IDF python wrapper
    - Path: 0023-cardputer-kb-scancode-calibrator/tools/collect_chords.py
      Note: serial log parser for keyboard chord collection
    - Path: 0025-cardputer-lvgl-demo/build.sh
      Note: tmux + port auto-pick + explicit IDF python wrapper
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/flash_device_usb_jtag.sh
      Note: stable esptool flash using build/flash_args
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_device_uart_raw.py
      Note: device JS eval test bypassing idf_monitor
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_qemu_uart_stdio.sh
      Note: QEMU JS eval smoke test over UART stdio
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh
      Note: tmux send-keys smoke test for device monitor (+ optional flash)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_uart_raw.py
      Note: device UART raw test bypassing idf_monitor
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh
      Note: tmux send-keys smoke test for idf.py qemu monitor
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_stdio.sh
      Note: QEMU UART stdio test bypassing idf_monitor
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_tcp_raw.sh
      Note: QEMU UART raw TCP test bypassing idf_monitor
    - Path: ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/scripts/extract_idf_paths_from_compile_commands.py
      Note: Helper to extract IDF root from compile_commands.json for source-level debugging
    - Path: ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/reference/01-diary.md
      Note: Documents QEMU watchdog/version/environment sharp edges
    - Path: ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/scripts/01-run-qemu-tcp-serial-5555.py
      Note: Historical raw-TCP QEMU UART transcript capture probe
    - Path: ttmp/2025/12/29/0018-QEMU-UART-ECHO--track-c-minimal-uart-echo-firmware-to-isolate-qemu-uart-rx/scripts/02-run-qemu-mon-stdio-pty.py
      Note: Historical PTY-based QEMU mon:stdio transcript capture probe
    - Path: ttmp/2025/12/30/0019-BLE-TEST--ble-temperature-sensor-logger-on-cardputer/reference/02-diary.md
      Note: documents tmux/python env mismatch + esp_idf_monitor failure mode
    - Path: ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/scripts/flash_monitor_tmux.sh
      Note: older tmux monitor helper (bash -lc + export.sh)
    - Path: ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/scripts/pair_debug.py
      Note: operator workflow automation (idf.py + serial + plz-confirm)
    - Path: ttmp/2026/01/02/0027-ESP-HELPER-TOOLING--esp-helper-tooling-go-debugging-flashing-helper/design-doc/01-go-helper-tool-design-env-ports-flash-monitor-tmux.md
      Note: Design proposal for consolidated Go helper tool
ExternalSources: []
Summary: Inventory existing ESP-IDF helper scripts (tmux/serial/flash/monitor) and derive requirements for a consolidated Go-based debugging + flashing tool.
LastUpdated: 2026-01-02T09:01:10.021314819-05:00
WhatFor: Reduce friction and failure modes in build/flash/monitor workflows by consolidating environment, port selection, tmux, and stable flashing patterns into one Go tool.
WhenToUse: Use when repeated manual shell/tmux/serial steps are causing errors or slowing iteration (port selection, missing esp-idf-monitor, USB-JTAG flash flakiness, repeatable smoke tests).
---




# ESP helper tooling: Go debugging + flashing helper

## Overview

This ticket captures an inventory of the repo’s existing ESP-IDF helper scripts (especially tmux + serial console workflows) and turns the recurring patterns/failure modes into requirements for a Go-based helper tool focused on debugging + flashing ergonomics.

## Key Links

- Analysis: [Inventory: ESP-IDF helper scripts (tmux/serial/idf.py)](./analysis/01-inventory-esp-idf-helper-scripts-tmux-serial-idf-py.md)
- Design: [Go helper tool: design (env/ports/flash/monitor/tmux)](./design-doc/01-go-helper-tool-design-env-ports-flash-monitor-tmux.md)
- Diary: [Diary](./reference/01-diary.md)
- **Related Files**: See frontmatter `RelatedFiles`

## Status

Current status: **active**

## Topics

- esp-idf
- esp32
- esp32s3
- tooling
- debugging
- flashing
- serial
- tmux

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
