---
Title: 'Debug: flashing 0011 aborts around 60–70%'
Ticket: 004-CARDPUTER-M5GFX
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - display
    - m5gfx
    - st7789
    - spi
    - animation
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/partitions.csv
      Note: Known-good Cardputer partition table (baseline)
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/sdkconfig.defaults
      Note: Known-good Cardputer defaults (comparison baseline)
    - Path: esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/build/flash_args
      Note: Exact esptool arguments used by idf.py flash
    - Path: esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/partitions.csv
      Note: Tutorial 0011 partition table
    - Path: esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/sdkconfig.defaults
      Note: Tutorial 0011 defaults under suspicion (flash/partition/CPU)
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T22:30:15.518158755-05:00
WhatFor: ""
WhenToUse: ""
---


## Executive summary

The **partition table and flash arguments** for tutorial `0011` match the known-good Cardputer tutorials (`0001`, `0007`). In particular:

- `partitions.csv`: **identical layout** (4M factory app + 1M FAT storage + system partitions)
- `build/flash_args`: **identical esptool flags** (`--flash_mode dio --flash_freq 80m --flash_size 8MB`) and the usual offsets (`0x0` bootloader, `0x8000` partition table, `0x10000` app)

So if flashing aborts around **60–70%** for `0011`, the most likely causes are **serial transport stability** (baud/USB cable/hub/port contention) or a misleading interpretation of what “crash” refers to (e.g. it flashes, then the app reboots/crashes immediately).

Next: capture the exact `idf.py flash` output and then run a short set of controlled experiments (lower baud, `--no-stub`, explicit port, flash a smaller known-good binary).

## What we know so far

- **Symptom (reported):** flashing `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation` “crashes” around **~60–70%** progress.
- **Build status:** `idf.py build` succeeds for `0011`.
- **Observed flash failure output (from /dev/ttyACM0):**

```text
Serial port /dev/ttyACM0

Connecting....
Chip is ESP32-S3 (QFN56) (revision v0.2)
Features: WiFi, BLE, Embedded Flash 8MB (GD)
Crystal is 40MHz
USB mode: USB-Serial/JTAG
...
Uploading stub...
Running stub...
Stub running...
Changing baud rate to 460800
Changed.
Configuring flash size...
...
Compressed 313632 bytes to 170728...
Writing at 0x0003d827... (63 %)
Lost connection, retrying...
Waiting for the chip to reconnect.......
A serial exception error occurred: Could not configure port: (5, 'Input/output error')
Note: This error originates from pySerial. It is likely not a problem with esptool, but with the hardware connection or drivers.
```

**Interpretation:** the USB CDC device likely **disconnected/re-enumerated** mid-transfer right after switching to **460800 baud**. This is a classic “transport” failure, not a partition layout issue.

## New evidence: `/dev/ttyACM0` can disappear (device node missing)

In a subsequent attempt to flash at low baud (115200), the failure happened even earlier:

```text
esptool.py --chip esp32s3 -p /dev/ttyACM0 -b 115200 ... write_flash ...
Serial port /dev/ttyACM0
Connecting...

A serial exception error occurred: [Errno 2] could not open port /dev/ttyACM0: [Errno 2] No such file or directory: '/dev/ttyACM0'
```

This indicates the `/dev/ttyACM0` node can **disappear** (or change) between the time the flash command is invoked and when esptool opens it (USB disconnect/re-enumeration timing).

### Mitigation: use a stable `/dev/serial/by-id/...` path

On this host, the stable symlink exists:

- `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00 -> ../../ttyACM0`

Prefer flashing with:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation && \
idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' -b 115200 flash
```

### Result: flashing succeeded using by-id + 115200

Using the by-id path and keeping baud at 115200 successfully completed the flash of bootloader + app + partition table:

- App write progressed through 9%..100% and verified hash
- Partition table write completed and verified hash
- Hard reset via RTS succeeded

**Conclusion:** this is a transport/re-enumeration issue (not partitions). The stable “known-good” flash command for this machine is:

```bash
idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' -b 115200 flash
```
- **`0011` flash args:**

```text
--flash_mode dio --flash_freq 80m --flash_size 8MB
0x0 bootloader/bootloader.bin
0x10000 cardputer_m5gfx_plasma_animation.bin
0x8000 partition_table/partition-table.bin
```

## Comparison against known-good Cardputer chapters

### Partition table

`0011` uses the same `partitions.csv` as `0007` (and `0001`):

- `nvs @0x9000 size 0x5000`
- `phy_init @0xf000 size 0x1000`
- `factory app @0x10000 size 4M`
- `storage fat size 1M`

**Conclusion:** partition layout is not a differentiator between `0011` and known-good Cardputer chapters.

### Flash arguments

`build/flash_args` for `0011` and `0007` are structurally identical (flags + offsets), differing only in the app filename.

**Conclusion:** `idf.py flash` is not doing anything “special” for `0011` that it wouldn’t also do for `0007`.

### sdkconfig.defaults differences that matter

`sdkconfig.defaults` differences observed:

- `0011`: `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=240`
- `0007`: `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ=160`

**Conclusion:** CPU frequency differences can affect *runtime* stability, but should not explain a flash transport abort. Still, if the “crash” happens immediately after flashing finishes and the board reboots into the app, aligning CPU freq to `160MHz` could be a quick A/B test.

## Hypotheses (ordered)

### H1: Serial transport instability (baud / cable / hub) — **confirmed likely**

When flashing over USB-Serial-JTAG (`/dev/ttyACM*`), high esptool baud rates and/or flaky cables/hubs can cause the connection to drop mid-transfer. This often shows up as a failure after a non-trivial percentage into writing the app binary.

**What would confirm it:**
- Retrying at a **lower baud** succeeds reliably.
- Flashing a **smaller** image succeeds more often than a larger one.

### H2: Port contention (monitor/screen/other process has the device open)

If another process (e.g. `idf.py monitor`, `screen`, IDE serial monitor) keeps the port open, flashing can fail mid-stream or fail to reset correctly.

### H3: Misinterpreting “crash” (flash completes; app crashes on boot)

If the command used was `idf.py flash monitor`, the “percent” may refer to the flash progress, but the failure may actually be the app crashing right after reset.

**What would confirm it:**
- `idf.py flash` completes, but `idf.py monitor` immediately shows an exception/backtrace.

## What to capture (required evidence)

Paste the exact output of the failing flash command, including:

- command used (`idf.py flash`, `idf.py flash monitor`, any `-p`/`-b`)
- device port (`/dev/ttyACM0`, etc.)
- the final error line (e.g. timeout, invalid header, serial exception)

## Debug playbook (do these in order)

### 1) Explicit port + lower baud (most likely fix)

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation && \
idf.py -p /dev/ttyACM0 -b 115200 flash
```

Then try `-b 230400` if 115200 is stable but too slow.

### 2) Try without the stub loader

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation && \
python -m esptool --chip esp32s3 --port /dev/ttyACM0 --no-stub write_flash @build/flash_args
```

### 3) A/B: flash a known-good small Cardputer app

If `0007` flashes reliably but `0011` doesn’t, that points strongly to “size/transfer stability”.

### 4) If it’s actually a runtime crash

Run:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation && \
idf.py -p /dev/ttyACM0 monitor
```

and capture the exception/backtrace.

