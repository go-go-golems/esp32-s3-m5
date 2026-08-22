---
Title: Fresh Base-Principles Reconstruction of the ESP-IDF ST25R3916 Failure
Ticket: ESP-60-M5STACKCHAN-NFC
Status: active
Topics:
    - m5stackchan
    - nfc
    - st25r3916
    - esp-idf
    - i2c
    - iso14443a
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.c
      Note: Current ESP-IDF transport and single-tag anticollision implementation
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/reference/01-investigation-diary.md
      Note: Prior evidence and refuted FSM-reset hypothesis
    - Path: repo://ttmp/2026/08/20/ESP-60-M5STACKCHAN-NFC--esp-idf-st25r3916-nfc-reader-console-app-for-m5stackchan-intern-guide/sources/code/m5unit-nfc/nfc_layer_a.cpp
      Note: Working M5 NFC-A request/anticollision reference
ExternalSources: []
Summary: A clean-room investigation that separates I2C transport, ST25R3916 state, RF request/response, collision resolution, selection, and UID output instead of treating all failures as one backend problem.
LastUpdated: 2026-08-21T21:35:00-04:00
WhatFor: Drive a new evidence ladder from the connected four-tag hardware to a stable ESP-IDF UID read.
WhenToUse: Use as the active investigation record after the bare per-transaction fsm_rst hypothesis was refuted.
---


# Fresh Base-Principles Reconstruction of the ESP-IDF ST25R3916 Failure

## 1. Purpose and reset of assumptions

This document restarts the investigation from externally observable behavior. Previous work proved several useful facts, but one strong explanation — that a preventive I2C command-FSM reset would eliminate the NACKs — was tested and refuted. That negative result is retained as evidence, not used as a premise.

The goal is not to make an error counter look better. The goal is for native ESP-IDF firmware to print at least one valid ISO14443-A UID from the four tags currently resting on the StackChan antenna, while preserving enough diagnostics to distinguish a transport defect from a legal multi-tag collision.

No layer may be skipped. A UID appears only when all of these layers work:

```mermaid
flowchart LR
    BUS["1. I2C transport"] --> CHIP["2. ST25R3916 register state"]
    CHIP --> RF["3. RF field + REQA/WUPA"]
    RF --> AC["4. Anticollision"]
    AC --> SEL["5. SELECT + SAK"]
    SEL --> UID["6. UID assembly/output"]
```

A failure at one layer must not be named after a different layer. In particular:

- `ESP_ERR_INVALID_STATE` from `i2c_master_transmit_receive` is a confirmed I2C NACK.
- `ESP_ERR_INVALID_STATE` returned by `nfca_anticoll_select` when `ST25R_IRQ_COL` is set is an RF collision in the current application logic.
- Four tags simultaneously on the antenna are expected to collide during the first anticollision round.
- The current standalone ESP-IDF implementation explicitly says "Phase-1 single-tag assumption" and aborts when it sees that collision. Therefore a four-tag run can prove RF reception while still producing no UID by design.

## 2. Current known facts (kept, but not overinterpreted)

### 2.1 Hardware and placement

- The ST25R3916 responds at I2C address `0x50`.
- Identity is type `0x05`, revision `0x02`.
- The antenna is the narrow top edge of the StackChan head.
- Official Arduino firmware reads real UIDs there.
- Four tags are currently present on the antenna.

### 2.2 I2C transport

- ESP-IDF intermittently receives `I2C_EVENT_NACK`; the public API returns `ESP_ERR_INVALID_STATE`.
- These are NACK events, not timeouts (driver DEBUG evidence; zero timeout log lines).
- M5GFX reports zero logical failures over 10,187 traced ST25R3916 transactions.
- A bare unconditional `fsm_rst` patch made the ESP-IDF failure rate worse and was reverted.
- The reverted board currently runs the standalone `0115` ESP-IDF debug build.

### 2.3 NFC protocol implementation

The current ESP-IDF code can only resolve a single tag:

```c
if (irq & ST25R_IRQ_COL) {
    /* Phase-1 single-tag assumption: a collision means two+ tags; report it. */
    return ESP_ERR_INVALID_STATE;
}
```

The Arduino control uses M5Unit-NFC's full anticollision/detection layer and can retain multiple PICCs. It is therefore not valid to compare "Arduino found four tags" against "ESP-IDF returned collision" as if both implemented the same protocol algorithm.

## 3. Observation ladder

Each experiment must answer one narrow question and record the exact evidence.

### Layer 1 — transport

Question: can the host perform the register/direct-command transactions needed by one read attempt without a NACK?

Evidence:

- trace count and transport-failure count;
- first failed logical/wire key;
- driver DEBUG event class;
- whether a retry succeeds immediately.

Pass condition for protocol debugging: either zero transport failures in the attempt or an explicit attempt result that says "protocol evidence observed despite N transport failures." Transport failures must never be silently converted to no-tag.

### Layer 2 — chip state

Question: after initialization, are the exact critical registers equal to the measured Arduino state?

Minimum matrix:

```text
MODE=09 RX1=08 RX2=2D RX3=D8 RX4=22
ANT1=82 ANT2=82 TXD=D0
NRT=0350 for a 4 ms request (depending on timer-step configuration)
Space-B OS=40/03 US=40/03 CORR=47/00 EMD=40
```

