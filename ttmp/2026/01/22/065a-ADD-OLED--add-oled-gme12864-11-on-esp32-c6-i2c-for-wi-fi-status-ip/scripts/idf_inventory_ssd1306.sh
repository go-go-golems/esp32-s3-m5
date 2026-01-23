#!/usr/bin/env bash
set -euo pipefail

# Inventory helper for ticket 065a-ADD-OLED.
#
# Goal: show "what ESP-IDF 5.4.1 already provides" for an SSD1306-class OLED:
# - esp_lcd panel driver (SSD1306)
# - I2C panel IO driver
# - reference test app that demonstrates init + draw_bitmap
#
# Usage:
#   ./idf_inventory_ssd1306.sh /home/manuel/esp/esp-idf-5.4.1

IDF_PATH="${1:-/home/manuel/esp/esp-idf-5.4.1}"

if [[ ! -d "$IDF_PATH" ]]; then
  echo "error: IDF_PATH not found: $IDF_PATH" >&2
  exit 2
fi

say() { printf '\n== %s ==\n' "$1"; }

say "SSD1306 panel driver"
ls -la "$IDF_PATH/components/esp_lcd/src/esp_lcd_panel_ssd1306.c"
ls -la "$IDF_PATH/components/esp_lcd/include/esp_lcd_panel_ssd1306.h"

say "I2C panel IO driver"
ls -la "$IDF_PATH/components/esp_lcd/include/esp_lcd_io_i2c.h"
ls -la "$IDF_PATH/components/esp_lcd/i2c/esp_lcd_panel_io_i2c_v2.c" || true
ls -la "$IDF_PATH/components/esp_lcd/i2c/esp_lcd_panel_io_i2c_v1.c" || true

say "I2C master (new driver)"
ls -la "$IDF_PATH/components/esp_driver_i2c/include/driver/i2c_master.h"

say "esp_lcd I2C SSD1306 test app (reference code)"
ls -la "$IDF_PATH/components/esp_lcd/test_apps/i2c_lcd/main/test_i2c_lcd_panel.c"

