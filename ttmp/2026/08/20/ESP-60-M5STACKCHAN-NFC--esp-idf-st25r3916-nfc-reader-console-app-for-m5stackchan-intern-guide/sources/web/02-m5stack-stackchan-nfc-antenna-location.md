# StackChan NFC antenna location (official M5 docs)

Source: https://docs.m5stack.com/en/arduino/stackchan/nfc (page title "StackChan NFC Near Field Communication")
Captured: 2026-08-21 via Playwright (JS-rendered; defuddle returns empty).

## The decisive facts

The NFC reader/writer antenna is on the **TOP sensing surface of StackChan**
(the literal narrow top edge / roof of the CoreS3 head), NOT the front display
surface and NOT the robot body. This interpretation is confirmed by the official
photographs preserved in `03-m5stack-stackchan-nfc-official-images.md`.

- Quick Scan Identification example:
  > "After uploading the above code ... place one or more tag cards near the
  > **top sensing surface of StackChan** to see the identification results."

- Complete Data Reading example:
  > "This process requires placing the card near the **top sensing surface of
  > StackChan** when touching the screen."

- Tag Emulation example (the StackChan emulates a tag):
  > "When other NFC readers (such as a smartphone) approach the **top of
  > StackChan**, they can detect and read the emulated NFC tag."

## Implication for ESP-60 Phase 1

The ST25R3916 sits on the body I2C bus, but the sensing position is the
**literal top edge / roof of the head**. The official photo shows cards resting
horizontally across that narrow upper face, perpendicular to the front display.
Do NOT interpret “top” as the display glass, and do NOT move the tag to the lower body.

Earlier debugging rounds first told the user to move the tag to the body, then
incorrectly interpreted “top” as the display face. Both were wrong. Correct
placement is: **tag/card horizontally across the literal narrow top edge of the
StackChan head**, as shown in the official images.

## The official reader workflow (from the M5 NFC docs, "Reader Basic Workflow")

1. Initialization: `M5.begin()` + `Wire.begin()`
2. Detection: `nfc_a.detect()` / `nfc_a.detect(piccs)`
3. Identification: `nfc_a.identify()`
4. Activation: `nfc_a.reactivate()`
5. Authentication (MIFARE Classic): `mifareClassicAuthenticateA/B()`
6. Operation: read/write
7. Deactivation: `nfc_a.deactivate()`

These use the same M5Unit-NFC ST25R3916 register-level driver our ESP-IDF port
is based on (`sources/code/unit_ST25R3916.hpp`), so the same antenna + chip
work for reader mode as for emulation.

## Expected serial output (from the docs)

```
PICC:3E86E2D5 MIFARE Classsic1K 0004/08 752/1024
PICC:04327CD2B97880 MIFARE Plus 2K X/EV SL0 0044/20 1520/2048 ==> 2 PICC
```

So a known-good ISO14443-A tag (MIFARE Classic 1K, MIFARE Plus, Ultralight, NTAG)
flat on the top of the head should produce `PICC:<uid> <type> ...`.
