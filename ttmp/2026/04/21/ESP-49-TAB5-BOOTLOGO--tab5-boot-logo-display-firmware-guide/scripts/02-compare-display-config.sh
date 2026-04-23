#!/usr/bin/env bash
set -euo pipefail

ROOT="/home/manuel/workspaces/2025-12-21/echo-base-documentation"
OURS="$ROOT/esp32-s3-m5/0051-tab5-boot-logo/sdkconfig"
REF="$ROOT/M5Tab5-UserDemo/platforms/tab5/sdkconfig"

show_keys() {
  local file="$1"
  echo "== $file =="
  rg -n "CONFIG_SPIRAM_SPEED_|CONFIG_CACHE_L2_CACHE_|CONFIG_CACHE_L2_CACHE_LINE_|CONFIG_COMPILER_OPTIMIZATION_|CONFIG_FREERTOS_HZ=|CONFIG_BSP_LCD_COLOR_FORMAT|CONFIG_BSP_DISPLAY_LVGL_AVOID_TEAR|CONFIG_LV_COLOR_SCREEN_TRANSP" "$file" -S || true
  echo
}

show_keys "$OURS"
show_keys "$REF"
