# Provenance: captures 12–16 (anticollision port and single-tag probes)

Date: 2026-08-21. Board: M5StackChan CoreS3, standalone `0115`, native ESP-IDF 5.5.4, field-enable fix `7465a834`, bounded M5Unit-NFC anticollision port `6e365ebc`. ESP-IDF driver source clean/reverted. Serial probes used one owner and waited for the actual `nfc>` prompt.

## Capture 12 — first four-tag anticollision build

`12-four-tag-anticollision-port.txt`: one attempt. Boot-time register read showed `OPC=0x8B`; no REQA/WUPA RF response; 3 transient I2C NACKs. The anticollision code was not reached.

## Capture 13 — field-state boundary, four tags

`13-field-state-boundary-probe.txt`: explicit `nfc-field on`, register checks, one + five attempts.

- `OPC=0xCB` before and after field-on/read: TX and RX both enabled.
- Several attempts produced no RF IRQ.
- One WUPA and one REQA produced `irq=0x34` (`RXS|RXE|COL`), FIFO zero.
- Collision-bearing request was allowed into anticollision, but anticollision timed out with no response (`NRT=0x069F`, timer IRQ `0x40` = no-response); no `anticoll retry` line because FIFO was empty before the collision-position path.
- Transport remained unstable (36 failures / 2742 transactions); one driver bus-busy software timeout appeared.

## Capture 14 — first-collision tail attempt

`14-first-collision-anticoll-tail.txt`: up to 20 one-attempt reads, stop on collision, then trace tail. No collision occurred in 20 attempts; several attempts failed with `ESP_ERR_INVALID_STATE`, most returned no-tag. This shows RF observation is intermittent even with field bits correct.

## Capture 15 — one tag after user removed three tags

`15-single-tag-after-field-and-anticoll-fixes.txt`: one prompt-aware attempt. Initial `OPC=0x8B`; no RF IRQ; no tag; five transport NACKs. It did not reach anticollision.

## Capture 16 — explicit field-state boundary, one tag

`16-single-tag-field-boundary.txt`: explicit field-on and repeated reads.

- `OPC=0xCB` at every checked boundary.
- Most attempts: no `RXS/RXE/COL`.
- One single-tag WUPA unexpectedly produced `irq=0x34` (`RXS|RXE|COL`) with FIFO zero; anticollision then timed out/no-response.
- A single physical tag should not require multi-tag collision resolution. Therefore `COL` cannot yet be assumed to mean legal multi-tag collision; it may indicate receiver/noise/configuration or a malformed RF exchange.
- Transport: 32 failures / 3450 transactions.

## Current interpretation

The reversed TX/RX field-on semantics was a real defect (capture 10→11 causal A/B), but correcting it is not sufficient. With one tag and `OPC=0xCB`, RF responses remain intermittent and sometimes report `COL` with empty FIFO, followed by anticollision no-response. The next comparison must hold the high-level NFC sequence constant while changing only the I2C backend (`idf-high` vs explicit defined operations, then M5-direct if needed), and must compare the exact contiguous register operations used by M5 (4-byte IRQ clear/read, 2-byte FIFO status, 16-bit NRT/TX length writes).
