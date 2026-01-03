# Cardputer LVGL Demo (ESP-IDF + M5GFX)

Minimal LVGL bring-up on M5Stack Cardputer using:
- ESP-IDF (target `esp32s3`)
- `M5GFX` component (LovyanGFX) for display I/O
- `cardputer_kb` for keyboard matrix scanning (and a small key-event wrapper)

## Build (ESP-IDF 5.4.1)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo && \
idf.py set-target esp32s3 && idf.py build
```

## Flash + Monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo && \
idf.py flash monitor
```

## Host scripting + screenshot (esp_console)

This demo starts an `esp_console` REPL over **USB-Serial/JTAG**. You can type commands from a host terminal (via `idf.py monitor` or any serial terminal).

### Commands

- `help`
- `heap`
- `menu`
- `basics`
- `pomodoro`
- `console`
- `sysmon`
- `files`
- `palette`
- `setmins <minutes>`
- `keys <token...>` — inject LVGL keycodes remotely (lets you drive the UI without touching the device):
  - tokens: `up down left right enter tab esc bksp space`
  - tokens: `<single-char>` | `0xNN` | `<decimal>` | `str:hello`
- `screenshot` — emits a framed PNG over the same serial port:
  - `PNG_BEGIN <len>\n<raw png bytes>\nPNG_END\n`

### Capture PNG on host

Recommended (triggers the `screenshot` console command and captures the PNG):

```bash
python3 tools/capture_screenshot_png_from_console.py \
  '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' \
  screenshot.png
```

Capture-only (if you trigger `screenshot` manually in a terminal):

```bash
python3 tools/capture_screenshot_png.py \
  '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' \
  screenshot.png
```

Optional: validate the screenshot contents via OCR:

```bash
pinocchio code professional --images screenshot.png "OCR this screenshot and confirm it shows the expected LVGL UI (menu/title) for the current demo."
```

## Controls

### Menu

- `Up` / `Down`: select demo
- `Enter` / `Space`: open selected demo
- `Fn + \``: return to menu (from any demo)

### Basics demo

- Type into the text area (letters, digits, punctuation).
- `Tab`: move focus (textarea ↔ button)
- `Enter`: activate focused widget (e.g. press the button)
- `Fn + \``: return to menu

### Pomodoro demo

- `Space` / `Enter`: start / pause
- `R` / `Backspace`: reset
- `[` / `]`: -1 / +1 minute (when paused)
- `Fn + \``: return to menu

### Console demo

- Type a command and press `Enter`:
  - `help`, `heap`, `menu`, `basics`, `pomodoro`, `setmins <minutes>`
- `Fn + \``: return to menu

### Command palette (overlay)

- `Ctrl + P`: open/close palette
- Type to filter actions
- `Up` / `Down`: move selection
- `Enter`: run selected action
- `Esc`: close palette

### System Monitor demo

- Shows `heap`/`dma` trends + UI loop Hz and LVGL handler duration.
- `Fn + \``: return to menu

### Files demo (MicroSD)

- Mounts MicroSD at `/sd` (FATFS over SDSPI) and browses directories.
- `Up` / `Down`: select entry
- `Enter`: open directory / view file preview
- `Backspace`: go to parent directory / return from viewer
- `Fn + \``: return to menu

MicroSD wiring (Cardputer SDSPI reference):
- `MISO=GPIO39`, `MOSI=GPIO14`, `SCK=GPIO40`, `CS=GPIO12`
