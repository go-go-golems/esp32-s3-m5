# Changelog

## 2026-01-24

- Initial workspace created


## 2026-01-24

Documented ticket-local scripts policy: any new investigation scripts live under ttmp/.../scripts for traceability.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/README.md — Defines the policy and conventions


## 2026-01-24

Uploaded bundled ticket PDF to reMarkable via remarquee; added ticket-local upload script (and fixed diary prompt formatting for Pandoc/LaTeX).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/reference/01-diary.md — Step 3 upload + Pandoc fix
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/01-upload-to-remarkable.sh — Repeatable upload workflow


## 2026-01-24

Expanded design doc with screenshot framing case studies + detailed esp_idf_monitor parsing/decoding analysis; added ticket-local unified PNG capture script.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/design-doc/01-serial-monitor-as-stream-processor-parsing-multiplexing-screenshots.md — Main analysis expanded
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/reference/01-diary.md — Step 4 research writeup
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/02-capture-screenshot-png.py — Unified host capture script


## 2026-01-24

Re-uploaded ticket bundle to reMarkable using overwrite mode; updated upload script to support --force.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/reference/01-diary.md — Step 5 upload notes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/01-upload-to-remarkable.sh — Added force mode for overwriting existing reMarkable doc


## 2026-01-24

Added Go reimplementation design+implementation doc for idf.py-compatible colors + panic/core dump/GDB workflows; added ticket-local script to upload only this doc to reMarkable.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/design-doc/02-go-serial-monitor-idf-py-compatible-design-implementation.md — Defines requirements + phases for Go monitor
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/04-upload-go-monitor-design-doc-to-remarkable.sh — Uploads only the Go design doc


## 2026-01-24

Added Go monitor design+impl doc (idf.py-compatible colors + panic/core dump/GDB workflows) and uploaded only that doc to reMarkable via a ticket-local script.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/design-doc/02-go-serial-monitor-idf-py-compatible-design-implementation.md — Go design doc
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/reference/01-diary.md — Step 6
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/04-upload-go-monitor-design-doc-to-remarkable.sh — Single-doc upload


## 2026-01-24

Initialized new Go repo 'esper' (github.com/go-go-golems/esper) with cmd/esper and pkg/ layout; first commit 49bb9c1.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/README.md — Repo overview
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/cmd/esper/main.go — CLI entrypoint
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/monitor.go — Monitor runner scaffold
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/tasks.md — Updated task list


## 2026-01-24

Started implementation in new esper repo: serial stream processor skeleton (bubbletea + go.bug.st/serial) and added ESP32-S3 test firmware + tmux exercise script. Commits: 30b6353, e3d022d.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/firmware/esp32s3-test/main/main.c — Test firmware
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/monitor.go — Monitor skeleton
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/06-tmux-exercise-esper.sh — tmux workflow
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/tasks.md — Task status updated


## 2026-01-24

esper UI: added minimal Bubbles text input prompt for line-oriented esp_console workflows (commit f316013).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/monitor/monitor.go — Text input prompt + Enter sends line


## 2026-01-24

esper: added linux 'scan' subcommand to enumerate serial consoles and score likely ESP32/Espressif devices (commit f73e40a).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/cmd/esper/main.go — Added scan subcommand
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/scan/scan_linux.go — Linux sysfs + /dev/serial/by-id based device scoring
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/tasks.md — Marked scan task complete


## 2026-01-24

Added reference doc analyzing esptool.py chip/device identification (source-based, esptool 4.10.0) and uploaded it as a separate reMarkable PDF.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/reference/02-how-esptool-py-identifies-esp-devices-source-analysis.md — Source analysis doc
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/07-upload-esptool-identification-doc-to-remarkable.sh — Upload-only-this-doc script


## 2026-01-24

esper: added esptool-based chip probe for scan (--probe-esptool) to report authoritative chip identity/description/features (commit fa9ecf4).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/cmd/esper/main.go — scan flags + output
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/scan/probe_esptool.go — Calls esptool.cmds.detect_chip and emits JSON
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/esper/pkg/scan/scan_linux.go — Plumbs probe results into scan output
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/reference/01-diary.md — Step 15
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/tasks.md — Checked off probe task

