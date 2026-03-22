# Tutorial 0076 - PaperS3 Protractor Gesture Trainer

This project is a standalone `PaperS3` gesture-training app for `esp32-s3-m5`.

It reuses the vendored PaperS3 component stack from:

- `../../M5PaperS3-UserDemo/components/M5GFX`
- `../../M5PaperS3-UserDemo/components/M5Unified`

The app provides:

- a large touch drawing canvas
- Protractor preprocessing with resampling and vectorization
- eight tap-select template slots (`A` through `H`)
- `SAVE SLOT`, `DELETE SLOT`, `CLEAR STROKE`, and `RESET ALL` actions
- a recognition panel that updates after stroke release

## Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0076-papers3-protractor-trainer
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32s3
idf.py build
```

## Flash + Monitor

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0076-papers3-protractor-trainer
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

## Interaction Model

- Tap a slot on the right to choose where the template will be stored.
- Draw a gesture in the canvas.
- Release the finger to see recognition results.
- Tap `SAVE SLOT` to store the current gesture vector in the selected slot.
- Tap `DELETE SLOT` to erase the selected template.
- Tap `CLEAR STROKE` to clear the current gesture while keeping saved templates.
- Tap `RESET ALL` to wipe both the current stroke and all templates.

## Notes

- The project intentionally uses USB Serial/JTAG for the console.
- Live ink uses `epd_fast`.
- Full UI redraws and recognition overlays use `epd_text`.
