---
Title: "ESP-02-ESPER-TUI: UI test firmware playbook (esp32s3-test)"
Ticket: ESP-02-ESPER-TUI
Status: active
Topics:
  - tui
  - firmware
  - esp-idf
  - esp-console
  - usb-serial-jtag
  - testing
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
  - esper/firmware/esp32s3-test/README.md
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/02-build-firmware.sh
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/03-flash-firmware.sh
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/05-run-esper-tui.sh
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/07-run-esper-tail-stdin-raw.sh
Summary: "How to flash and use the esper UI test firmware to force specific UI states (logs, partial lines, gdb stub detection, core dump markers, panic/backtrace)."
LastUpdated: 2026-01-25T14:35:27-05:00
---

# UI test firmware playbook (esp32s3-test)

This playbook uses the built-in esper UI test firmware to generate deterministic serial patterns that exercise esper UI states.

Firmware:
- `esper/firmware/esp32s3-test/`
- Key design point: console is **USB Serial/JTAG** (`esp_console`), so we can safely use the same port for monitor + REPL-style commands.

## 1) Build + flash firmware

From repo root:

- Build:
  - `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/02-build-firmware.sh`
- Flash:
  - `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/03-flash-firmware.sh`

## 2) Run esper

### Option A: TUI (preferred for UX work)

- `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/05-run-esper-tui.sh`

In DEVICE mode, type a command and press Enter to send it.

### Option B: tail + raw stdin (good for interactive REPL)

- `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/07-run-esper-tail-stdin-raw.sh`
- Exit key: `Ctrl-]` (always exits)

## 3) Commands to trigger UI states

These commands are provided by the firmware’s `esp_console` prompt (`esper> `):

- `help`
  - sanity check the REPL is alive

- `logdemo`
  - emits sample `ESP_LOGI/W/E` lines and a plain `printf`
  - exercises auto-color and general log rendering

- `partial`
  - emits a partial line without newline, then finishes later
  - exercises line splitting + tail finalize logic

- `gdbstub`
  - emits a valid `$T..#..` stop-reason packet (not a real session)
  - exercises GDB stub detection and inspector event logging

- `coredumpfake`
  - emits “Press Enter…” + CORE DUMP START/END markers + dummy base64-ish payload
  - exercises core dump recognizers and inspector event logging
  - note: full “core dump report” UI likely needs a real core dump flow/config, but this is a good first trigger

- `panic`
  - triggers `abort()` causing a panic/backtrace
  - exercises backtrace decode + inspector event logging

## 4) Screenshot capture (UX iteration loop)

- Deterministic virtual PTY capture (good for overlay/palette layout work):
  - `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`

- Real hardware capture (required for panic/core dump/backtrace realism):
  - `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`
  - then manually run the commands above while capturing additional screenshots as needed

### Optional: auto-trigger firmware commands during capture

If you want the capture harness to automatically drive the firmware REPL (real hardware only), set `FIRMWARE_TRIGGERS` (comma-separated). This will run the commands in DEVICE mode right after connect, then capture an extra `01b-device-triggered` screenshot.

Example (recommended order: put `panic` last because it reboots):
- `FIRMWARE_TRIGGERS=logdemo,partial,gdbstub,coredumpfake ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`
- `FIRMWARE_TRIGGERS=logdemo,gdbstub,coredumpfake,panic ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`
