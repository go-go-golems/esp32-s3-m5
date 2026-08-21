# StackChan NFC Debug UI

A reproducible source overlay for the upstream M5Stack StackChan firmware. It adds a Mooncake/LVGL NFC diagnostics app without vendoring the full upstream firmware into this repository.

## Upstream

- Repository: `https://github.com/m5stack/StackChan.git`
- Commit: `1b5765599fba8aaad1811d9a79358ccc7051f5f3`
- Firmware toolchain: ESP-IDF 5.5.4

The pinned revision is recorded in `upstream.env`. `scripts/prepare.sh` creates a disposable composed checkout under `.work/StackChan`, applies `overlay/`, and patches app registration idempotently.

## Prepare and build

```bash
cd 0116-m5stackchan-nfc-debug-ui
./scripts/prepare.sh
source ~/esp/esp-idf-5.5.4/export.sh
./scripts/build.sh
```

For a local upstream clone:

```bash
STACKCHAN_SOURCE=/tmp/nfc-research/repos/StackChan ./scripts/prepare.sh
```

The composed firmware is generated at `.work/StackChan/firmware`. `.work/` is ignored and may be deleted at any time.

## Architecture

- `NfcDebugService` is the sole owner of NFC I2C operations.
- UI callbacks enqueue fixed-size commands and return immediately.
- The worker publishes complete `NfcDebugSnapshot` values through a one-element overwrite queue.
- The UI reads snapshots and mutates LVGL only while holding `LvglLockGuard`.
- The driver attaches to the existing shared bus returned by `hal_bridge::board_get_i2c_bus()`; it never creates a second bus.

## Current phase

UI-0 establishes the pinned overlay, service command/snapshot contract, shared-bus driver lifecycle, and build baseline. Visual pages are added in later phases.
