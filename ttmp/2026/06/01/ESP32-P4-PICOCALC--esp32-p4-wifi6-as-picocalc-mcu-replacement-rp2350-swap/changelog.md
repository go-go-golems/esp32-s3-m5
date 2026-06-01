# Changelog

## 2026-06-01

- Initial workspace created


## 2026-06-01

Step 1: Ticket created, ChatGPT transcript captured, web research completed, 17 sources saved, primary design doc written with full pin mapping and firmware migration plan, 8 tasks added

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/pico-sdk-picocalc-wm/CMakeLists.txt — Current firmware build to be ported


## 2026-06-01

Step 2: Corrected GPIO availability — board has 25 header GPIOs (not 9). Initial count confused adsb-p4 project allocations with board-inherent constraints. Updated design doc with complete pin-by-pin header table.


## 2026-06-01

Confirmed Waveshare ESP32-P4-WIFI6 console path: /dev/ttyACM1 is the CH343 UART0 bridge (GPIO37/38), not native USB Serial/JTAG; controlled pyserial reset capture showed full boot, PSRAM, and app logs.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0097-esp32-p4-picocalc-bringup/sdkconfig.defaults — UART0 console configuration now matches the board's CH343 bridge
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/reference/01-investigation-diary.md — Recorded corrected serial-console analysis and successful capture


## 2026-06-01

Added NVS-backed Wi-Fi credential persistence to 0098 webserver; verified wifi save, reset reload, and /status saved=true (commit 84dd320).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0098-esp32-p4-wifi6-webserver/main/app_main.c — NVS credential load/save/clear implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/reference/01-investigation-diary.md — Recorded Step 5 credential persistence validation


## 2026-06-01

Committed the 0097 ESP32-P4 PicoCalc bring-up firmware source now that Phase 1 serial/PSRAM validation is recorded (commit 432aadd).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0097-esp32-p4-picocalc-bringup/main/0097-esp32-p4-picocalc-bringup.c — Phase 1 bring-up firmware source
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/reference/01-investigation-diary.md — Backfilled Step 3 code commit reference


## 2026-06-01

Added PicoCalc keyboard implementation guide and buildable ESP-IDF I2C diagnostic driver with kbd console commands

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0098-esp32-p4-wifi6-webserver/main/app_main.c — kbd console integration
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0098-esp32-p4-wifi6-webserver/main/picocalc_keyboard.c — Keyboard I2C driver
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/02-picocalc-keyboard-implementation-guide.md — Detailed keyboard guide


## 2026-06-01

Corrected PicoCalc keyboard physical adapter mapping to SDA GPIO50/SCL GPIO49 and validated kbd status ACK at address 0x1F

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0098-esp32-p4-wifi6-webserver/main/picocalc_keyboard.h — Corrected keyboard SDA/SCL GPIO constants
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/03-full-rpico-socket-to-waveshare-esp32-p4-pin-map.md — Full corrected physical adapter map
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/reference/01-investigation-diary.md — Validation diary entry


## 2026-06-01

Added lean 0099 display+keyboard firmware; validated keyboard status and LCD init/bars command path

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/main/app_main.c — Lean console
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/main/picocalc_keyboard.h — Keyboard GPIO constants copied from validated mapping
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/reference/01-investigation-diary.md — 0099 validation diary

