# Tutorial 0077 - PaperS3 Alphabet Graffiti

This project is a standalone `PaperS3` handwriting app for `esp32-s3-m5`.

The planned product has two modes:

- `TRAIN`: record persistent Protractor templates for `A-Z` and `0-9`
- `WRITE`: draw single-stroke graffiti-style characters and append recognized output

This first task creates the new project shell, reuses the donor PaperS3 component stack, and shows a simple placeholder UI that already exposes the two-mode product direction.

## Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32s3
idf.py build
```

## Flash + Monitor

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```

## Planned Interaction Model

- `TRAIN` mode will let the user choose a glyph and save its template to persistent storage.
- `WRITE` mode will let the user draw characters even if not all glyphs are registered yet.
- Template storage will move to on-device disk in the next implementation step.
