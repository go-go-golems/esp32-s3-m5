# Provenance: 08-fsm-reset-patch-runtime + 09-reverted-baseline-runtime

- **Captured:** 2026-08-21, ESP-60 Step 35 (FSM-reset diagnostic patch experiment)
- **Firmware:** `0115-m5stackchan-nfc-reader` debug build, two flashes:
  - `08-fsm-reset-patch-runtime.txt` — ESP-IDF patched with the **unconditional**
    `s_i2c_hw_fsm_reset(i2c_master, false)` in `s_i2c_transaction_start`
    (patch `sources/code/esp-idf-5.5.4-i2c-fsm-reset-diagnostic.patch`).
  - `09-reverted-baseline-runtime.txt` — patch reverted, original gated reset restored.
- **Patch verification:** disassembly of the patched `i2c_master.c.obj` confirmed the
  unconditional `callx8` to `s_i2c_hw_fsm_reset` with `a11=0` (clear_bus=false) at the
  function prologue, no guarding branch. Object rebuilt 21:08:03 > source 21:07:16.
- **Serial:** `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_44:1B:F6:E2:80:28-if00`,
  single-owner probe `scripts/07-probe-i2c-driver-debug.py` (`nfc-i2c-debug on`,
  `nfc-trace clear`, `nfc-read --attempts 25`, `nfc-i2c-debug off`, `nfc-trace annotate nack`,
  `nfc-trace status`, `nfc-trace first-error`). No tag present.

## Decisive result: HYPOTHESIS REFUTED

The leading hypothesis (design doc 05 / Step 34) was that the NACKs come from
inherited dirty I2C command-FSM state, and that a **preventive per-transaction
`fsm_rst`** (mirroring M5GFX) would eliminate them. The experiment refutes it:

| Build | failures / total | rate | first failure | first phase |
|---|---|---|---|---|
| **Patched** (unconditional fsm_rst, clear_bus=false) | 213 / 11,807 | **1.80%** | seq ~12 | **field-on / req-setup** |
| **Reverted baseline** (gated fsm_rst) | 144 / 12,274 | **1.17%** | seq 32 | **irq-wait** (READ_A 0x1C) |

- The patched build failed **more** (1.80% vs 1.17%), not less.
- The patched build introduced failures in **field-on / req-setup** — phases that
  were **always clean** in every prior run (Steps 31, 32, and the reverted baseline).
- The reverted baseline restored the original profile exactly: failures only in
  `irq-wait` polling, first error `READ_A 0x1C` (`ESP_ERR_INVALID_STATE`,
  `hint=NACK` after annotate), the same pattern as Steps 31–32.
- Zero `I2C transaction timeout detected` lines in either build (failures remain
  pure NACK in both).

So `fsm_rst` alone is **not** the fix and is actively **harmful**: resetting the
command FSM right before every transaction, without M5GFX's surrounding
bus-idle-wait + pin re-route + mode reinit, disrupts transactions that were
previously fine.

## Why my hypothesis was wrong

M5GFX `beginTransaction` does a **full controller reinit** on every transaction —
acquire lock, wait for bus-idle (up to 128 µs), `save_reg`, `set_pin` (re-route
SDA/SCL), `fsm_rst`, timeout reinit, disable interrupts, master-mode reinit, FIFO
reset, timing — of which `fsm_rst` is only one line (`common.cpp:2000`). My patch
replicated only the `fsm_rst` line with `clear_bus=false` (which on ESP32-S3 is
*just* `i2c_ll_master_fsm_rst`, no full reinit). The bare `fsm_rst` without the
bus-idle wait and full reinit context perturbs the settling bus. The experiment
therefore tests `fsm_rst`-alone, and `fsm_rst`-alone is refuted.

## Revised direction (replaces design doc 05 Section 7's expected outcome)

The real M5GFX-vs-ESP-IDF difference is broader than `fsm_rst`. Remaining
candidates, in revised order:

1. **Full per-transaction controller reinit discipline** — specifically the
   bus-idle wait *before* touching the controller, plus pin re-route + mode
   reinit, not just the FSM bit. A faithful M5GFX-mirror patch would do the full
   reinit, not the bare bit.
2. **Command sequencing / framing** — how the driver programs START/address/
   repeated-START/final-read-NACK/STOP may differ in a way the ST25R3916 is
   sensitive to (this is what the defined-operations backend, design doc 04
   Phase 6, is meant to test).
3. **SDA/SCL signal-level difference** — still unproven; needed to locate the
   NACK byte stage and rule out a marginal analog/edge effect.

The decisive next step is **SDA/SCL logic-analyzer capture** on GPIO12/GPIO11
during a failing `READ_A 0x1C` (`wire 0x5C`) read: it will show whether the NACK
lands on the address byte, the command byte, or the data byte, and whether the
START/STOP edges look clean. That distinguishes framing (candidate 2) from
analog (candidate 3) and constrains the full-reinit experiment (candidate 1).

## State

- ESP-IDF source reverted to original (gated reset); `git diff` clean at
  commit `73550728`. The patch file is retained in `sources/code/` for
  reproducibility (it documents a refuted hypothesis, which is itself valuable).
- Board runs the reverted baseline `0115` debug build.
