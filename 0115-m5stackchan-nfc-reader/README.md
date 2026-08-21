# 0115-m5stackchan-nfc-reader

ESP-60-M5STACKCHAN-NFC — Phase 1: a standalone ESP-IDF NFC reader for the M5StackChan.

Reads ISO14443-A tags (NTAG, MIFARE Ultralight, MIFARE Classic) from the on-board
**ST25R3916** NFC reader IC (I2C address 0x50) and prints UID/ATQA/SAK over a
USB Serial/JTAG `esp_console` REPL. No graphical UI in this phase.

## Hardware

- M5StackChan (CoreS3 = ESP32-S3) with body module attached.
- ST25R3916 on the shared body I2C bus: **port 1, SDA=GPIO12, SCL=GPIO11, addr 0x50**.
- Console over USB Serial/JTAG (the USB-C cable; shows up as `/dev/ttyACM0`).

## Build & flash

```bash
source ~/esp/esp-idf-5.5.4/export.sh
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash
idf.py -p /dev/ttyACM0 monitor
```

## Console commands

| Command | What it does |
|--------|--------------|
| `nfc-scan` | I2C bus scan (0x00–0x7f); 0x50 should appear |
| `nfc-probe` | Read ST25R3916 IC identity (type should be 0x05) |
| `nfc-field on` / `nfc-field off` | Toggle the 13.56 MHz RF field |
| `nfc-read` | Poll once for an ISO14443-A tag; prints UID/ATQA/SAK |
| `nfc-poll` | Continuously poll; prints tag on change, "tag removed" when gone |

Expected `nfc-read` output with an NTAG on the reader:
```
PICC: UID=04:34:56:78:9A:BC:DE ATQA=0044 SAK=00 type=MIFARE Ultralight/NTAG
```

## Design

See the intern guide in ticket `ESP-60-M5STACKCHAN-NFC`
(`ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--.../design-doc/01-...md`).
The driver mirrors the StackChan firmware's I2C style (`PY32IOExpander_Class`):
`driver/i2c_master.h` with `i2c_master_bus_add_device` + transmit/transmit_receive.
