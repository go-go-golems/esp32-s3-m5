# Agent Instructions (esp32-s3-m5)

## Console default: USB Serial/JTAG

Prefer the USB Serial/JTAG console for all interactive REPL work (`esp_console`) on Cardputer/ESP32-S3 projects, because UART pins are frequently repurposed for peripherals (keyboard/Grove/UART-to-NCP links) and UART console output can corrupt protocol traffic.

Recommended baseline in `sdkconfig.defaults` for firmwares that use a console:

```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
```

If you must use UART console, explicitly document which UART/pins are reserved and ensure it does not overlap with any protocol UART (e.g. host↔NCP SLIP link).

