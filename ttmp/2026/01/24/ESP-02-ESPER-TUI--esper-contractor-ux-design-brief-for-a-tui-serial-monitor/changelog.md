# Changelog

## 2026-01-24

- Initial workspace created
- Added contractor UX design brief for Esper TUI
- Added investigation diary capturing esper/ analysis

## 2026-01-24

Created full UX specification with ASCII wireframes and YAML widget pseudocode (reference/02-esper-tui-full-ux-specification-with-wireframes.md)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md — Complete UX spec with 5 screens


## 2026-01-24

Added Device Registry/Nickname UX to specification: enhanced Port Picker with nicknames, Assign Nickname dialog, Device Manager view

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-02-DEVICE-REGISTRY--device-registry-nicknames-for-serial-consoles/design-doc/01-device-registry-nickname-resolution.md — Source design for device registry feature

## 2026-01-24

Started implementation: added an initial Bubble Tea full-screen TUI skeleton (Port Picker + Monitor + Help overlay) in the nested `esper/` repo, and updated ticket tasks accordingly.

## 2026-01-25

Added `esper tail` (non-TUI serial pipeline streamer) design doc and implementation in the nested `esper/` repo (supports `--timeout` and output/decoder configuration flags).

## 2026-01-25

TUI: added an initial host-mode Inspector panel to surface decoder events (core dump, backtrace decode, GDB stub detection) while keeping host/device key routing safe.

## 2026-01-25

Added a UI test firmware playbook (`esp32s3-test`) describing how to flash and drive deterministic serial patterns (logs, partial lines, gdb stub packets, core dump markers, panic/backtrace) to exercise Esper UI states.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/playbooks/02-esper-ui-test-firmware-esp32s3-test.md — Flash/run/capture procedure and REPL trigger commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/firmware/esp32s3-test/README.md — Firmware capabilities and supported commands

## 2026-01-25

Extended the tmux screenshot harness to optionally auto-trigger UI test firmware REPL commands during capture (`FIRMWARE_TRIGGERS=...`), producing a consistent “rich events” output for Inspector and missing-screen work.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh — Added optional firmware trigger step + extra capture
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/playbooks/02-esper-ui-test-firmware-esp32s3-test.md — Documented `FIRMWARE_TRIGGERS` usage
