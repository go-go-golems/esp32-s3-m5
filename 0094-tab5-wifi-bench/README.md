# Tab5 UI Screen Viewer

A firmware for the M5Tab5 that runs a Wi-Fi webserver and accepts uploaded images via HTTP, blitting them fullscreen onto the 720×1280 MIPI DSI display. Designed for previewing UI mockups on actual hardware without reflashing.

## Build

```bash
cd 0093-tab5-ui-screen-viewer
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
```

## Flash + monitor

```bash
idf.py -p /dev/ttyACM1 flash monitor
```

## Console Wi-Fi setup

The firmware starts an `esp_console` REPL on the USB serial/JTAG port. Useful commands:

```bash
wifi status
wifi scan
wifi set "YourSSID" "YourPassword" save
wifi connect
wifi clear
```

The saved SSID/password are stored in NVS, so the board will try to rejoin your network on the next boot.

## What it does

- Starts a Wi-Fi SoftAP named `Tab5-UI-Viewer` (password: `tab5viewer`) for recovery/setup
- Tries to join a saved Wi-Fi network as STA when credentials exist
- Serves a web UI at `http://<ip>/` with drag-and-drop image upload
- Accepts raw RGB565 pixel data via `POST /api/upload` (1280×720×2 = 1,843,200 bytes)
- Browser-side JS converts any image to 1280×720 RGB565 and uploads it
- `POST /api/clear` fills the screen with black
- `GET /api/screen` returns resolution and format info

## Architecture

- **MCU**: ESP32-P4, WiFi via ESP32-C6 slave (ESP-Hosted SDIO)
- **Display**: 720×1280 portrait panel, ST7123 MIPI DSI, rotated to 1280×720 landscape
- **Color format**: RGB565, 16-bit, little-endian
- **Frame buffer**: Full-screen SPIRAM buffer (1.76 MB), LVGL `lv_image_dsc_t` with `LV_IMG_CF_TRUE_COLOR`
