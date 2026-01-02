# Cardputer keyboard scancode calibrator (0023)

Developer tool firmware for discovering the Cardputer keyboard matrix “scancodes” (physical `KeyPos` / vendor `keyNum`) and Fn-combos.

Ticket docs:
- `esp32-s3-m5/ttmp/2026/01/02/0023-CARDPUTER-KB-SCANCODES--cardputer-keyboard-scancode-calibration-firmware/`

## Build / flash

```bash
cd esp32-s3-m5/0023-cardputer-kb-scancode-calibrator
./build.sh build
./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash
```

Monitoring requires a TTY; use tmux:

```bash
./build.sh tmux-flash-monitor
```

## Collect chords (host helper)

If you want a machine-readable list of all unique chords you pressed:

```bash
python3 tools/collect_chords.py /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* --out chords.json
```
