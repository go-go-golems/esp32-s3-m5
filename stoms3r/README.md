# SToMS3R — AtomS3R Lite Thermal Printer Console Firmware

SToMS3R ("Screw This, On My S3R") is an ESP-IDF firmware for the M5Stack
AtomS3R Lite (ESP32-S3-PICO-1-N8R8) that drives the M5Stack K118 thermal
printer kit through an interactive `esp_console` REPL over USB Serial/JTAG.

## Hardware

| Component | Details |
|-----------|---------|
| Controller | M5Stack AtomS3R Lite (ESP32-S3, 8MB Flash, 8MB PSRAM) |
| Printer | M5Stack K118 Thermal Printer Kit (58mm, 203dpi) |
| Connection | UART1 9600 8N1, TX=GPIO5, RX=GPIO6 |
| Console | USB Serial/JTAG (`/dev/ttyACM0`) |
| Power | USB-C for logic, 12V/2.5A for printer mechanism |

## Console Commands

### Printer
- `printer_init` — Reset printer
- `printer_text <text>` — Print a line of text
- `printer_feed [n]` — Feed n lines (default 3)
- `printer_size <0-7>` — Set font size
- `printer_bold <on|off>` — Bold on/off
- `printer_align <0|1|2>` — Left/center/right
- `printer_barcode <type> <data>` — Print barcode (CODE128, EAN13, ...)
- `printer_qr <text>` — Print QR code
- `printer_bitmap_test` — Print test pattern

### WiFi
- `wifi_scan` — Scan for access points
- `wifi_connect --ssid <s> --pass <p>` — Connect and save credentials
- `wifi_status` — Show connection state
- `wifi_disconnect` — Disconnect
- `wifi_forget` — Erase saved credentials

## Build / Flash / Monitor

```bash
./build.sh build
./build.sh /dev/ttyACM0 flash-monitor
```

Or manually:

```bash
source ~/esp/v5.4.x/esp-idf/export.sh
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Exit monitor: `Ctrl+]`
