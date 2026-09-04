# NFC LAB structured serial runtime capture

- Captured: 2026-08-21
- Firmware commit: `9c9fa2e3168d2486923c6bab2b03d34c5afd3107`
- ESP-IDF: 5.5.4
- Device: `/dev/ttyACM0`, 115200 baud
- Capture duration: 20 seconds
- Raw gzip: `01-nfc-lab-structured-serial-runtime.log.gz`
- Raw uncompressed SHA-256: `072def832962ac38ab4a3b2a3c45a3226352c8953dd16def5e74f1c363f973b9`
- Readable normalized log: `01-nfc-lab-structured-serial-runtime.log`
- Readable SHA-256: `43846368f11de0fdd4f10885970ec6c3bce70c7bf1655fb281de0b92bfffde2c`

The serial port had no owner before flashing or capture. Opening the port caused
`USB_UART_CHIP_RESET`, so the log includes a complete boot and NFC.LAB reader
initialization. The structured logger caught a real transaction failure:

`NFC_I2C_FAIL txn=65 failed=1 op=READ_A(1) key=0x02 err=ESP_ERR_INVALID_STATE(0x103) elapsed_us=195`

The service then correlated it with initialization:

`NFC_INIT event=failed err=ESP_ERR_INVALID_STATE(0x103) elapsed_us=59057 txns=65 failed=1 last_op=1 last_key=0x02`

This failure occurred while `st25r3916_field_on()` read
`ST25R_REG_OPERATION_CONTROL` (Space-A register `0x02`) during NFC-A
configuration. It is another pre-REQA transport failure and is independent of
tag presence or placement.
