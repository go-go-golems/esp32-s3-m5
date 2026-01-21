---
title: Diary — MO-034
---

# Diary — MO-034 (ESP32-C6 LED Patterns Web UI)

## 2026-01-21

### Context / Problem Report

- Browser XHR requests to `/api/led/frame`, `/api/led/pause`, `/api/led/resume` were returning `404 Not Found`.
- Device was also crashing when the UI tried to set very small `frame_ms` values:
  - `assert failed: xTaskDelayUntil tasks.c:1476 (( xTimeIncrement > 0U ))`
  - Backtrace pointed at `0046-xiao-esp32c6-led-patterns-webui/main/led_task.c` inside `led_task_main`.
- Requirement: Web UI parameters should match `esp_console` (`led ...`) parameter semantics (RPM / LEDs/sec / breaths/min), not “make it faster by reducing frame_ms”.

### What Changed

#### 1) Fix low `frame_ms` crash (FreeRTOS tick clamp)

- Root cause: `pdMS_TO_TICKS(frame_ms)` can produce `0` when `frame_ms < portTICK_PERIOD_MS`.
- `vTaskDelayUntil(&last_wake, 0)` triggers the assert.
- Fix: normalize stored `frame_ms` to a value that is >= 1 RTOS tick and a multiple of the tick period.

Files:
- `0046-xiao-esp32c6-led-patterns-webui/main/led_task.c`

#### 2) Align HTTP API + UI with console parameter semantics

- Add per-pattern endpoints so the UI can set the same fields as the console commands:
  - `POST /api/led/rainbow` → `speed` (0..20), `sat` (0..100), `spread` (1..50)
  - `POST /api/led/chase` → `speed` (0..255), `tail` (1..255), `gap` (0..255), `trains` (1..255), `fg`, `bg`, `dir`, `fade`
  - `POST /api/led/breathing` → `speed` (0..20), `color`, `min` (0..255), `max` (0..255), `curve`
  - `POST /api/led/sparkle` → `speed` (0..20), `color`, `density` (0..100), `fade` (1..255), `mode`, `bg`
- Expand `GET /api/led/status` to include active-pattern parameters (using the same naming as console flags).
- Update the Preact UI to show per-pattern forms and use those endpoints (instead of trying to speed patterns up only via `frame_ms`).

Files:
- `0046-xiao-esp32c6-led-patterns-webui/main/http_server.c`
- `0046-xiao-esp32c6-led-patterns-webui/web/src/store/led.ts`
- `0046-xiao-esp32c6-led-patterns-webui/web/src/app.tsx`

### Build / Validation Notes

- Web bundle rebuild:
  - `./build.sh web`
- Firmware rebuild:
  - `./build.sh build`
- Result: build succeeds; app partition is tight (~1% free).

### Follow-ups

- Flash/hardware-validate:
  - Confirm no crash when setting `frame_ms` to values below the RTOS tick (status should report the normalized effective `frame_ms`).
  - Verify `/api/led/*` endpoints map 1:1 to console behavior for each pattern.
- Consider reducing flash usage (disable sourcemaps by default or gate behind Kconfig) to regain partition headroom.

