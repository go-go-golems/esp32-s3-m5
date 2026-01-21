# Changelog

## 2026-01-21

Align Web UI + HTTP API with `esp_console` LED parameters; fix low-frame crash

### Notes

- Add per-pattern config endpoints that accept the same fields/ranges as `led <pattern> set ...`:
  - `POST /api/led/rainbow` (`speed`, `sat`, `spread`)
  - `POST /api/led/chase` (`speed`, `tail`, `gap`, `trains`, `fg`, `bg`, `dir`, `fade`)
  - `POST /api/led/breathing` (`speed`, `color`, `min`, `max`, `curve`)
  - `POST /api/led/sparkle` (`speed`, `color`, `density`, `fade`, `mode`, `bg`)
- Expand `GET /api/led/status` to include active-pattern parameters (same naming as console flags).
- Update Preact UI to show per-pattern forms (RPM / LEDs/sec / breaths/min / sparkle controls).
- Fix `xTaskDelayUntil(..., 0)` assert by normalizing/clamping `frame_ms` to >= 1 RTOS tick.
- Add a detailed flash/partition/firmware-size walkthrough for XIAO ESP32C6, including measurement commands and optimization strategies.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/http_server.c — New `/api/led/<pattern>` endpoints + richer `/api/led/status`
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/led_task.c — `frame_ms` normalization to prevent `xTaskDelayUntil` assert
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/web/src/app.tsx — Per-pattern UI forms matching console semantics
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-034-ESP32C6-LED-PATTERNS-WEBUI--esp32-c6-led-patterns-web-ui-control-mo-032-0044/analysis/02-firmware-flash-size-partitions-and-optimization-xiao-esp32c6.md — Size/partitioning/optimization walkthrough

## 2026-01-20

- Initial workspace created


## 2026-01-20

Scaffold 0046 firmware+web UI, add minimal /api/led endpoints; bump httpd max_uri_handlers to prevent 404s

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/app_main.c — Starts LED task + Wi-Fi and starts HTTP server on got-IP
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/http_server.c — Registers /api/led/* endpoints and sets max_uri_handlers
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/web/src/store/led.ts — UI client calling /api/led/*
