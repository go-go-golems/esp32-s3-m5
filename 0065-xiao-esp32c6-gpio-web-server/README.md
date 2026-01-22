# ESP32-C6 (XIAO) 0065 — GPIO Web Server (toggle D2/D3) + esp_console Wi‑Fi

Small ESP32‑C6 firmware that:

- starts an interactive `esp_console` REPL over **USB Serial/JTAG** (Wi‑Fi STA selection)
- once connected, starts `esp_http_server` and serves a tiny web UI that can toggle **D2** and **D3**

Defaults (XIAO ESP32C6 mapping):
- `D2 = GPIO2`
- `D3 = GPIO21`

Electrical defaults:
- D2 and D3 are treated as **active-low outputs** by default (logical “ON” drives the pin low).

## Build

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd 0065-xiao-esp32c6-gpio-web-server

idf.py set-target esp32c6
idf.py build
```

## Flash + Monitor

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

## Use

At the `c6>` prompt:

```text
wifi scan
wifi join 0 mypassword --save
wifi status
```

Then open the printed URL: `http://<sta-ip>/`.

## Configure (menuconfig)

`Component config` → `MO-065: GPIO web server`

- D2 GPIO number (default `2`)
- D3 GPIO number (default `3`)
- Active-low options (default `y`)
