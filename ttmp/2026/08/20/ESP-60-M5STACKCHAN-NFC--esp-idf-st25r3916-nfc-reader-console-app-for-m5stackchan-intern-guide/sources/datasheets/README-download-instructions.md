# ST25R3916B Datasheet (canonical reference)

The official ST25R3916B / ST25R3917B / ST25R3919B datasheet is hosted by STMicroelectronics:

- **URL:** https://www.st.com/resource/en/datasheet/st25r3916b.pdf
- **Title:** ST25R3916B ST25R3917B ST25R3919B — NFC reader for payment and e-Government
- **Publisher:** STMicroelectronics

> Note: ST.com blocks automated downloads (HTTP/2 INTERNAL_ERROR on `curl`). Download it
> manually in a browser and place the PDF in this `datasheets/` directory:
>
> ```
> # in a browser, save as:
> sources/datasheets/ST25R3916B-datasheet.pdf
> ```

## Why it matters

This datasheet is the canonical register-map and direct-command reference for the ST25R3916
chip on the M5StackChan body board. For day-to-day coding, the register constants are already
captured in machine-usable form in `sources/code/ST25R3916_definition.hpp` (extracted from the
M5Unit-NFC library). Cross-reference that header against the datasheet's:

- **Section: Register map** (Space A and Space B, direct-command byte format)
- **Section: Direct commands** (e.g. `CMD_SET_DEFAULT` 0xC1, `CMD_TRANSMIT_REQA`, `CMD_ADJUST_REGULATORS`)
- **Section: I2C interface** (the read/write command-byte protocol: `(reg & 0x3F) | direction`)
- **Section: Operation control register** (RF field on/off, TX/RX enable)
- **Section: FIFO** (max depth 512 bytes)

## Supporting community references (read for troubleshooting, not for coding)

- ST Community: ST25R3916 driver for external MCU (ESP32) — https://community.st.com/st25-nfc-rfid-tags-and-readers-54/st25r3916-driver-for-using-external-mcu-esp32-41742
- ST Community: ST25R3916B controlled by ESP32 not working (rfalFieldOn failure) — https://community.st.com/st25-nfc-rfid-tags-and-readers-54/st25r3916b-controlled-by-esp32-not-working-142927
- ST Community: ST25R3916B I2C reference design — https://community.st.com/st25-nfc-rfid-tags-and-readers-54/st25r3916b-i2c-reference-design-3501
- ST Community: ST25R3916B writing to registers bare metal — https://community.st.com/st25-nfc-rfid-tags-and-readers-54/st25r3916b-writing-to-registers-bare-metal-150029
- ST Community: I2C communication not working between ST25R3916B and MCU — https://community.st.com/st25-nfc-rfid-tags-and-readers-54/i2c-communication-is-not-working-between-st25r3916b-and-mcu-stm32g0b1rt6n-154873
