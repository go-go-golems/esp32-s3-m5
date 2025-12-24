## AtomS3R Tutorial 0013 — “GIF Console” (real GIFs + button + `esp_console`) (ESP-IDF 5.4.1)

This tutorial is the “control plane + real payload” chapter: it plays **real GIFs** (decoded with **bitbank2/AnimatedGIF**) selected via **serial console commands** and a **hardware button**. GIF files are stored on a flash-bundled FATFS partition (`storage`) mounted at `/storage`.

- **Serial console control** (`esp_console`): `list`, `play`, `stop`, `next`, `info` (and optional `brightness`)
- **Button control**: press a GPIO button to go to the **next** GIF
- **Stable present**: M5GFX `M5Canvas` + `pushSprite()` + `waitDMA()`

### Why this exists

This complements the other AtomS3R chapters:

- `0008` (`esp_lcd`): native ESP-IDF LCD path
- `0009` (M5GFX full-frame blit): immediate-mode rendering
- `0010` (M5GFX canvas): known-good canvas + DMA present pattern

`0013` is the “control plane + playback engine” reference: **commands + button + playback state machine + decoder + flash-bundled assets**.

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

These are configurable via `idf.py menuconfig` (`Tutorial 0010: Backlight`).

### Expected output

- The LCD shows frames from the selected **GIF** (decoded on-device).
- Over serial console you can:
  - `list` to see GIF assets from `/storage/gifs`
  - `play <id|name>` to select one
  - `stop` to pause
  - `next` to cycle
  - `info` to view current state
- Pressing the configured button triggers `next` (debounced).

### Assets: where GIF files live

- Partition: `storage` (FATFS), defined in `partitions.csv`
- Mount point: `/storage`
- Directory scanned for GIFs:
  - preferred: `/storage/gifs`
  - fallback: `/storage` (for “flat” FATFS images)

### Build + flash a FATFS image with GIF files

This tutorial includes two helper scripts:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console && \
./make_storage_fatfs.sh && \
./flash_storage.sh /dev/ttyACM0
```

### Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console && \
idf.py set-target esp32s3 && idf.py build
```

### Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console && \
idf.py flash monitor
```

### Notes

This project does **not** copy M5GFX into the tutorial directory. Instead it uses:

- `EXTRA_COMPONENT_DIRS` in `CMakeLists.txt` pointing at:
  - `M5Cardputer-UserDemo/components/M5GFX`
  - `esp32-s3-m5/components` (symlinked `animatedgif` component)

So `0013` stays small, but we reuse the exact M5GFX code we already vendor in the repo.

### Button configuration

The “next animation” button defaults to `GPIO41`, but it is configurable via menuconfig:

- `Tutorial 0013: Input / Button`

#### ESP-IDF 5.4.x compatibility note

On ESP-IDF 5.4.1, the vendored M5GFX code needed a small compatibility fix in:

- `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/common.cpp`

Specifically, ESP-IDF 5.x removed the `.module` field from `i2c_signal_conn_t`, so the code now maps I2C port number → `PERIPH_I2C{0,1}_MODULE` instead.


