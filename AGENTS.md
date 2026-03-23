# Agent Instructions (esp32-s3-m5)

## Console default: USB Serial/JTAG

Prefer the USB Serial/JTAG console for all interactive REPL work (`esp_console`) on Cardputer/ESP32-S3 projects, because UART pins are frequently repurposed for peripherals (keyboard/Grove/UART-to-NCP links) and UART console output can corrupt protocol traffic.

Recommended baseline in `sdkconfig.defaults` for firmwares that use a console:

```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
```

If you must use UART console, explicitly document which UART/pins are reserved and ensure it does not overlap with any protocol UART (e.g. host↔NCP SLIP link).

## Serial ownership during flash/probe work

Treat each `/dev/tty*` device as single-owner during ESP-IDF flashing, monitoring, and scripted probing.

- Do not run multiple `idf.py flash`, `idf.py monitor`, `idf_monitor.py`, or custom serial probe scripts against the same serial device in parallel.
- Before starting a new flash/probe session, clear stale monitor holders for that exact port.
- If a serial experiment needs multiple commands, prefer a single boot session with one probe script instead of several competing processes.

This matters on the ESP32-S3 USB Serial/JTAG path because parallel access creates misleading failures:

- serial write timeouts
- missing or interleaved console output
- false prompt-detection failures
- confusing “crash” evidence that is actually port contention