Pass condition: successful reads and exact values immediately before RF request.

### Layer 3 — RF request/response

Question: with four tags present, does REQA or WUPA produce RF evidence?

Evidence, in increasing strength:

1. `RXS` IRQ — receiver saw start-of-frame.
2. `COL` IRQ — at least two tag responses overlap; this is positive evidence that tags and RF field are active.
3. `RXE` IRQ + FIFO bytes — a complete response frame.
4. ATQA bytes — complete request response.

With four tags, `COL` is not a failure of the RF layer. It is the expected input to Layer 4.

### Layer 4 — anticollision

Question: can the firmware resolve one UID bit path when multiple tags collide?

ISO14443-A anticollision uses `SEL` (`0x93`, `0x95`, `0x97`) and NVB to request progressively more known UID bits. On collision, read the collision position, choose a branch bit, and retransmit with a longer NVB. The current ESP-IDF implementation does none of this; it requests the full cascade-level UID once (`NVB=0x20`) and aborts on collision.

Pseudocode:

```text
known_bits = empty
while known_bits < 40:
    send SEL + NVB(known_bits) + known_prefix
    wait for RXE or COL
    if COL:
        p = collision_bit_position
        copy valid received bits before p
        append chosen branch bit (0 or 1)
        continue
    if RXE:
        append remaining UID+BCC bits
        validate BCC
        break
```

Pass condition: one 5-byte cascade-level result (four UID bytes/cascade tag + BCC) with valid BCC.

### Layer 5 — selection

Question: can the resolved cascade-level UID be selected with `NVB=0x70`, and does the chip receive SAK + CRC?

Pass condition: valid SAK frame; continue to CL2/CL3 when the cascade bit (`SAK & 0x04`) is set.

### Layer 6 — UID output

Question: does the assembled 4-, 7-, or 10-byte UID match a UID already observed by Arduino?

Pass condition: print UID/ATQA/SAK and retain transport counters separately.

## 4. First experiment: observe the four-tag RF result before changing code

The connected hardware is valuable because four tags create a deterministic protocol condition. Run the reverted baseline firmware and collect:

```text
nfc-trace clear
nfc-i2c-debug off
nfc-read --attempts 1
nfc-trace status
nfc-trace first-error
```

Interpretation matrix:

| Observation | Meaning |
|---|---|
| no RXS/RXE/COL, FIFO=0 | RF request not producing a visible response; inspect field/config/request timing |
| COL=1 | RF field and multiple tag responses confirmed; implement collision resolution |
| RXE=1, FIFO=2, ATQA | request layer works; proceed to anticollision |
| transport NACK before direct command `0xC6/0xC7` | request was not transmitted; protocol conclusion invalid |
| NACK only during IRQ polling but later COL/RXE | transport unstable, but RF evidence still usable; report both |

## 5. Fresh hypotheses (ordered by layer, not confidence)

### H1 — current four-tag failure is legal RF collision

The code explicitly aborts on `ST25R_IRQ_COL`; with four tags on the antenna, this is expected. If the first experiment sees COL, the immediate implementation task is ISO14443-A collision resolution, not more I2C backend theory.

### H2 — request timing/configuration prevents any RF response

If no RXS/COL/RXE occurs, compare the exact request sequence against M5Unit-NFC: field sequencing, NRT timer, interrupt masks/clears, auxiliary `no_crc_rx`, antcl bit, and direct command completion.

### H3 — I2C NACK prevents the request from being sent

If the first transport failure precedes `DIRECT 0xC6/0xC7`, the RF layer was never exercised. The attempt must be retried observably or the transport backend must change.

### H4 — RF request succeeds but selection implementation is wrong

If ATQA is read but no UID appears with one tag, compare anticollision frame length, TX byte count encoding, CRC mode, FIFO bytes, BCC, SELECT frame, and SAK/CRC handling.

### H5 — transport backend framing differs from M5GFX

The bare FSM reset is refuted. Remaining host differences include explicit command programming, final-read NACK/STOP handling, bus-idle wait, pin routing/mode reinit, and timeout programming. Defined operations can isolate framing without copying M5GFX wholesale.

## 6. Rules for this investigation

- Do not make a transport conclusion from an RF collision.
- Do not make an RF conclusion if the request command was never transmitted.
- Do not hide the first NACK behind eventual success.
- Do not add broad retries until a one-attempt trace identifies the failing layer.
- Commit at evidence boundaries: analysis baseline, instrumentation/behavior change, hardware result, diary.
- Keep the board in a documented firmware state after every flash.

## 7. Immediate plan

1. Capture one four-tag read on the reverted baseline.
2. Identify the last successful direct command and RF IRQ/FIFO result.
3. Compare the exact M5Unit-NFC request and anticollision implementation to the current driver at source level.
4. If COL is present, implement bounded anticollision for one UID branch, then SELECT and print one UID.
5. If no RF evidence, fix the request layer before anticollision.
6. If a NACK prevents request transmission, test explicit defined operations for just the failing transaction class.

## 8. Active evidence log

This section will be updated chronologically. The initial state is:

- board firmware: reverted baseline `0115` debug build;
- local ESP-IDF source: clean `73550728`;
- hardware: four tags present on the top-edge antenna;
- serial: `/dev/ttyACM0`, currently unowned;
- next evidence: one-attempt four-tag `nfc-read` with trace status and first-error bundle.
