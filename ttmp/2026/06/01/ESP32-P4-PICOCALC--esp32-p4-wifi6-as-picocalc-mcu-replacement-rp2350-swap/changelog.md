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

