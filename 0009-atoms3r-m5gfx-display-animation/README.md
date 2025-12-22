## AtomS3R Tutorial 0009 — Display Animation (GC9107 via M5GFX / LovyanGFX + ESP-IDF 5.4.1)

This tutorial is the “same animation as 0008”, but implemented using **M5GFX (LovyanGFX)** instead of ESP-IDF `esp_lcd`.

### Why this exists

`0008` uses `esp_lcd` + `espressif__esp_lcd_gc9107`. During real hardware bring-up we hit cases where the panel init logs looked correct but the screen stayed black. `0009` is a controlled comparison: **same device, same pixel math**, but a different display stack (M5GFX) that is known to work in M5’s demos.

### Hardware wiring (AtomS3R)

Hard-coded in `main/hello_world_main.cpp`:

- `DISP_CS` = GPIO14
- `SPI_SCK` = GPIO15
- `SPI_MOSI` = GPIO21
- `DISP_RS/DC` = GPIO42
- `DISP_RST` = GPIO48

### Backlight (revision ambiguity)

This tutorial supports both common mechanisms:

- **Gate GPIO**: GPIO7 active-low (enable power to LED_BL via FET)
- **I2C brightness device**: addr `0x30`, reg `0x0e` on SCL=GPIO0 / SDA=GPIO45

These are configurable via `idf.py menuconfig` (`Tutorial 0009: Backlight`).

### Expected output

- The LCD shows a full-screen 128×128 **smooth, shifting color gradient** (same math as `0008`).
- Serial logs show:
  - backlight gate/I2C actions
  - M5GFX init parameters
  - a periodic heartbeat (configurable)

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation && \
idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation && \
idf.py flash monitor
```

### Notes on M5GFX integration

This project does **not** copy M5GFX into the tutorial directory. Instead it uses:

- `EXTRA_COMPONENT_DIRS` in `CMakeLists.txt` pointing at:
  - `M5Cardputer-UserDemo/components/M5GFX`

So `0009` stays small, but we reuse the exact M5GFX code we already vendor in the repo.

#### ESP-IDF 5.4.x compatibility note

On ESP-IDF 5.4.1, the vendored M5GFX code needed a small compatibility fix in:

- `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/common.cpp`

Specifically, ESP-IDF 5.x removed the `.module` field from `i2c_signal_conn_t`, so the code now maps I2C port number → `PERIPH_I2C{0,1}_MODULE` instead.


