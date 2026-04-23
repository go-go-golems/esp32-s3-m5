# ATOM Lite Printer Provisioning Firmware

Native ESP-IDF firmware for the M5Stack ATOM Thermal Printer Kit controller.

## Hardware

- Board/controller: M5Stack ATOM Lite
- MCU: ESP32-PICO-D4
- ESP-IDF target: `esp32`
- USB: FTDI UART bridge, usually `/dev/ttyUSB0`
- Printer: UART2, TX GPIO23, RX GPIO33, 9600 8N1
- Reset button: GPIO39 active-low, hold five seconds to erase NVS and reboot

## Build

```bash
source /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/.envrc
idf.py set-target esp32
idf.py build
```

## Flash and monitor

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

If the FTDI/autoreset path is unreliable:

```bash
idf.py -p /dev/ttyUSB0 -b 115200 flash monitor
```

Exit monitor with `Ctrl-]`.

## Provisioning

Use Espressif's **ESP BLE Provisioning** app:

- Transport: BLE
- Device name: logged as `M5PRN_XXXXXX`
- Security: Security 1
- PoP: `12345678`

After WiFi connects, the firmware prints a short status receipt with the assigned IP address.
