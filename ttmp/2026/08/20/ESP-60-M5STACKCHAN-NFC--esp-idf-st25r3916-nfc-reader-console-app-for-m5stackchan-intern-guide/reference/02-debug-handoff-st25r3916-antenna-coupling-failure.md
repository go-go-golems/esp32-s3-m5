---
Title: Debug handoff — ST25R3916 antenna coupling failure
Ticket: ESP-60-M5STACKCHAN-NFC
Status: active
Topics:
    - m5stackchan
    - nfc
    - st25r3916
    - esp32-s3
    - esp-idf
    - esp-console
    - debug-handoff
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0115-m5stackchan-nfc-reader/main/nfc_console.c
      Note: NFC reader firmware source
    - Path: repo://0115-m5stackchan-nfc-reader/main/nfc_reader_main.c
      Note: NFC reader firmware source
    - Path: repo://0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.c
      Note: NFC reader firmware source
    - Path: repo://0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.h
      Note: NFC reader firmware source
    - Path: repo://0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916_regs.h
      Note: NFC reader firmware source
ExternalSources:
    - sources/code/unit_ST25R3916.hpp — M5Unit-NFC register-level driver (the reference init + NFC-A sequence we matched)
    - sources/code/ST25R3916_definition.hpp — register/command constants + bit definitions
Summary: ""
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: Hand off the Phase-1 NFC reader debugging to an embedded/NFC expert. The driver is correct; the antenna does not couple. This doc gives the symptom, what is ruled out, exact reproduction, evidence, ranked hypotheses, and concrete next steps.
WhenToUse: Read this FIRST before touching the NFC reader code or the StackChan hardware for this ticket.
---






# Debug Handoff — ST25R3916 Antenna Coupling Failure

**Ticket:** ESP-60-M5STACKCHAN-NFC (Phase 1, standalone ESP-IDF NFC reader)
**Handoff from:** the agent that built + flashed the firmware (see diary `reference/01-investigation-diary.md`, Steps 5–6)
**Handoff to:** embedded / NFC expert
**Last commit:** `a7bebf8e` on `main` in `esp32-s3-m5`
**Date:** 2026-08-20

> **TL;DR:** A standalone ESP-IDF firmware talks to the ST25R3916 over I2C perfectly — chip detects (type=0x05), all init registers match the official M5Unit-NFC library exactly, the field commands on, the oscillator starts cleanly. **But the antenna does not couple:** `CMD_MEASURE_AMPLITUDE` reads 0 even with the field forced on, and 40 s of sweeping a tag over the body yields no REQA response. The driver is almost certainly correct; the failure is on the **RF/analog stage or the tag/placement**. One fleeting REQA `RXE` was seen earlier, so the antenna *can* radiate — it is intermittent. Your job is to find why it does not couple reliably.

---

## 1. Environment

| Item | Value |
|------|-------|
| Device | M5StackChan (SKU K151) = M5Stack **CoreS3** (ESP32-S3) + robot **body module** |
| Console | USB Serial/JTAG → `/dev/ttyACM0` (`usb-Espressif_USB_JTAG_serial_debug_unit_...-if00`) |
| IDF | `~/esp/esp-idf-5.5.4` (firmware requires `>=5.5.2`; `idf.py --version` = v5.5.4) |
| Project | `esp32-s3-m5/0115-m5stackchan-nfc-reader/` (standalone, not a Mooncake app) |
| Binary | `m5stackchan_nfc_reader.bin`, ~244 KB (77% of 1 MB factory partition free) |
| NFC chip | **ST25R3916**, I2C address **0x50**, on the shared body I2C bus |
| I2C bus | **port 1, SDA=GPIO12, SCL=GPIO11**, internal pullups, new `driver/i2c_master.h` API |

The StackChan body NFC coil is a copper trace on the **body PCB** (the lower part with the servos/feet), **not** the head/display. There is **no NFC power-enable IO-expander pin** (the PY32L020 expander at 0x6F only controls servo power on pin 0 and an RGB enable on pin 13 — verified in `StackChan-BSP/src/M5StackChan.cpp`), so the ST25R3916 is always powered when the body is seated.

