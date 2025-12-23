## AtomS3R Tutorial 0014 — AnimatedGIF (bitbank2) single-GIF playback harness (ESP-IDF 5.4.1)

This tutorial is the **minimal harness** for ticket `0010-PORT-ANIMATED-GIF`: it brings up the AtomS3R GC9107 LCD via **M5GFX** and plays **one GIF** via the **AnimatedGIF** decoder.

### Scope / non-goals

- No serial console UI (that belongs to `0013-atoms3r-gif-console` / ticket 008)
- No “GIF directory” or asset pack format (ticket 008)
- First milestone is **embedded GIF bytes**; a later milestone may switch input to `esp_partition_mmap`

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single && \
idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single && \
idf.py flash monitor
```

### Notes on M5GFX integration

This project does **not** copy M5GFX into the tutorial directory. Instead it uses:

- `EXTRA_COMPONENT_DIRS` in `CMakeLists.txt` pointing at:
  - `M5Cardputer-UserDemo/components/M5GFX`


