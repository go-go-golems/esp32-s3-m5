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

