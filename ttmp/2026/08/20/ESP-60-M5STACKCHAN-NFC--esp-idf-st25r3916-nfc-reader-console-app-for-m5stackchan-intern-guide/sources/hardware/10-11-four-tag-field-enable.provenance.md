# Provenance: four-tag RF field-enable experiment (captures 10 and 11)

- Date: 2026-08-21
- Board: M5StackChan CoreS3, four tags physically present on top-edge antenna.
- Firmware: standalone `0115`, reverted ESP-IDF 5.5.4 baseline driver, debug logging compiled but `i2c.master` runtime level INFO.
- Serial: one owner through `scripts/09-probe-four-tag-layered.py`; prompt-aware command sequencing.

## Before (`10-four-tag-layered-baseline.txt`)

- Critical registers: `OPC=8B MODE=09 ...`.
- `OPC=0x8B` means oscillator + TX + external field detector, but **RX_EN (`0x40`) is clear**.
- REQA: no RXS/RXE/COL, FIFO 0.
- WUPA: no RXS/RXE/COL, FIFO 0.
- Result: `no tag`; three transient polling-read NACKs.

## Source comparison

Working M5Unit-NFC `nfc_initial_field_on()`:

```c
writeDirectCommand(CMD_NFC_INITIAL_FIELD_ON);
delay(5);
return modify_bit_register8(REG_OPERATION_CONTROL, tx_en | rx_en, 0x00);
```

`modify_bit_register8(set_mask, clear_mask)` computes `(v & ~clear_mask) | set_mask`, so it **sets** both bits.

ESP-IDF `st25r3916_field_on()` incorrectly did:

```c
return clear_bits(OPERATION_CONTROL, TX_EN | RX_EN);
```

The accompanying comment also incorrectly claimed M5 cleared them.

## Fix (`7465a834`)

Changed `st25r3916_field_on()` to set `TX_EN|RX_EN` after the 5 ms field-on guard.

## After (`11-four-tag-field-enable-fix.txt`)

- Critical registers: `OPC=CB` — oscillator + TX + RX + external field detector all enabled.
- REQA: no response (tags may remain HALTed from prior Arduino selection).
- WUPA: `irq=0x34` = `RXS|RXE|COL`; timer byte `0xA4`; four-tag RF response and collision confirmed.
- FIFO status was zero at request-result handling, so current request helper returned `ESP_FAIL`.
- Result is now a protocol-layer limitation, not absent RF: current code requires clean 2-byte ATQA and its anticollision function explicitly aborts on `COL`.

## Conclusion

The primary no-RF defect was application code: TX/RX were cleared after field-on, opposite the working M5 implementation. This one-line semantic correction establishes RF reception. Next step is to let a collision-bearing WUPA transition into M5-style bounded anticollision and SELECT one UID branch.
