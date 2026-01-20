# ESP32-C6 (XIAO) 0042 — D0 GPIO Toggle (ESP-IDF 5.4.1)

This firmware toggles a configurable GPIO output on an **ESP32-C6** target. It defaults to the **Seeed Studio XIAO ESP32C6** board mapping **D0 = GPIO0**.

Notes:
- Toggling a bare GPIO pin is easiest to observe with a scope/logic analyzer, or by driving an external LED + resistor.
- If you want a “visible” default on XIAO ESP32C6, the on-board user LED is on **GPIO15** and is **active-low** (set GPIO to `15` and enable active-low in `menuconfig`).

## Build

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0042-xiao-esp32c6-d0-gpio-toggle

idf.py set-target esp32c6
idf.py menuconfig
idf.py build
```

## Flash + Monitor

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0042-xiao-esp32c6-d0-gpio-toggle

idf.py -p /dev/ttyACM0 flash monitor
```

## Configure (menuconfig)

`menuconfig` path:

`Component config` → `MO-030: ESP32-C6 D0 GPIO toggle`

- `GPIO number to toggle` (default `0` for XIAO D0)
- `Active-low output` (default `n`)
- `Toggle period (ms)` (default `500`)

