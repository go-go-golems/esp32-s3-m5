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

### Expected output (current scaffold)

- On boot, the display should show a sequence of solid fills: **red → green → blue → white → black**
- Serial logs should print `display width/height` and a “scaffold OK” message

### Notes on M5GFX integration

This project reuses the vendored M5GFX component already present in this repo:

- `EXTRA_COMPONENT_DIRS` points at `../../M5Cardputer-UserDemo/components/M5GFX`