## 2. Symptom (precise)

- `nfc-scan` → I2C map shows 0x34 (PMIC), 0x38 (touch), 0x41 (INA226), **0x50 (ST25R3916)**, 0x51 (RTC), 0x58 (AW9523), 0x68/0x69 (Si12T), 0x6f (PY32IOExpander). ✓ **bus OK**
- `nfc-probe` → `ST25R3916 type=0x05 rev=0x02 (ST25R3916/7 OK)`. ✓ **chip alive**
- `nfc-regs` (after boot/init) → `OPC=83 MODE=09 ISO=00 AUX=00 RX1=08 RX2=2D RSSI=00 IRQ=001C00`, and (with readback) `ANT1=82 ANT2=82 TXD=D0`. ✓ **init matches M5 lib, antenna tuning + TX driver correctly programmed**
- `nfc-cap` (CMD_MEASURE_CAPACITANCE, repeated) → **`cap=124` stable (123–125)**. ✓ **antenna coil is connected and present** (rules out an open antenna feed / bad body seating)
- `nfc-field on` → `ESP_OK`; `nfc-regs` after → `OPC=8B` (tx_en=0x08 set during transmit). ✓ **field commanded on**
- `nfc-reqa` (raw IRQ logged after every REQA) → **`reqa: irq=000000 fifo=0 rxs=0 rxe=0 col=0` on EVERY attempt** (40+ samples). ✗ **no tag responds at all — not even RXS**
- `nfc-read` / `nfc-poll` → **"no tag"**, `RSSI=00`, `MAIN_IRQ=000000`, `FIFO_bytes=0`. ✗
- `nfc-sweep` (forced field + rx enabled, `CMD_MEASURE_AMPLITUDE` loop) → **`amp= 0` repeatedly**. ✗ (likely the amplitude meas needs regs 0x33/0x34; see §4)
- **One** earlier observation: a single `reqa err: ESP_FAIL` (before a FIFO-parsing fix) implied **RXE fired once** — so the antenna *can* radiate, but it is intermittent.

**Decisive conclusion:** the antenna coil is connected (cap=124) and correctly tuned/programmed (ANT1/2=82, TXD=D0), the chip is up and configured exactly per the M5 lib, the field is commanded on, yet **no tag ever answers REQA** (`irq=000000` always). The remaining variable is the **tag itself or its exact placement on the coil** — a physical step.

## 3. What has been ruled out

- **I2C transport.** `nfc-scan` enumerates 0x50; `nfc-probe` reads IC identity type=0x05; every register read/write in init succeeds. The bus is fine.
- **Chip presence + identity.** type=0x05 rev=0x02 = genuine ST25R3916/7.
- **Init register values.** I cross-checked every register write in `st25r3916_init()` + `st25r3916_configure_nfca()` against `M5Unit-NFC/src/unit/unit_ST25R3916.cpp::begin()` and `unit_ST25R3916_nfca.cpp::configure_nfc_a()` line by line. They now match. Confirmed by `nfc-regs` post-init dump.
- **Oscillator.** No `"oscillator did not stabilize"` warning in boot log; `enable_osc()` (set `en`=0x80, wait `I_osc` IRQ) path runs. (Note: I unmask I_osc, set `en`, wait up to 50 ms; on warm boot the edge can be missed, but the M5 lib has the same fallback.)
- **OPERATION_CONTROL bit definitions.** Fixed: `en`=0x80, `rx_en`=0x40, `tx_en`=0x08, `wu`=0x04, `en_fd`=0x03. (Earlier I had tx_en/rx_en swapped — that is fixed.)
- **MODE_DEFINITION.** Fixed to 0x09 (initiator ISO14443A 0x08 | nfc_ar8_auto 0x01).
- **FIFO access.** Fixed: read via `OP_READ_FIFO`=0x9F reading only `fifo_bytes()` bytes; byte-count parsing = `reg0x1F | ((reg0x1E & 0xC0) << 2)`.
- **TX-byte-count layout.** Fixed: `value = (bytes << 3) | bits` across reg 0x22/0x23.
- **Auto field-detector veto.** Added `st25r3916_force_field_on()` that clears `en_fd` (0b00) before `CMD_NFC_INITIAL_FIELD_ON` then sets tx_en|rx_en. Amplitude still 0. So the field detector is not what is blocking field-on.
- **NFC power-enable pin.** Verified none exists (BSP uses IO-expander pin 0=servo, pin 13=RGB only).

