# Provenance: 07-espidf-full-dump-for-comparison + 02 comparison analysis

- **Captured:** 2026-08-21, ESP-60 Step 33 (Phase 5 comparison)
- **Firmware:** `0115-m5stackchan-nfc-reader` debug build (`CONFIG_I2C_ENABLE_DEBUG_LOG=y`),
  still flashed from Step 32.
- **Serial:** `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_44:1B:F6:E2:80:28-if00`,
  115200 baud, single-owner inline probe.
- **Tag state:** NO tag present.

## Captures

- `07-espidf-full-dump-for-comparison.txt`: `nfc-trace clear`, `nfc-read --attempts 8`,
  `nfc-trace dump` (full 512-event ring), `nfc-trace first-error`. Yields 512
  `I2C_TRACE` events (500 `irq-wait` + 11 `req-setup` + 1 `req-tx`) with 5 NACK
  failures on the polling keys.
- `02-official-arduino-four-chip-full-i2c-trace.log.gz`: the existing 4-chip Arduino
  control (10,187 `M5_I2C` events, 0 transport failures).

## Comparison (`analysis/02-arduino-vs-espidf-trace-comparison.json`)

Produced by `scripts/08-compare-arduino-espidf-traces.py` (design S12):

- **Summary:** Arduino init/detect/identify = 0 failures across 10,187 events;
  ESP-IDF irq-wait = 5 failures / 500 events (1%), max elapsed 1244 us (FSM-reset
  recovery), median 177 us.
- **Key coverage (apples-to-apples):**
  - wire `0x5C` (READ_A register 0x1C, ERROR_AND_WAKEUP): Arduino 2614/2614 OK,
    ESP-IDF 247 events / 3 failures.
  - wire `0x5A` (IRQ_R MAIN_INTERRUPT 2-byte read): Arduino 2925/2925 OK,
    ESP-IDF 246 events / 2 failures.
  - Both backends touch the same register set; Arduino succeeds 100% on the exact
    keys where ESP-IDF NACKs.
- **Divergence:** ESP-IDF first failure `seq=3707 phase=irq-wait op=READ_A wire=0x5C
  logical=0x1C ESP_ERR_INVALID_STATE`; Arduino same wire key `0x5C`: 2614 events,
  0 failures.

## Interpretation

This is the apples-to-apples transport comparison the design called for: the same
I2C operations (same wire keys, same register addresses) succeed 100% under the
M5 direct backend and fail intermittently (NACK) under the ESP-IDF new master
driver. Combined with the Step 32 driver-DEBUG confirmation, the divergence is
isolated to the ESP-IDF host-controller path (NACK -> non-DONE -> INVALID_STATE),
not the ST25R3916 register set or the tag.

## Caveats

- The two captures are from different runs/conditions (Arduino: 4 tags present;
  ESP-IDF: no tag). The comparison is transport-level (per wire-key success
  rates), not an event-by-event alignment. The script deliberately does not
  force an event-by-event divergence across mismatched runs.
- The ESP-IDF window is 512 events (ring capacity); earlier init transactions
  were overwritten by polling, so the ESP-IDF side is dominated by `irq-wait`.
- `hint=UNKNOWN`/`class=NOT_DONE_UNKNOWN` in this capture because the dump was
  taken before `nfc-trace annotate nack`; Step 32 separately confirmed NACK via
  the driver DEBUG log and annotated a different run's first error.
