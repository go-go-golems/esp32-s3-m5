# Cell D operator observations — 2026-07-14

## Configuration

- Matrix cell: D
- ESP-IDF: 5.4.2
- M5GFX: 0.2.25 (`ad9b814264d4e2000e9f30070002310bbccaffc9`)
- M5Unified: 0.2.18 (`b1ffcc677014ed8bd01e5a1f240736ae654bfe12`)
- Board USB identity: `Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC`

## Current controlled test

- Cell D flashed successfully at 115200 baud.
- Boot autodetected PaperS3, one 960×540 EPD, and 8 MiB PSRAM; initial heap integrity passed.
- Full-screen TEXT white followed by full-screen TEXT black completed automatically.
- Operator visual disposition: same as Cell C—the nominal full-screen TEXT black is very light/almost white with slight ghosting.
- The Issue 181 boundary corpus subsequently passed rotations 0–3 automatically without reboot or heap failure.
- Final automatic status after 267 updates: heap integrity passed; 305,655 bytes internal heap and 7,088,344 bytes SPIRAM remained free.
