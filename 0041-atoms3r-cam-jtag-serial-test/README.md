# AtomS3R-CAM JTAG Serial Test

Minimal ESP-IDF app that prints a 1 Hz tick on the USB Serial/JTAG console.

## Build

```bash
source ~/esp/esp-idf-5.1.4/export.sh
idf.py build
```

## Flash

```bash
idf.py -p /dev/ttyACM0 flash monitor
```
