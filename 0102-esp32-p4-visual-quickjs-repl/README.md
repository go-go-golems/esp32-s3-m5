# 0102 — ESP32-P4 Visual QuickJS REPL

Ticket: `ESP32-P4-VISUAL-QUICKJS-REPL`.

This firmware will combine the PicoCalc LCD/keyboard work from 0099 with the native QuickJS service from 0101. The first skeleton initializes the extracted components and provides a UART debug console while the visual REPL model and renderer are built.

## Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py set-target esp32p4
idf.py build
```

## Flash / monitor

Use a single owner for `/dev/ttyACM0`.

```bash
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 monitor
```

Expected early skeleton output includes LCD init, keyboard init, QuickJS service init, and a `0102>` UART debug prompt.
