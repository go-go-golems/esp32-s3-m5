# AtomS3R-CAM Camera Init Test

Minimal ESP-IDF app that initializes the camera and logs frame metadata over
USB Serial/JTAG. No Wi-Fi or USB UVC is included.

## Build

```bash
source ~/esp/esp-idf-5.1.4/export.sh
idf.py build
```

## Flash

```bash
idf.py -p /dev/ttyACM0 flash monitor
```
