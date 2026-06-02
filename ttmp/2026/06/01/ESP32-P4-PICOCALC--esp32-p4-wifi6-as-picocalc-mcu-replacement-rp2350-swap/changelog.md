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


## 2026-06-01

Optimized 0099 LCD throughput by selecting SPLL-backed 80 MHz SPI, using a 32 KiB internal DMA fill buffer, documenting the optimization guide, and benchmarking full-screen fills at 21 ms/frame (code commit 7bb4d1ac2554e894263b7fbce0c325777c389a08).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/main/app_main.c — 32 KiB DMA LCD fill optimization and benchmark output
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md — Analysis guide and task list for LCD throughput work


## 2026-06-01

Confirmed visually that the optimized 80 MHz LCD color-bar output is good on the physical PicoCalc display.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/reference/01-investigation-diary.md — Recorded operator visual confirmation of the optimized LCD baseline
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/tasks.md — Marked 80 MHz LCD visual-inspection task complete


## 2026-06-01

Added high-frequency LCD pattern tests and dirty-rectangle benchmarks to 0099; validated checker/stripe/diagonal patterns and rectbench commands at actual 80 MHz (code commit 9f7e979cf598f9f970242722f61b2c3a37b1e459).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/main/app_main.c — Pattern generation and dirty-rectangle benchmark commands
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md — Updated benchmark table and task list with pattern/rectbench results


## 2026-06-01

Added LCD terminal workload benchmarks to 0099 and measured cell, row, and scroll-style redraw performance at actual 80 MHz (code commit 1414dfd6cd1e676bfa37eb7cf0921e57fb8b676d).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/main/app_main.c — cellbench
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md — Updated terminal workload benchmark results and task status


## 2026-06-01

Added row-batched pseudo-text LCD benchmark to 0099, reaching about 21 full 40x20 pseudo-text screens/s at actual 80 MHz (code commit 749f254162cc774f23eac26dca8272ae8a4fe744).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/main/app_main.c — pseudo-glyph row batching and textbench command implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md — Updated pseudo-text benchmark results and task status


## 2026-06-01

Added repeatable LCD performance suite to 0099 with text render-vs-transfer timing and warning-free full-suite metrics at actual 80 MHz (code commit 5c4887abecdaeddd7d2b60d32915decd3ef8a42c).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/main/app_main.c — lcd perf suite and pseudo-text render/transfer timing
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md — Updated performance suite metrics and task status


## 2026-06-01

Updated the Obsidian deep-dive project report with the LCD performance experimentation and structured benchmark phase (vault commit cd3b3fd).

### Related Files

- /home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/06/01/ARTICLE - ESP32-P4-WIFI6 as PicoCalc MCU - Deep Technical Dive.md — Updated durable project report with LCD SPLL
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/reference/01-investigation-diary.md — Recorded project-report update and vault commit


## 2026-06-01

Expanded the LCD optimization backlog into explicit transfer, renderer, panel, runtime, stress-test, and hardware-routing tasks so queued DMA and related follow-ups stay tracked.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md — Phase-ordered backlog mirroring ticket tasks
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/tasks.md — Detailed LCD optimization backlog


## 2026-06-01

Implemented queued LCD pseudo-text row benchmark: polling text8x16 was 950 ms/20 screens; queued double-buffered rows were 568 ms/20 screens at actual 80 MHz (code commit e91b3e5).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/README.md — New queued benchmark command docs
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/main/app_main.c — Queued row-payload SPI transfer implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/04-picocalc-lcd-spi-throughput-optimization-guide.md — Queued benchmark result and follow-up backlog
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/tasks.md — Queued optimization task status and visual-confirmation follow-up

