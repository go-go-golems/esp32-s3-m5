# Changelog

## 2026-01-22

- Initial workspace created


## 2026-01-22

Scaffolded ticket + drafted OLED integration docs (wiring, esp_lcd SSD1306 plan, font strategy) and added reproducible research scripts.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/design-doc/01-oled-integration-wiring-esp-lcd-wi-fi-status-ip.md — Main integration design doc
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/playbook/01-bring-up-checklist-i2c-oled-gme12864-11.md — Bring-up checklist
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/reference/01-diary.md — Step-by-step diary
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/scripts/idf_inventory_ssd1306.sh — ESP-IDF SSD1306 inventory script
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/scripts/lookup_gme12864_11_sources.py — Research breadcrumb script


## 2026-01-22

Implemented optional SSD1306-class I2C OLED status display in 0065 firmware (commit 44acb37).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0065-xiao-esp32c6-gpio-web-server/main/Kconfig.projbuild — OLED config knobs (pins/address/speed/scan)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0065-xiao-esp32c6-gpio-web-server/main/app_main.c — Start OLED after wifi_mgr_start
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0065-xiao-esp32c6-gpio-web-server/main/oled_font_5x7.h — Bitmap font used for status text
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0065-xiao-esp32c6-gpio-web-server/main/oled_status.c — OLED init + status render task