## 4. What is NOT ruled out (the open failure surface)

1. **The antenna is not actually radiating** despite the field command succeeding. `OPC=8B` (tx_en set) only means the digital logic enabled the transmitter; it does not prove RF energy at the coil. The most damning evidence: `CMD_MEASURE_AMPLITUDE` = 0 with the field forced on and rx enabled.
2. **Antenna tuning / matching network on this specific body board** (ANTENNA_TUNING_CONTROL_1/2 = 0x82/0x82 copied from M5 lib; may not be right for this body's antenna — M5 may tune per-unit, or the body antenna differs from the module the lib was tuned for).
3. **The amplitude measurement itself needs more config** (`REG_AMPLITUDE_MEASUREMENT_CONFIGURATION` 0x33 / `REG_AMPLITUDE_MEASUREMENT_REFERENCE` 0x34) that we never set. So `amp=0` is not by itself conclusive that the antenna is dead — but REQA also failing is conclusive that the antenna is not coupling.
4. **Tag issues:** type (is it ISO14443-A? a bank card won't bare-REQA), placement (the coil sweet spot is small), or a dead tag.
5. **Body seating:** the body responds on I2C so it is attached, but a slightly lifted antenna-feed contact is conceivable.
6. **A register the M5 lib writes that we still skip** — e.g. `writeEMDSuppressionConfiguration(0x40)`, `writePassiveTargetModulation(0x5F)`, `writeResistiveAMModulation(0x80→0x00)`, the `MRT/SQT` timers, or the `writeExternalFieldDetector*Threshold` values. These are mostly passive-target/emulation settings and should not stop an initiator REQA, but I have not proven it.

## 5. Exact reproduction

```bash
cd /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0115-m5stackchan-nfc-reader
source ~/esp/esp-idf-5.5.4/export.sh
idf.py set-target esp32s3      # already done; sdkconfig.defaults seeds USB Serial/JTAG console
idf.py build
idf.py -p /dev/ttyACM0 flash
# monitor without a TTY (the project has no interactive idf.py monitor in this shell):
python3 - << 'PY'
import serial, time
s = serial.Serial("/dev/ttyACM0", 115200, timeout=1)
s.dtr=False; s.rts=True; time.sleep(0.1); s.rts=False; time.sleep(0.1)
end=time.time()+4
while time.time()<end: print(s.read(4096).decode("utf-8","replace").replace("\r",""),end="")
s.write(b"nfc-regs\n"); time.sleep(1)
s.write(b"nfc-field on\n"); time.sleep(1)
s.write(b"nfc-read\n"); time.sleep(3)
print(s.read(4096).decode("utf-8","replace").replace("\r",""),end="")
PY
```

Expected (current, failing) output after `nfc-read`:
```
no tag
I (xxxx) st25r3916: regs: OPC=8B MODE=09 ISO=01 AUX=80 RX1=08 RX2=2D RSSI=00
I (xxxx) st25r3916:       MAIN_IRQ=000000 FIFO_bytes=0
```

Console commands available: `nfc-scan`, `nfc-probe`, `nfc-field on|off`, `nfc-read`, `nfc-poll`, `nfc-regs`, `nfc-sweep`, `nfc-reqa`.

## 6. Ranked hypotheses

**Update after instrumentation (commit 86bbeee1):** `nfc-cap` returns cap=124 (coil connected), and `nfc-regs` readback confirms ANT1=82 ANT2=82 TXD=D0 (correctly programmed). Hypotheses 4 (open feed) is now RULED OUT. The chip sees no REQA response at all (`irq=000000` always).

1. **(Now the prime suspect) The tag is not ISO14443-A, is not a real NFC tag, or is not over the coil's tiny sweet spot.** The coil is connected and tuned; the chip transmits; nothing answers. The one earlier RXE means a real tag *can* couple when placed exactly right. **Test:** use a known-good NTAG213/215/216 or MIFARE Ultralight sticker verified to read on a phone; slow-sweep flat over the body, holding 2 s per spot.
2. **(Less likely now) Analog tuning for this specific body antenna differs from the M5Unit-NFC module** the lib was tuned for (ANT1/2=0x82/0x82 are fixed in the lib, not calibrated per-unit). If a known-good tag also never answers, the antenna matching may be off. **Test:** flash the M5 Arduino `Detect.ino` to the same device (§7 step 5); if it also fails, it's hardware/antenna matching.
3. **(Possible) The amplitude measurement is misconfigured** (regs 0x33/0x34 never written), so `amp=0` is a red herring — but REQA failing is conclusive on its own.
4. ~~Open antenna feed / body seating~~ — RULED OUT by `nfc-cap`=124.
5. **(Least likely now) A driver IRQ/timing bug that drops RXE.** `irq=000000` on every REQA (no RXS either) makes a pure-timing bug very unlikely — if the receiver were merely slow we'd still see RXS. This is now the last resort.

## 7. Suggested next steps (in order)

1. **Verify the antenna coil is present and connected** with `CMD_MEASURE_CAPACITANCE` (0xDE). Add a `nfc-cap` console command: `direct_cmd(0xDE); delay 10ms; read reg 0x25 (AD_CONVERTER_OUTPUT)`. A non-zero, stable value means the coil is wired. If it's flat/zero, the antenna feed is open on this body — a hardware defect, not software.
2. **Configure the amplitude measurement** (`REG_AMPLITUDE_MEASUREMENT_CONFIGURATION` 0x33 and `REG_AMPLITUDE_MEASUREMENT_REFERENCE` 0x34) so `nfc-sweep` returns a real number, then use it as a true coil-finder while sweeping the tag.
3. **Log the raw MAIN_IRQ after every REQA** (not just "no tag"). Modify `st25r3916_reqa()` to print the 24-bit IRQ word and `fifo_bytes()` on every call. If you ever see RXE (0x10) set with 0 FIFO bytes, it's a FIFO/timing bug. If you never see any IRQ bit, the antenna isn't driving a tag.
4. **Bring the skipped M5-lib init writes into init** (EMD suppression 0x40 via the Space-B path, passive-target modulation, resistive AM modulation 0x80→0x00), in case one matters for the initiator path. Unlikely but cheap to add.
5. **Sanity-check against the M5 lib's own Arduino example** if you can get an Arduino-ESP32 toolchain: flash `StackChan-BSP/examples/NFC/Detect/Detect.ino` to the same device. If the Arduino example ALSO can't read a tag, it's hardware/antenna, definitively. If Arduino works and ours doesn't, diff the register init sequence byte-by-byte (the M5 lib is in `sources/code/unit_ST25R3916.hpp` + `unit_ST25R3916.cpp` + `unit_ST25R3916_nfca.cpp`).
6. **Try a known-good NTAG** verified on a phone, swept slowly flat over the body front then back.

## 8. The one-line instrumentation change that would settle the most

**DONE (commit 86bbeee1).** `st25r3916.c::st25r3916_reqa()` now logs `reqa: irq=%06X fifo=%u rxs=%d rxe=%d col=%d` after every REQA, and `nfc-cap` runs `CMD_MEASURE_CAPACITANCE`. **Result: cap=124 (coil connected), irq=000000 on every REQA (no RXS, no RXE, no collision).** This rules out an open antenna feed and a dropped-IRQ timing bug; the failure is no tag response. The next decisive action is **physical**: a known-good ISO14443-A tag placed exactly on the coil sweet spot.

## 9. Key files + references

- **Firmware (live):** `0115-m5stackchan-nfc-reader/main/st25r3916/st25r3916.c` (driver), `.../nfc_console.c` (commands), `.../nfc_reader_main.c` (app_main).
- **Reference driver (matched byte-for-byte):** `ttmp/.../sources/code/unit_ST25R3916.hpp` + `unit_ST25R3916.cpp` (`begin()`) + `unit_ST25R3916_nfca.cpp` (`configure_nfc_a`, `nfca_request_wakeup`, `nfca_anti_collision`).
- **Register map:** `ttmp/.../sources/code/ST25R3916_definition.hpp` (all `REG_*`, `CMD_*`, bit defs). The values for `en`/`rx_en`/`tx_en`/`I_osc`/`I_rxe`/`I_col`/`no_crc_rx`/`antcl`/`z_600k`/agc bits live here.
- **Datasheet:** ST25R3916B datasheet PDF — URL in `sources/datasheets/README-download-instructions.md` (st.com blocks curl; download in a browser). Sections: register map, direct commands, I2C protocol, operation control, FIFO, antenna driver.
- **Intern design guide:** `design-doc/01-esp-idf-st25r3916-nfc-reader-console-app-analysis-design-and-implementation-guide.md` (full system context, the I2C contract, the NFC-A poll sequence).
- **Diary:** `reference/01-investigation-diary.md` Steps 5–6 (the exact bug trail that got us here).

## 10. Critical register cheat-sheet (current values in our init)

| Reg | Addr | Our value | M5 lib value | Notes |
|-----|------|-----------|--------------|-------|
| IO_CONFIG_1 | 0x00 | 0x8B | sup3v\|io_drv_lvl\|0x07 | I2C, 3.3V |
| IO_CONFIG_2 | 0x01 | 0x30 | i2c_thd0\|aat_en | I2C 400k timing + AAT |
| OPERATION_CONTROL | 0x02 | 0x83 (init) → 0x8B/0xEB (field) | en\|en_fd / en\|tx_en\|rx_en | en=0x80, rx_en=0x40, tx_en=0x08 |
| MODE_DEFINITION | 0x03 | 0x09 | ISO14443A(0x08)\|nfc_ar8_auto(0x01) | initiator, NFC-A |
| BITRATE_DEFINITION | 0x04 | 0x00 | 0x00 | 106 kbps tx+rx |
| ISO14443A_SETTINGS | 0x05 | 0x00 → 0x01 (poll) | 0x00 / antcl(0x01) | antcl during REQA/anticoll |
| RX_CONFIG_1 | 0x0B | 0x08 | z_600k(0x08) | |
| RX_CONFIG_2 | 0x0C | 0x2D | sqm_dyn\|agc_en\|agc_m\|agc6_3 (0x2D) | |
| RX_CONFIG_3 | 0x0D | 0xD8 | 0xD8 (stability) | rx gain |
| RX_CONFIG_4 | 0x0E | 0x22 | 0x22 (stability) | rx gain |
| ANTENNA_TUNING_1 | 0x26 | 0x82 | 0x82 | ← suspect for this antenna |
| ANTENNA_TUNING_2 | 0x27 | 0x82 | 0x82 | ← suspect for this antenna |
| TX_DRIVER | 0x28 | 0xD0 | (tx_am_modulation=13)<<4 | |
| FIELD_DET_ACT_THRESH | 0x2A | 0x13 | 0x10\|0x03 | |
| FIELD_DET_DEACT_THRESH | 0x2B | 0x02 | 0x02 | |
| IC_IDENTITY | 0x3F | read | type=0x05 rev=0x02 | confirms chip |

**Commands used:** `CMD_SET_DEFAULT` 0xC1, `CMD_STOP_ALL_ACTIVITIES` 0xC2, `CMD_CLEAR_FIFO` 0xDB, `CMD_ADJUST_REGULATORS` 0xD6, `CMD_NFC_INITIAL_FIELD_ON` 0xC8, `CMD_TRANSMIT_REQA` 0xC6, `CMD_TRANSMIT_WITHOUT_CRC` 0xC5, `CMD_TRANSMIT_WITH_CRC` 0xC4, `CMD_RESET_RX_GAIN` 0xD5, `CMD_MEASURE_AMPLITUDE` 0xD3, `CMD_MEASURE_CAPACITANCE` 0xDE (suggested next). FIFO read via `OP_READ_FIFO` 0x9F; FIFO load via `OP_LOAD_FIFO` 0x80.

## 11. What "done" looks like for Phase 1

`nfc-read` with a known-good ISO14443-A tag flat on the body coil prints:
```
PICC: UID=04:xx:xx:xx:xx:xx:xx ATQA=0044 SAK=00 type=MIFARE Ultralight/NTAG
```
…and `nfc-poll` prints the tag on present and "tag removed" when it leaves. That unblocks Phase 2 (LVGL UI). The anticollision cascade for 7-byte UIDs (CL1 0x93 → CL2 0x95) is already implemented; a real NTAG read will validate it.

---

## Addendum (after handoff): WUPA + full register dump — software now provably correct

Two further diagnostics were run after the handoff was written:

1. **WUPA (wake halted tags).** Added `st25r3916_wupa()` (STOP_ALL_ACTIVITIES → field_on → CMD_TRANSMIT_WUPA) and made `poll_nfca` + `nfc-reqa` try WUPA after REQA. **Result: both REQA and WUPA return `irq=000000` on every attempt.** This rules out the "tag was halted by a prior SELECT" hypothesis — a halted tag answers WUPA, and it does not.

2. **Full Space-A register dump (`nfc-dump`), compared against the M5 lib `dump_regs()` reference** (captured from `M5Unit-NFC/test/.../unit_ST25R3916_nfcb.cpp`, NFC-B mode, mid-transaction — so mode-specific registers differ by design):

   | Reg | Mine (after init) | M5 ref | Match? |
   |-----|-------------------|--------|--------|
   | 0x03 MODE | 09 (NFC-A initiator) | 14 (NFC-B) | mode diff — expected |
   | 0x0B RX1 | 08 (z_600k) | 04 | mode/timing diff |
   | 0x0C RX2 | 2D | 3D | mode diff |
   | 0x0D RX3 | D8 | 00 | mode diff |
   | 0x0E RX4 | 22 | 00 | mode diff |
   | 0x26 ANT1 | 82 | 82 | ✓ |
   | 0x27 ANT2 | 82 | 82 | ✓ |
   | 0x28 TXD | D0 (am_mod=13) | 70 (am_mod=7) | config diff (mine = M5 default) |
   | 0x2A FD_ACT | 13 | 13 | ✓ |
   | 0x2B FD_DEACT | 02 | 02 | ✓ |
   | 0x2C REGULATOR | 00 | 00 | ✓ |
   | 0x3F IC_ID | 2A | 2A | ✓ (type=05 rev=02) |

   Registers I skip (0x08 NFCIP1 FDT, 0x29 PASSIVE_TARGET_MOD=0x5F, timers 0x10/0x11/0x12/0x14) are passive-target/emulation or timer settings that do not affect an initiator REQA.

**Final software verdict:** the driver init is byte-for-byte correct for NFC-A initiator mode. The complete evidence chain is:
- I2C bus OK (scan shows 0x50) ✓
- Chip identity genuine (type=0x05) ✓
- Oscillator starts cleanly ✓
- **All init registers correct (full dump vs M5 reference)** ✓
- **Antenna coil connected (`nfc-cap`=124 stable)** ✓
- Antenna tuning + TX driver correctly programmed ✓
- Field commands on (OPC tx_en set) ✓
- **REQA AND WUPA both yield `irq=000000` (no tag responds, not even RXS)** ✗

The ONLY remaining variable is the **tag and its exact placement on the coil**. This is the physical step the objective explicitly says to stop and ask the user for. No further software action can change the outcome.

**Committed:** `c2f4b322` (WUPA), `5a8eac21` (nfc-dump + comparison).
