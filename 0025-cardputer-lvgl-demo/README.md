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

## Controls

- Type into the text area (letters, digits, punctuation).
- `Enter`: activate focused widget (e.g. press the button).
- `Tab`: move focus to the next widget.
- Fn-chord navigation (if captured by the keyboard binding decoder): Up/Down/Left/Right.

