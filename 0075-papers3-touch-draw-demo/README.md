# Tutorial 0075 - PaperS3 Touch Draw Demo

This project is a small standalone `PaperS3` drawing demo for `esp32-s3-m5`.

It uses the vendored display and GT911 touch stack from:

- `../M5PaperS3-UserDemo/components/M5GFX`
- `../M5PaperS3-UserDemo/components/M5Unified`

The firmware boots into a simple screen with:

- a bordered drawing canvas
- single-finger freehand drawing inside the canvas
- a touch `CLEAR` button that redraws the screen cleanly

## Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0075-papers3-touch-draw-demo
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32s3
idf.py build
```

## Flash + Monitor

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0075-papers3-touch-draw-demo
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

## Notes

- The project intentionally prefers USB Serial/JTAG for the console.
- Live pen updates use `epd_fast` for responsiveness.
- Full-screen redraws, including `CLEAR`, use `epd_text` for cleaner UI rendering.
