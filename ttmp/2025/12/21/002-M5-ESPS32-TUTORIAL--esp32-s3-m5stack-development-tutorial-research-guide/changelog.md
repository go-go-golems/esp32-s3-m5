# Changelog

## 2025-12-21

- Initial workspace created


## 2025-12-21

Created comprehensive research guide for ESP32-S3 M5Stack development tutorial. Guide includes research methodology, codebase analysis checklist, example project specifications, and deliverables. Inspired by book structure debate from 001-FIRMWARE-ARCHITECTURE-BOOK.


## 2025-12-21

Step 2: created esp32-s3-m5/0001-cardputer-hello_world from ESP-IDF 5.4.1 hello_world and confirmed it builds for esp32s3.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0001-cardputer-hello_world/README.md — Documents the baseline build workflow


## 2025-12-21

Step 3: validated Cardputer baseline sdkconfig assumptions (8MB flash, custom partitions, 160MHz CPU) and updated tutorial 0001 to use sdkconfig.defaults + Cardputer-style partitions.csv; rebuilt successfully.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0001-cardputer-hello_world/sdkconfig.defaults — Encodes the validated defaults


## 2025-12-21

Step 4-5: tried building M5Cardputer-UserDemo on ESP-IDF 5.4.1. Fix 1: select TinyUSB component name by IDF major version. Fix 2: suppress -Werror=dangling-reference for main. Build still fails in PikaPython (PikaVM.c incompatible pointer types).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/main/CMakeLists.txt — IDF5 compatibility tweaks (TinyUSB + warning handling)


## 2025-12-21

Step 6: created esp32-s3-m5/0002-freertos-queue-demo (tasks + queue) and confirmed it compiles on ESP-IDF 5.4.1.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0002-freertos-queue-demo/README.md — Build/flash instructions
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0002-freertos-queue-demo/main/hello_world_main.c — Implements the FreeRTOS queue demo


## 2025-12-21

Step 8: created esp32-s3-m5/0003-gpio-isr-queue-demo (GPIO ISR -> FreeRTOS queue -> task) and confirmed it compiles on ESP-IDF 5.4.1 (added esp_driver_gpio dependency).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0003-gpio-isr-queue-demo/main/CMakeLists.txt — Requires esp_driver_gpio for driver/gpio.h
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0003-gpio-isr-queue-demo/main/hello_world_main.c — ISR->queue->task demo implementation


## 2025-12-21

Step 9: created esp32-s3-m5/0004-i2c-rolling-average (esp_timer callback -> poll queue -> I2C read task -> sample queue -> rolling avg) and confirmed it compiles on ESP-IDF 5.4.1.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0004-i2c-rolling-average/main/CMakeLists.txt — Requires esp_driver_i2c + esp_timer
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c — Implements I2C polling + rolling average pipeline


## 2025-12-21

Step 10: created esp32-s3-m5/0005-wifi-event-loop (WiFi STA + esp_event handlers) and confirmed it compiles on ESP-IDF 5.4.1.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0005-wifi-event-loop/main/Kconfig.projbuild — SSID/password settings
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0005-wifi-event-loop/main/hello_world_main.c — WiFi+event-loop implementation


## 2025-12-21

Security: scrubbed accidentally committed WiFi SSID/password from tutorial 0005 sdkconfig

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0005-wifi-event-loop/sdkconfig — Remove real SSID/password (set to empty)


## 2025-12-21

Tutorial 0006: WiFi STA + HTTP client (esp_http_client) added and builds on ESP-IDF 5.4.1

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0006-wifi-http-client/README.md — Build/flash instructions + HTTPS note
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0006-wifi-http-client/main/CMakeLists.txt — Component deps incl esp_http_client
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0006-wifi-http-client/main/Kconfig.projbuild — Menuconfig options for WiFi + HTTP URL/timeout/period
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0006-wifi-http-client/main/hello_world_main.c — WiFi connect + periodic HTTP GET task


## 2025-12-21

Tutorial 0007: Cardputer keyboard GPIO matrix scan with realtime serial echo (builds on ESP-IDF 5.4.1)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial/README.md — Usage + pin notes + schematic refs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial/main/Kconfig.projbuild — Configurable keyboard pins/scan/debounce
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c — Keyboard scanner + serial echo


## 2025-12-21

ATOMS3R display: rendered schematic to PNG + drafted questions for schematics specialist

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/reference/02-atoms3r-display-schematics-questions-for-specialist.md — Question checklist + known pin mapping


## 2025-12-21

ATOMS3R display: captured schematic pinout + backlight details (added Steps 14-15 to diary)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/reference/01-diary.md — Steps 14-15 recorded


## 2025-12-21

Tutorial 0008: ATOMS3R display animation (esp_lcd over SPI) added and builds on ESP-IDF 5.4.1

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation/README.md — Usage + wiring notes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation/main/Kconfig.projbuild — Config knobs for offsets/colorspace/clock
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation/main/hello_world_main.c — esp_lcd init + animation loop; backlight GPIO7 active-low


## 2025-12-22

Research: GC9107 on ESP-IDF (official esp_lcd_gc9107 component found) + new analysis doc + diary Step 17

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/analysis/01-gc9107-on-esp-idf-atoms3r-driver-options-integration-approaches-and-findings.md — New analysis doc
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/reference/01-diary.md — Added Step 17 research diary


## 2025-12-22

Tutorial 0008 updated for AtomS3R: GC9107 panel via esp_lcd_gc9107, default y_offset=32, I2C backlight option

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation/README.md — Update docs for AtomS3R/GC9107/backlight
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation/main/Kconfig.projbuild — Default y_offset=32 + backlight config
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation/main/hello_world_main.c — Use esp_lcd_gc9107 + AtomS3R backlight modes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0008-atoms3r-display-animation/main/idf_component.yml — Add espressif/esp_lcd_gc9107 dependency


## 2026-01-15

Ticket closed

