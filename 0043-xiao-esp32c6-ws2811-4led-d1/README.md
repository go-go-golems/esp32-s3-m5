# ESP32-C6 (XIAO) 0043 — WS2811 (4 LEDs) on D1 (ESP-IDF 5.4.1)

Drives a small WS2811 1-wire LED chain (RGB) using the ESP-IDF RMT TX driver.

Defaults:
- XIAO ESP32C6 **D1 = GPIO1** (data output)
- `LED_COUNT = 50`
- Timing defaults are set for a typical **WS2811 800kHz**-style protocol, but are configurable via `menuconfig`.
- Pattern: rotating rainbow (GRB byte order).

Tuning:
- `Animation frame time (ms)` controls smoothness (default 16ms).
- Intensity is modulated between `Intensity min (%)` and `Brightness (%)`.

## Wiring (typical)

- XIAO `GND` → strip `GND`
- External `5V` → strip `5V`
- XIAO `D1` → strip `DAT` (prefer a 5V level shifter)

Recommended:
- `74AHCT125` (5V) level shift for `DAT`
- 220–470Ω series resistor on `DAT` near strip input
- 470–1000µF bulk cap across 5V/GND at strip input

## Build

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1

idf.py set-target esp32c6
idf.py menuconfig
idf.py build
```

## Flash + Monitor

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1

idf.py -p /dev/ttyACM0 flash monitor
```

## Configure (menuconfig)

`Component config` → `MO-031: WS2811 (RMT) test`
