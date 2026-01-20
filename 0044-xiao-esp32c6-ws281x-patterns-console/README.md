# ESP32-C6 (XIAO) 0044 — WS281x Patterns + Console (ESP-IDF)

Goal: drive a WS2811/WS2812-style LED chain via RMT TX and control animations at runtime via an `esp_console` REPL over USB Serial/JTAG.

## Build

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console

idf.py set-target esp32c6
idf.py build
```

## Flash + Monitor

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Console backend: USB Serial/JTAG (see `sdkconfig.defaults`).

