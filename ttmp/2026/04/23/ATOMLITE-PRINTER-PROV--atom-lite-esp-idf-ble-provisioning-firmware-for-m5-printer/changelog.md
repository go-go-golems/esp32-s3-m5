# Changelog

## 2026-04-23

- Initial workspace created


## 2026-04-23

Created ATOM Lite ESP-IDF BLE provisioning firmware under 0092, documented build/flash flow, and validated idf.py build for target esp32.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/app_printer.c — Printer UART support
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/main.c — Provisioning and WiFi event flow
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/23/ATOMLITE-PRINTER-PROV--atom-lite-esp-idf-ble-provisioning-firmware-for-m5-printer/design-doc/01-implementation-guide.md — Implementation and validation guide


## 2026-04-23

Recorded implementation commit 2b74d824e25c9ab59ecaf8cab7bfcdb6a14589e7 for the ATOM Lite ESP-IDF provisioning firmware.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/main.c — Committed implementation entry point
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/23/ATOMLITE-PRINTER-PROV--atom-lite-esp-idf-ble-provisioning-firmware-for-m5-printer/reference/01-diary.md — Diary now references implementation commit


## 2026-04-23

Switched 0092 build instructions to source the repo .envrc and revalidated the firmware with ESP-IDF 5.4.1.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc — Canonical ESP-IDF 5.4.1 environment for this repo
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/README.md — Updated build/flash instructions
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/23/ATOMLITE-PRINTER-PROV--atom-lite-esp-idf-ble-provisioning-firmware-for-m5-printer/reference/01-diary.md — Recorded ESP-IDF 5.4.1 rebuild


## 2026-04-23

Hardware flash attempt reached ESP32-PICO-D4 ROM stub but failed after baud switch to 460800 with 'Unable to verify flash chip connection'; likely serial speed/stub issue rather than wrong bootloader.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/README.md — Flash troubleshooting commands should mention low baud/no-stub fallback
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/23/ATOMLITE-PRINTER-PROV--atom-lite-esp-idf-ble-provisioning-firmware-for-m5-printer/reference/01-diary.md — Diary should record first hardware flash failure


## 2026-04-23

Added production PoP and iPhone provisioning analysis guide, covering unique per-device PoP, QR onboarding, Espressif iOS SDK flow, manufacturing storage, and the current post-provisioning watchdog finding.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0092-m5-printer-esp-idf-provision/source/atomlite-printer-prov/main/main.c — Development PoP and event loop references
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/23/ATOMLITE-PRINTER-PROV--atom-lite-esp-idf-ble-provisioning-firmware-for-m5-printer/analysis/01-production-pop-and-iphone-provisioning-analysis.md — Detailed production provisioning analysis


## 2026-04-23

Wrote durable production provisioning analysis and Obsidian project report; recorded successful phone provisioning context and post-provisioning watchdog follow-up.

### Related Files

- /home/manuel/code/wesen/obsidian-vault/Projects/2026/04/23/ARTICLE - ATOM Lite ESP-IDF Provisioning - Project Report.md — Textbook-style Obsidian project report
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/23/ATOMLITE-PRINTER-PROV--atom-lite-esp-idf-ble-provisioning-firmware-for-m5-printer/analysis/01-production-pop-and-iphone-provisioning-analysis.md — Production PoP and iPhone provisioning guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/04/23/ATOMLITE-PRINTER-PROV--atom-lite-esp-idf-ble-provisioning-firmware-for-m5-printer/reference/01-diary.md — Diary records documentation step and watchdog finding

