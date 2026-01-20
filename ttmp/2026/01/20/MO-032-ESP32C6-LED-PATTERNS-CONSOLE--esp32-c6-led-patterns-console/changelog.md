# Changelog

## 2026-01-20

- Initial workspace created


## 2026-01-20

Added LED pattern engine + esp_console REPL design doc; imported patterns.jsx reference; uploaded PDF to reMarkable (/ai/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/design-doc/01-led-pattern-engine-esp-console-repl-ws281x.md — Primary design document for MO-032
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/sources/local/patterns.jsx — Imported simulator reference


## 2026-01-20

Implemented 0044 ESP32-C6 WS281x patterns firmware: driver wrapper, patterns engine, queue-driven animation task, and esp_console REPL; added build/flash smoke-test playbook.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c — Console verbs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.c — Queue-driven realtime task
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/main.c — Firmware entrypoint for MO-032
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/playbook/01-0044-build-flash-led-console-smoke-test.md — Validation procedure


## 2026-01-20

Made periodic loop/status logging opt-in; added `led log on|off` toggle and removed app_main status spam.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c — REPL toggle for logs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.c — Optional periodic status logging under task owner
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/main.c — Removed periodic loop/status logs

## 2026-01-20

Step 17: Add led help + pattern-aware led status; widen pattern speed validation to 1..255 (commit 6c1fa69).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c — Implement led help and improve status output
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c — Clamp chase step interval and define sparkle speed semantics
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.h — Update speed range documentation


## 2026-01-20

Step 18: Add tmux flash/monitor helper + smoke commands list for interactive parameter testing (commit 6e48255).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/scripts/led_smoke_commands.txt — copy/paste smoke command list
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/scripts/tmux_flash_monitor.sh — tmux helper to run idf.py flash monitor
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/playbook/01-0044-build-flash-led-console-smoke-test.md — document tmux workflow
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/reference/01-diary.md — step-by-step record


## 2026-01-20

Step 19: Rebuilt 0044 with ESP-IDF 5.4.1 (idf.py build ok) after REPL + tmux workflow changes.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c — Verified build after help/status changes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/scripts/tmux_flash_monitor.sh — Verified build after adding scripts
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/reference/01-diary.md — Build record


## 2026-01-20

Step 20: Updated design doc to match current REPL/message semantics and uploaded updated PDF to reMarkable (/ai/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/design-doc/01-led-pattern-engine-esp-console-repl-ws281x.md — Updated REPL semantics and queue protocol naming
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/reference/01-diary.md — Documented upload and validation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/tasks.md — Checked off remaining tasks


## 2026-01-20

Step 21: Flashed + monitored via tmux on /dev/ttyACM0; verified led chase set --speed 20 works and status/help/log toggles behave.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c — Verified help/status parsing
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/scripts/tmux_flash_monitor.sh — Used for flash/monitor
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/reference/01-diary.md — Recorded results


## 2026-01-20

Step 22: Added follow-up tasks for improved speed scaling, multi-train chase, ticket-local scripts, and a new analysis doc.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/reference/01-diary.md — Recorded follow-up plan
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/tasks.md — Added tasks 12-15


## 2026-01-20

Step 23: Moved tmux/smoke scripts into the ticket scripts/ folder and updated playbook references.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/playbook/01-0044-build-flash-led-console-smoke-test.md — Updated tmux usage
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/reference/01-diary.md — Recorded change
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/scripts/led_smoke_commands.txt — Ticket-local smoke command list
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/scripts/tmux_flash_monitor.sh — Ticket-local tmux flash/monitor helper


## 2026-01-20

Step 24: Tuned pattern speed semantics (rainbow RPM 0..20, breathing BPM 0..20, sparkle slower time-based spawn) and added multi-train chase with --gap/--trains (commit fd67b4a).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_console.c — Updated help/status/parser for new semantics
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.c — Speed mapping updates + multi-train chase + fixed sine wave
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.h — Config struct updates (gap/trains
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/reference/01-diary.md — Implementation record


## 2026-01-20

Step 25: Added analysis doc explaining speed scaling, chase train spacing, sparkle/breathing behavior, and brightness/power mapping.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/reference/01-diary.md — Recorded doc creation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/scripts/led_smoke_commands.txt — Script documented
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/scripts/tmux_flash_monitor.sh — Script documented


## 2026-01-20

Step 26: Uploaded speed/brightness analysis PDF to reMarkable (/ai/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE) and completed remaining task.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/analysis/01-speed-scaling-brightness-power-and-operator-scripts.md — Uploaded to reMarkable
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/reference/01-diary.md — Recorded upload
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/tasks.md — Task 15 checked

