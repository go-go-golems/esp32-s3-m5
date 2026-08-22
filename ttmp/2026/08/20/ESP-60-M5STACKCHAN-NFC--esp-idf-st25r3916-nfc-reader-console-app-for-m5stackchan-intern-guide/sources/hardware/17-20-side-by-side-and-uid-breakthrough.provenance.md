# Provenance: captures 17–20 — same-firmware backend A/B and UID breakthrough

Date: 2026-08-21. Hardware: M5StackChan CoreS3, one physical tag left on the top-edge antenna. Serial: one-owner prompt-aware ticket scripts. The Arduino and ESP-IDF full flashes used their own partition tables.

## Capture 17 — same-firmware transport A/B before field idempotence

`17-same-firmware-high-vs-defined-one-tag.txt`, firmware `c339ea7a`.

The same native ESP-IDF firmware selected two runtime backends:

- `idf-high`: standard `i2c_master_transmit[_receive]` addressed device handle.
- `idf-defined`: `i2c_master_execute_defined_operations` with explicit START, `0xA0`, payload, repeated START, `0xA1`, ACK reads, final NACK, STOP.

Both re-applied NFC-A configuration in the same boot. Results:

- `idf-high`: 0 failures / 2836 transactions; no RF response.
- `idf-defined`: 0 failures / 2556 transactions; no RF response.

Conclusion: transport NACK/framing was not the active blocker. Both backends produced identical no-RF behavior with clean transport.

## Capture 18 — Arduino control, same single tag and placement

`18-arduino-single-tag-current-placement.txt`: full-flashed instrumented persistent Arduino monitor.

- Init: 335/335 transport success.
- First cycle: WUPA `ATQA=0x0044`, one PICC detected and identified, registry `seen=1`.
- 13,874+ cumulative ST25R3916 transactions, zero transport failures in captured window.

Conclusion: hardware, tag, placement, field, and M5 protocol sequence work at the time of the ESP-IDF tests.

## Capture 19 — idempotent field-on A/B

`19-idempotent-field-high-vs-defined-one-tag.txt`, firmware `fe6252a5`.

M5 `nfc_initial_field_on()` refuses to issue C8 when TX is already enabled. ESP-IDF previously called C8 before every poll. Field-on was made idempotent: if TX+RX are set, return; if TX-only (`0x8B`), set RX without restarting field; otherwise issue C8, delay, set/read-back TX+RX.

Results:

- Both backends: zero transport failures.
- `idf-high`: REQA `RXS`, FIFO=2; anticollision timed out.
- `idf-defined`: REQA `RXS+RXE`, FIFO=2; anticollision timed out.

This crossed the request layer and isolated the failure to FIFO-based anticollision transmit setup.

## Source comparison: NUM_TX_BYTES byte order

M5Unit-NFC `writeNumberOfTransmittedBytes()` documents:

```text
REG_NUMBER_OF_TRANSMITTED_BYTES_1 (0x22) = MSB
REG_NUMBER_OF_TRANSMITTED_BYTES_2 (0x23) = LSB
```

ESP-IDF wrote the encoded value reversed. For two bytes, encoded value is `0x0010`; ours wrote `0x22=0x10`, `0x23=0x00` instead of `0x22=0x00`, `0x23=0x10`. REQA/WUPA are special direct commands and therefore worked; FIFO-based anticollision and SELECT transmitted an invalid length and received no response.

## Capture 20 — TX-length byte-order fix: UID on both backends

`20-tx-length-fix-high-vs-defined-one-tag.txt`, firmware `ace8a809`.

`idf-high`:

```text
REQA: RXS, FIFO=2 (fallback recognizes complete ATQA)
anticoll CL1: RXE, FIFO=5, BCC valid
anticoll CL2: RXE, FIFO=5, BCC valid
PICC: UID=04:91:D4:4C:9E:61:80 ATQA=0044 SAK=00 type=MIFARE Ultralight/NTAG
TRACE_STATUS recorded=200 failed=0 first_error=none
```

`idf-defined`:

```text
WUPA: RXS+RXE, FIFO=2
anticoll CL1: RXE, FIFO=5, BCC valid
anticoll CL2: RXE, FIFO=5, BCC valid
PICC: UID=04:91:D4:4C:9E:61:80 ATQA=0044 SAK=00 type=MIFARE Ultralight/NTAG
TRACE_STATUS recorded=439 failed=0 first_error=none
```

## Final causal conclusion

The native ESP-IDF port failed primarily because of deterministic application-port defects, not an inherent inability of ESP-IDF I2C to communicate with the ST25R3916:

1. `st25r3916_field_on()` cleared TX/RX instead of setting them.
2. Polling reissued `CMD_NFC_INITIAL_FIELD_ON` while the field was already active instead of preserving the carrier.
3. `NUM_TX_BYTES_1/2` were written LSB/MSB instead of required MSB/LSB, breaking every FIFO-based anticollision/SELECT frame while special REQA/WUPA commands still worked.

Once corrected, both the standard and explicit-defined-operation ESP-IDF backends read the same UID with zero transport failures. The intermittent NACKs were real, but they were amplified by the broken request/IRQ polling path and are not the root cause of the missing UID.
