## Cardputer Tutorial 0011 — M5GFX Plasma Animation (ST7789 via M5GFX / LovyanGFX + ESP-IDF 5.4.1)

This tutorial is a **Cardputer** display demo built around **M5GFX autodetect** plus the **sprite/canvas** present path. The end goal is a full-screen plasma animation that avoids “flutter” by ensuring DMA transfers don’t overlap frame rendering.

### Why this exists

This is the Cardputer counterpart to AtomS3R `0010`:

- `0010-atoms3r-m5gfx-canvas-animation`: plasma reference using `M5Canvas` + `waitDMA()`

Key difference: Cardputer uses **ST7789** with a **visible window offset** and is easiest to bring up via **M5GFX autodetect** (board `board_M5Cardputer`).

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation && \
idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation && \
idf.py flash monitor
```

### Flash tip: use stable by-id path + conservative baud

If `/dev/ttyACM0` disappears/re-enumerates during flashing, use the stable by-id path:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation && \
idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' -b 115200 flash monitor
```

### Expected output (current scaffold)

On boot:

- The display shows a short solid-color bring-up sequence: **red → green → blue → white → black**
- Then a full-screen **plasma animation** starts
- Serial logs print `display width/height`, canvas allocation info, and an optional heartbeat

### Notes on M5GFX integration

This project reuses the vendored M5GFX component already present in this repo:

- `EXTRA_COMPONENT_DIRS` points at `../../M5Cardputer-UserDemo/components/M5GFX`


