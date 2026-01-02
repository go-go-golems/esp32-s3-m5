# Changelog

## 2026-01-01

- Initial workspace created


## 2026-01-01

Created LVGL Cardputer demo plan + scaffolded 0025-cardputer-lvgl-demo (LVGL display+keyboard ports, initial textarea/button UI); build succeeds.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/app_main.cpp — Initial LVGL demo UI and loop
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp — LVGL flush to M5GFX + tick
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/01-lvgl-on-cardputer-esp-idf-m5gfx-integration-plan.md — Integration plan and API references
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/reference/01-diary.md — Research/implementation diary


## 2026-01-01

Flashed 0025-cardputer-lvgl-demo to Cardputer; demo UI appears to work on hardware (monitor not captured due to non-TTY harness).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/reference/01-diary.md — Step 3 flash smoke test notes


## 2026-01-01

Added demo menu + Pomodoro screen; refactored Basics into demo catalog; build+flash verified.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_manager.cpp — Screen switching + shared LVGL group
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_pomodoro.cpp — Pomodoro UI + timer + key controls
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/design-doc/01-lvgl-demo-menu-screen-architecture-cardputer.md — Design for menu/screen architecture


## 2026-01-01

Fix Fn+1 (Esc) chord reliability by prioritizing Back binding when keynum 2 is held with Fn.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/input_keyboard.cpp — Force Fn+1 to map to Action::Back (Esc)


## 2026-01-01

Fix Back/Esc chord mapping to Fn+` (keynum 29+1) and reduce task_wdt noise by blocking at least 1 tick per loop.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo/main/app_main.cpp — Use vTaskDelay(1) to ensure yielding
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h — Back/Esc binding corrected to keynum 1

