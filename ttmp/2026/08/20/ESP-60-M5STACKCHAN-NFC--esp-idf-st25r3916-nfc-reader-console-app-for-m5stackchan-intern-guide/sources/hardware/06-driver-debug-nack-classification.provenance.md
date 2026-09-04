# Provenance: 06-driver-debug-nack-classification

- **Captured:** 2026-08-21, ESP-60 Step 32 (Phase 4 driver-DEBUG classification)
- **Firmware:** `0115-m5stackchan-nfc-reader` rebuilt with `CONFIG_I2C_ENABLE_DEBUG_LOG=y`
  (sdkconfig.defaults), runtime level raised via the new `nfc-i2c-debug on` command.
- **Serial:** `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_44:1B:F6:E2:80:28-if00`,
  115200 baud, single-owner probe `scripts/07-probe-i2c-driver-debug.py`.
- **Tag state:** NO tag present (transport classification only).
- **Commands:** `nfc-i2c-debug on`, `nfc-trace clear`, `nfc-read --attempts 10`,
  `nfc-i2c-debug off`, `nfc-trace annotate nack`, `nfc-trace status`, `nfc-trace first-error`.

## Decisive result: the failure is I2C_EVENT_NACK

The ESP-IDF I2C master driver printed, at DEBUG level, for failing transactions:

```
D (7299) i2c.master: I2C transaction unexpected nack detected
D (7389) i2c.master: I2C transaction unexpected nack detected
...
```

This is the `ESP_LOGD(TAG, "I2C transaction unexpected nack detected")` line at
`i2c_master.c:162`, reached from `s_i2c_err_log_print(event, bypass_nack_log)` at
line 606 when `event == I2C_EVENT_NACK` and the transaction is not a probe
(`bypass_nack_log == false` for normal register reads).

Counts in the captured window:
- `unexpected nack detected`: 18 captured (61 total failures this run; the DEBUG
  lines are frequent and some fell outside the probe's settle window).
- `transaction timeout detected` (ERROR, `i2c_master.c:158`): **0**
- `bus is still busy but software timeout detected` (ERROR): **0**

So the `ESP_ERR_INVALID_STATE` results are **purely NACK events**, not timeouts.
This confirms design doc 04 Section 3 / S9.1: the synchronous path maps
`I2C_EVENT_NACK` -> status stays non-DONE -> `s_i2c_transaction_start` returns
`ESP_ERR_INVALID_STATE` (`i2c_master.c:725-727`).

## Annotated first error (this boot)

After `nfc-trace annotate nack`, the frozen first error was upgraded from
`hint=UNKNOWN class=NOT_DONE_UNKNOWN` to:

```
ERROR seq=326 phase=irq-wait op=IRQ_R logical=1A wire=5A wlen=1 rlen=2
       api=ESP_ERR_INVALID_STATE hint=NACK class=HOST_NACK flags=FIRST_ERROR
       elapsed_us=226
```

- This boot the first NACK hit `IRQ_R logical=1A` (the 2-byte MAIN_INTERRUPT read
  inside `read_main_irq()`), whereas the Step 31 boot first hit `READ_A 0x1C`.
  Both legs of the `read_main_irq()` polling pair (0x1C read + 0x1A 2-byte read)
  are subject to NACK; the failure hits whichever is in flight.
- 16-event prefix: all `ESP_OK` (irq-wait polling), gap 2-13 us (busy-spin).

## Back-to-back failure + recovery

The suffix captured an immediate second failure:

```
SUFFIX seq=327 op=READ_A logical=1C api=ESP_ERR_INVALID_STATE elapsed_us=1378
SUFFIX seq=328 op=IRQ_R   logical=1A api=ESP_OK            elapsed_us=243
```

`seq=327` took 1378 us — far longer than the ~200 us norm — indicating the
post-error path ran `s_i2c_hw_fsm_reset(clear_bus=true)` (`i2c_master.c:684`,
triggered when `status == TIMEOUT || i2c_ll_is_bus_busy()`), then the transaction
itself NACKed again. `seq=328` then succeeded, so recovery completes within one
extra failed transaction. This matches the design's note that ESP-IDF resets the
FSM after a synchronous error and recovers, but M5GFX resets preventively before
every transaction (design S4).

## Interpretation

- The "strong inference" from design doc 04 / Step 29 is now **confirmed at the
  driver-event level**: ESP-IDF's new master driver observes `I2C_EVENT_NACK` on
  ST25R3916 polling transactions and exposes it as `ESP_ERR_INVALID_STATE`.
- The remaining open question is the **physical byte stage** of the NACK
  (address ACK vs register-command ACK vs data ACK) — that requires SDA/SCL
  logic-analyzer capture (design Experiment C), which this run does not provide.
- The failures remain load-correlated (irq-wait busy-spin at ~2.6 kHz) and
  transient; `read_main_irq()` still masks them as "no IRQ" / "no tag".

## Caveats

- Only 18 of 61 NACK DEBUG lines were captured in the probe window; the count is
  a lower bound, not the exact total. The trace ring's `failed=61` is the
  authoritative total.
- `nfc-i2c-debug on` raises only the `i2c.master` tag to DEBUG; the rest of the
  system stays at INFO, so the DEBUG lines are cleanly attributable to the driver.
- Opening the serial port resets the board (fresh boot each probe run); the
  annotate step had to run after a read that generated failures, not on a
  freshly-reset idle board (an earlier attempt returned `FIRST_ERROR: none`).
