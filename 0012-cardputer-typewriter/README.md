## Cardputer Tutorial 0012 — Typewriter (keyboard → on-screen text via M5GFX + ESP-IDF 5.4.1)

This tutorial is a **Cardputer** “typewriter” app: **press keys on the built-in keyboard → text appears on the LCD**.

It intentionally reuses two known-good building blocks already present in this repo:

- **Display bring-up (M5GFX autodetect)**: see `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/`
- **Keyboard scan (GPIO matrix scan)**: see `esp32-s3-m5/0007-cardputer-keyboard-serial/`

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter && \
idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter && \
idf.py flash monitor
```

### Flash tip: use stable by-id path + conservative baud

If `/dev/ttyACM0` disappears/re-enumerates during flashing, use the stable by-id path:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter && \
idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' -b 115200 flash monitor
```

### Expected output (current scaffold)

On boot:

- The display shows a short solid-color bring-up sequence: **red → green → blue → white → black**
- Serial logs print `display width/height` and sprite allocation info



