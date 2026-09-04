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

## Serial diagnostics

NFC.LAB emits structured diagnostics through the board's USB Serial/JTAG console at 115200 baud. After flashing, capture a complete boot and interaction session with one serial owner only:

```bash
cd .work/StackChan/firmware
idf.py -p /dev/ttyACM0 monitor 2>&1 | tee /tmp/nfc-lab-serial.log
```

Press `Ctrl+]` to stop the monitor before flashing or opening another serial tool. Useful record prefixes are:

- `NFC_INIT`: reader initialization, identity, capacitance, transaction totals, and failure context.
- `NFC_I2C_FAIL`: every failed low-level transaction, including sequence number, operation, register/command key, ESP-IDF error, and elapsed time.
- `NFC_READ`: tag UID/ATQA/SAK or rate-limited no-tag summaries.
- `NFC_COMMAND`: command result and any transport-failure delta observed during it.
- `NFC_RF`: REQA/WUPA IRQ, FIFO, collision, and error evidence; no-response records are DEBUG level while RF events remain INFO.
- `NFC_SAMPLE` and `NFC_VERIFY`: long-running diagnostic start/completion summaries.

For a focused failure timeline:

```bash
rg 'NFC_(INIT|I2C_FAIL|READ|COMMAND|RF|SAMPLE|VERIFY)' /tmp/nfc-lab-serial.log
```

## Architecture

- `NfcDebugService` is the sole owner of NFC I2C operations.
- UI callbacks enqueue fixed-size commands and return immediately.
- The worker publishes complete `NfcDebugSnapshot` values through a one-element overwrite queue.
- The UI reads snapshots and mutates LVGL only while holding `LvglLockGuard`.
- The driver attaches to the existing shared bus returned by `hal_bridge::board_get_i2c_bus()`; it never creates a second bus.

## Current phase

The current firmware boots directly into NFC.LAB. UI-0 through UI-2 provide the serialized service, Reader page, RF/IRQ page, and I2C Bus page. Register/log and continuous-operation phases follow.
