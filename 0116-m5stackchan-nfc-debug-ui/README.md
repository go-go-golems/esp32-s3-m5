# StackChan NFC Debug UI

A reproducible source overlay for the upstream M5Stack StackChan firmware. It produces an **NFC-only** Mooncake/LVGL diagnostic firmware without vendoring the full upstream firmware into this repository. Standard StackChan apps are excluded from compilation and NFC.LAB opens automatically at boot.

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

## Flash

The first migration from a standalone project requires a full flash because the StackChan partition table and generated-assets partition differ:

```bash
source ~/esp/esp-idf-5.5.4/export.sh
./scripts/flash.sh --full
```

Subsequent NFC iterations write only the combined application partition:

```bash
./scripts/flash.sh app
```

This still writes one ESP-IDF app image, but standard Mooncake app implementations are excluded from that image.

## Architecture

- `NfcDebugService` is the sole owner of NFC I2C operations.
- UI callbacks enqueue fixed-size commands and return immediately.
- The worker publishes complete `NfcDebugSnapshot` values through a one-element overwrite queue.
- The UI reads snapshots and mutates LVGL only while holding `LvglLockGuard`.
- The driver attaches to the existing shared bus returned by `hal_bridge::board_get_i2c_bus()`; it never creates a second bus.

## Current phase

The current firmware boots directly into NFC.LAB. UI-0 through UI-2 provide the serialized service, Reader page, RF/IRQ page, and I2C Bus page. Register/log and continuous-operation phases follow.
