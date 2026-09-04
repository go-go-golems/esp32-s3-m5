# Provenance: 05-standalone-trace-runtime

- **Captured:** 2026-08-21, ESP-60 Step 31 hardware validation
- **Firmware:** `0115-m5stackchan-nfc-reader` (ESP-IDF 5.5.4) with the observer-safe
  `st25r_trace` ring (Phase 1-3), recording mode = `all` from init.
- **Board:** M5StackChan CoreS3 (ESP32-S3), ST25R3916 at I2C 0x50.
- **Serial:** `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_44:1B:F6:E2:80:28-if00`,
  115200 baud, single-owner probe script `scripts/06-probe-st25r-trace.py`.
- **Tag state:** NO tag present (no-tag transport validation run).
- **Commands exercised:** `nfc-trace status`, `nfc-probe`, `nfc-regs`,
  `nfc-read --attempts 25`, `nfc-trace status`, `nfc-trace dump --last 40`,
  `nfc-trace first-error`.

## Key results

1. **Boot init was clean:** `recorded=66 failed=0 first_error=none`. The earlier
   "txn-65 init failure" is intermittent; this boot's init succeeded 66/66.
2. **`nfc-probe` OK:** type=0x05 rev=0x02.
3. **96 transport failures out of 13,816 transactions (0.69%)**, all during the
   `irq-wait` REQA/WUPA polling loop. Ring capacity 512 overflowed 13,304 times.
4. **All 25 `nfc-read` attempts reported `no tag`** — but the trace ring proves
   96 of those poll iterations actually hit `ESP_ERR_INVALID_STATE`, silently
   masked by `read_main_irq()` (returns 0 = "no IRQ" on I2C failure). The
   `nfc-read` transport notice printed: `transport: 96 failed transaction(s)`.
5. **First error (frozen, survived 13,304 overwrites):**
   `seq=426 phase=irq-wait op=READ_A logical=1C wire=5C api=ESP_ERR_INVALID_STATE
   elapsed_us=251 hint=UNKNOWN class=NOT_DONE_UNKNOWN flags=FIRST_ERROR`
   - Register `0x1C` = ERROR_AND_WAKEUP_INTERRUPT (read via `i2c_master_transmit_receive`).
   - `elapsed_us=251` vs ~176-201 us for surrounding successes → consistent with
     the NACK -> STOP -> non-DONE -> INVALID_STATE path (design S3), slightly
     longer than a clean DONE.
   - 16-event prefix and 16-event suffix all `ESP_OK` → the failure is transient
     and the chip/driver recovers immediately on the next transaction.
6. **Polling cadence:** consecutive `irq-wait` transactions show `gap_us` of 2-13 us,
   i.e. the `wait_irq` loop busy-spins the I2C bus at ~2.6 kHz. Root cause:
   `vTaskDelay(pdMS_TO_TICKS(1))` rounds to `vTaskDelay(0)` at the default 100 Hz
   FreeRTOS tick (yield-only, ~12 us), so the loop does not actually sleep 1 ms.

## Interpretation

- The ESP-IDF new-master transport is unstable under rapid-fire polling, not
  under the slower init/diagnostic reads (which were clean this boot).
- The application is blind to these failures: `read_main_irq()` treats a failed
  I2C read as "no IRQ", so a transport failure during `wait_irq` is
  indistinguishable from "tag not answering". Only the trace ring surfaces them.
- This does NOT by itself prove a physical NACK byte; `hint=UNKNOWN` is preserved
  until driver DEBUG (`CONFIG_I2C_ENABLE_DEBUG_LOG`) or SDA/SCL evidence annotates
  it (Phase 4). The 251 us elapsed is consistent with, not proof of, the NACK path.

## Caveats

- The `nfc-read` log tail interleaves slightly with the following `nfc-trace status`
  command because the probe's settle window (2.5 s) was shorter than the 25-attempt
  run (~1.75 s of polling plus per-attempt REQA+WUPA). The trace data itself
  (status/dump/first-error) is not affected; only the human-readable REQA/WUPA
  log lines overlapped the next command echo.
- No tag was present, so this run does not address the UID-reading failure; it
  isolates the transport layer under load.
- The board now runs the standalone 0115 firmware (the Arduino persistent monitor
  was overwritten by the full flash). Reflash via PlatformIO to restore it.
