# 0099 — ESP32-P4 PicoCalc display + keyboard smoke test

Lean PicoCalc peripheral firmware for the same-position RPico-to-Waveshare ESP32-P4-WIFI6 adapter.

No ESP-Hosted, no Wi-Fi, no HTTP server. This exists to iterate quickly on the PicoCalc keyboard and LCD without compiling the networking stack.

## Physical adapter pin mapping

Keyboard:

| PicoCalc / RPico net | ESP32-P4 GPIO |
|---|---:|
| GP6 / SDA | GPIO50 |
| GP7 / SCL | GPIO49 |

LCD:

| PicoCalc / RPico net | ESP32-P4 GPIO |
|---|---:|
| GP10 / LCD SCK | GPIO3 |
| GP11 / LCD MOSI | GPIO2 |
| GP12 / LCD MISO | not used |
| GP13 / LCD CS | GPIO7 |
| GP14 / LCD DC | GPIO24 |
| GP15 / LCD RST | GPIO25 |

Console remains the Waveshare CH343 USB-UART bridge on UART0 GPIO37/GPIO38.

## Build / flash / monitor

```bash
source ~/esp/esp-idf-5.4.2/export.sh
idf.py set-target esp32p4
idf.py build
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
lsof "$PORT" || true
idf.py -p "$PORT" flash monitor
```

## Console commands

```text
kbd status
kbd poll 10
kbd raw on
kbd raw off
lcd init
lcd fill red|green|blue|white|black
lcd bars
status
```
