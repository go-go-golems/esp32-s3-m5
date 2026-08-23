# Changelog

## 2026-08-23

- Initial workspace created


## 2026-08-23

Created ticket + intern design/implementation guide for CoreS3 + Module13.2 QRCode scanner. Identified module as Module13.2 QRCode (M145) from the user's 12 V + CoreS3-expansion facts; gathered official protocol PDF, ZBarcode user guide, M14-Pro datasheet, product pinmap, and the official M5Module-QRCode Arduino library into sources/. Wrote host probe script (01-probe-qrcode-uart.py) and build/flash helper (02-bringup-build-flash.sh). Design doc: 12 sections incl. hardware block diagram, full UART protocol with command tables + pseudocode, M5Unified/M5GFX-on-ESP-IDF architecture, 4 decision records, 5-phase plan. Diary in reference/01-diary.md.

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/design-doc/01-cores3-module13.2-qrcode-scanner-analysis-design-and-implementation-guide.md — Primary deliverable


## 2026-08-23

Uploaded design doc + diary + sources manifest bundle to reMarkable at /ai/2026/08/23/ESP-62-CORES3-QRCODE; verified via remarquee cloud search. Doctor passes. Commits b49b37f4, e07cce39.

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/23/ESP-62-CORES3-QRCODE--esp-idf-cores3-module13-2-qrcode-barcode-qr-scanner-with-on-screen-display-intern-guide/design-doc/01-cores3-module13.2-qrcode-scanner-analysis-design-and-implementation-guide.md — Uploaded deliverable


## 2026-08-23

Implemented firmware 0118-cores3-qrcode-scanner (Phases 1-4): skeleton+display, scanner driver (UART+PI4IOE5V6408), on-screen UI, full qr console. Builds clean on IDF 5.3.4; fullclean reproducible (515KB app). Commits 9981d58f, bd0e0a5b, 79eeb454, 52d01d75. Flash + live probe blocked on host loading cdc_acm (no passwordless sudo).

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0118-cores3-qrcode-scanner/main/app_main.cpp — Boot wiring


## 2026-08-23

Step 12: isolated no-scan state to zero scanner UART RX across all ACK commands, documented baud rates, and safe QRCode DIP routes; added observable diagnostics and nonblocking UI (commits 66ed582d, 8604fd71, 6e28274c, 888a0f66)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0118-cores3-qrcode-scanner/main/qr_engine.cpp — Command TX and ACK result instrumentation
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0118-cores3-qrcode-scanner/main/qr_module.cpp — Owner-mediated power, trigger, baud, route, and electrical diagnostics


## 2026-08-23

Step 13: added separate minimal direct CoreS3 QRCode probe and operator confirmed scanning works with H2 removed (commit fe21fc26)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0119-cores3-qrcode-minimal-probe/main/app_main.cpp — Known-good minimal implementation


## 2026-08-23

Step 14: proved scanner was persisted in USB mode, recovered TTL serial by scanning official 21424000 programming barcode, and captured firmware 1.0 + X0052L3WPN raw bytes + emit + UI delivery

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0118-cores3-qrcode-scanner/main/qr_module.cpp — Safe trigger sequencing, owner-mediated pulse, and raw UART evidence
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0118-cores3-qrcode-scanner/main/qr_ui.cpp — Hardware trigger fallback and UART recovery instructions

