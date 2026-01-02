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

### Menu

- `Up` / `Down`: select demo
- `Enter` / `Space`: open selected demo
- `Esc`: return to menu (from any demo)

### Basics demo

- Type into the text area (letters, digits, punctuation).
- `Tab`: move focus (textarea ↔ button)
- `Enter`: activate focused widget (e.g. press the button)
- `Esc`: return to menu

### Pomodoro demo

- `Space` / `Enter`: start / pause
- `R` / `Backspace`: reset
- `[` / `]`: -1 / +1 minute (when paused)
- `Esc`: return to menu
