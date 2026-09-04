# Kagi Web Search: ST25R3916 NFC controller ESP32 I2C driver ESP-IDF

Source: https://kagi.com/search?q=ST25R3916+NFC+controller+ESP32+I2C+driver+ESP-IDF+example+read+tag
Date: 2026-08-20

## Key results

1. **wilson-elechouse/ST25R3916** (GitHub) — ESP32-focused fork of ST25R3916/ST25R3916B + NFC-RFAL Arduino libraries. Validates SPI (ISO15693, A/B/V scanning, ICODE, MIFARE Classic S70) and I2C (chip probe, ISO14443A scanning, S70 read/auth/dump, ISO15693 on ICODE SLIX2, NDEF). Targets ESP32 Dev Module, ESP32-S3, ESP32-C3.
   - https://github.com/wilson-elechouse/ST25R3916
   - I2C examples available after configuring board for I2C mode.

2. **mcqn/nfc-st25r3916** (GitHub) — Alternative Arduino library with examples + wiring.
   - https://github.com/mcqn/nfc-st25r3916/blob/main/Examples.md

3. **elechouse ST25R3916 ESP32 I2C Quick Start** — Guide: module works SPI by default; for I2C must configure board mode.
   - https://www.elechouse.com/st25r3916-esp32-i2c-quick-start/

4. **ST Community: ST25R3916 driver for external MCU (ESP32)** — ST forum thread on I2C/SPI driver port.
   - https://community.st.com/st25-nfc-rfid-tags-and-readers-54/st25r3916-driver-for-using-external-mcu-esp32-41742

5. **ST25R3916B controlled by ESP32 not working** — ST forum thread: I2C comms working, rfalNfcInitialize succeeds, rfalFieldOn fails (useful troubleshooting).
   - https://community.st.com/st25-nfc-rfid-tags-and-readers-54/st25r3916b-controlled-by-esp32-not-working-142927

6. **ESP-IDF I2C docs** (stable) — new I2C master driver API.
   - https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2c.html

## Conclusion for this project
The M5StackChan body board has a ST25R3916 at I2C 0x50. The M5Unit-NFC library (M5Stack official) already contains a register-level ST25R3916 driver (`unit_ST25R3916.cpp` + `ST25R3916_definition.hpp`) that works over I2C via the M5UnitUnified `Component` abstraction. For a standalone ESP-IDF reader app, the cleanest path is a minimal from-scratch ST25R3916 driver using the ESP-IDF `driver/i2c_master.h` API (matching the firmware's existing PY32IOExpander pattern), guided by the register definitions in `ST25R3916_definition.hpp`.
