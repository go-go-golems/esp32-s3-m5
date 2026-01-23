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

## Optional: OLED status display (I2C, SSD1306-class)

If you wire a 128×64 SSD1306-class OLED (e.g. GME12864-11 I2C variant), the firmware can show:
- Wi‑Fi state (IDLE/CONNECTING/CONNECTED)
- SSID (if set)
- IP address (when DHCP succeeds)

### Wiring (Seeed Studio XIAO ESP32C6)

- OLED `VCC` → `3V3`
- OLED `GND` → `GND`
- OLED `SDA` → XIAO pin4 (`GPIO22`)
- OLED `SCL` → XIAO pin5 (`GPIO23`)

### Configure (menuconfig)

`Component config` → `MO-065: OLED status display (SSD1306-class, I2C)`

- Enable OLED status display (default `n`)
- I2C SDA GPIO number (default `22`)
- I2C SCL GPIO number (default `23`)
- OLED I2C address (default `0x3C`)
