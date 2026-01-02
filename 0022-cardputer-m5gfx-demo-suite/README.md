## Cardputer Demo Suite — M5GFX (LovyanGFX) Graphics Catalog (ESP-IDF 5.4.1)

This project is the code companion to ticket `0021-M5-GFX-DEMO`.

It provides a single firmware that acts like a “graphics demo catalog” for the M5 Cardputer:
- keyboard-first navigation
- reusable UI building blocks (list views, status bars, overlays)
- small demos that illustrate concrete M5GFX/LovyanGFX techniques

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite && \
idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite && \
idf.py flash monitor
```

### Controls (current scaffold)

- `W` / `S`: move selection up/down
- `Enter`: open (currently logs selection)
- `Del`: back (currently no-op)

