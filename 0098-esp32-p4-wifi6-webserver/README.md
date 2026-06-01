# 0098 — ESP32-P4-WIFI6 simple webserver

Simple networking experiment for the Waveshare ESP32-P4-WIFI6 board before the PicoCalc peripherals are connected.

The ESP32-P4 has no native Wi-Fi radio. On this board, networking runs through the onboard ESP32-C6 module over ESP-Hosted SDIO. The application uses normal `esp_wifi_*` calls, but `esp_wifi_remote` forwards those calls to the C6.

## Serial console

Use the CH343 USB-UART bridge, not native USB Serial/JTAG:

```bash
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
```

The bridge is wired to ESP32-P4 UART0 (GPIO37/GPIO38), so `sdkconfig.defaults` uses:

```text
CONFIG_ESP_CONSOLE_UART_DEFAULT=y
# CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG is not set
```

Before flashing or monitoring, make sure the serial port has only one owner:

```bash
lsof "$PORT" || true
```

## Build and flash

```bash
source ~/esp/esp-idf-5.4.2/export.sh
idf.py set-target esp32p4
idf.py build
idf.py -p "$PORT" flash
idf.py -p "$PORT" monitor
```

`idf.py monitor` must run in a real terminal or tmux pane. For non-interactive captures, use a pyserial reset-and-capture script instead.

## Network behavior

The app connects as a station using these default credentials:

```text
SSID:     yolobolo
Password: bring3248camera
```

When it receives an IP address, the serial log prints:

```text
Browse:  http://<device-ip>/
Status:  http://<device-ip>/status
```

Useful endpoints:

```text
GET /
GET /status
GET /api/ping
```

## esp_console commands

The app starts an `esp_console` REPL over the CH343 UART. In `idf.py monitor`, try:

```text
help
wifi status
wifi scan
wifi reconnect
wifi set <ssid> [password]
wifi set <ssid> [password] save
wifi save
wifi clear
wifi reconnect
kbd status
kbd poll 10
kbd raw on
kbd raw off
```

Credential behavior:

- On boot, the app first tries to load `ssid` and `password` from NVS namespace `wifi`.
- If no saved credentials exist, it falls back to the compiled defaults: `yolobolo` / `bring3248camera`.
- `wifi set <ssid> [password]` changes runtime credentials only.
- `wifi set <ssid> [password] save` changes runtime credentials and persists them.
- `wifi save` persists the current runtime credentials.
- `wifi clear` removes saved credentials from NVS; the current runtime connection remains active until changed, disconnected, or rebooted.

Keyboard diagnostics:

- The PicoCalc keyboard southbridge is expected at I2C address `0x1f`.
- `kbd status` reads register `0x04` and prints the raw FIFO/caps/num status.
- `kbd poll [limit]` drains up to `limit` pending `{state,key}` events.
- `kbd raw on` starts a background raw-event printer; `kbd raw off` stops it.
- The current adapter plan uses ESP32-P4 GPIO7 as SDA and GPIO8 as SCL at 10 kHz.

## Board-specific ESP-Hosted wiring

The C6 SDIO wiring comes from the Waveshare pinout source used in the PicoCalc ticket:

| ESP32-P4 GPIO | C6 / SDIO signal |
|---|---|
| GPIO18 | CLK |
| GPIO19 | CMD |
| GPIO14 | D0 |
| GPIO15 | D1 |
| GPIO16 | D2 |
| GPIO17 | D3 |
| GPIO54 | C6 reset / EN, active-high from P4 firmware view |

Do not copy Tab5 ESP-Hosted defaults blindly; Tab5 uses different SDIO pins and an active-low reset GPIO.
