---
Title: Synchronized Serial Capture Validation
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - hardware-qualification
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Validation and failure record for the shared-host-clock Printalyzer and PaperS3 serial event collector."
LastUpdated: 2026-07-14T23:45:00Z
WhatFor: "Establish what is timestamped, what commands are allowed, and whether serial attachment perturbs either device."
WhenToUse: "Before any synchronized optical-density run or interpretation of host-receipt timestamps."
---

# Synchronized serial capture validation

## Result

**Conditional PASS.** Printalyzer capture, read-only inventory, raw-stream state sequencing under a fake device, and truly passive ESP32 read-only attachment pass. Actual Printalyzer raw-stream illumination has **not** been run. A pyserial ESP32 attachment attempt caused a real reset into ROM download mode and is preserved as a failed observer-control test.

## Captured time model

Each NDJSON event records:

- one process-global sequence;
- source and per-source line sequence;
- host `CLOCK_MONOTONIC` nanoseconds;
- UTC nanoseconds derived from fixed realtime/monotonic anchors;
- first- and last-byte host monotonic timestamps;
- raw line hex plus decoded text;
- parsed Printalyzer density/raw-sensor fields or ESP log/device timestamp fields where recognized.

UTC is derived from the monotonic anchor so wall-clock adjustments cannot reorder a run.

## Read-only Printalyzer inventory

Evidence: `29-printalyzer-read-only-inventory-20260714T2340Z.jsonl`

```text
GS V,"Printalyzer Densitometer","v1.1.0"
GS B,"2023-06-13 17:41","g7101373",8E155935
GS DEV,1.10.6 ,0x447,0x2008,32MHz
GS UID,323147103439323344002900
GS ISEN,3313mV,27.0C
GM REFL,7FC00000
GC LIGHT,128,122
```

The remaining gain, slope, reflection, and transmission calibration getters also returned successfully. `GM REFL` decoded as the IEEE-754 NaN sentinel, consistent with no prior reflection result in the current session. No setter, invoke, remote-mode, light, sensor, or calibration command was sent.

## Preserved observer failure

Evidence: `29-dual-passive-smoke-20260714T2341Z.jsonl`

The first dual-source implementation used pyserial for both ports and attempted to protect the ESP32 by opening it with DTR and RTS deasserted. That assumption was wrong. Merely opening the ESP32-S3 USB Serial/JTAG tty produced:

```text
ESP-ROM:esp32s3-20210327
rst:0x15 (USB_UART_CHIP_RESET),boot:0x0 (DOWNLOAD(USB/UART0))
waiting for download
```

This reset the board into ROM download mode. It did not boot FactoryTest or intentionally drive the panel, but it was a real observer effect and invalidates that attachment method. The board is intentionally left in download mode pending an explicit operator-authorized reset because booting F0 would replay the display sequence.

The collector now opens the firmware tty with `os.open(O_RDONLY | O_NOCTTY | O_NONBLOCK)` and never issues modem-control ioctls or firmware writes. Evidence `29-firmware-read-only-fd-smoke-20260714T2345Z.jsonl` shows a clean open/close with:

```json
{"open_mode":"read-only-os","modem_control_issued":false}
```

No reset output or input occurred.

## Printalyzer CDC requirement

Opening the Printalyzer with DTR deasserted caused the first read-only inventory to time out at `GS V`; evidence is `29-printalyzer-read-only-inventory-20260714T2338Z.jsonl`. The firmware only treats CDC as host-connected while DTR is asserted. The corrected Printalyzer-specific pyserial path uses DTR true and RTS false. Unlike the ESP32 path, this does not control a boot strap.

## Raw-stream synthetic test

A pseudo-terminal fake Printalyzer validated the exact gated command and cleanup sequence:

```text
IS REMOTE,1
SD S,CFG,1,0
SD LR,32
ID S,START
ID S,STOP
SD LR,0
IS REMOTE,0
```

Nine synthetic `GD S,...` records were parsed. The final three cleanup commands were verified in order. Cleanup is armed before the first state-changing command so partial-entry failures still attempt sensor stop, reflection-light off, and remote-mode exit.

## Safety gate

Actual raw streaming requires the literal CLI token:

```text
--confirm ENABLE-DENS-RAW-STREAM
```

The mode is explicitly labeled uncalibrated. It must not be run until probe geometry, gain, integration time, light duty, saturation limits, and a separate experiment ledger are reviewed.

## Interpretation limits

Common host timestamps substantially improve correlation, but they are not exact physical event timestamps:

- serial receipt occurs after sensor integration and USB buffering;
- Printalyzer integration index contributes a known but not yet measured latency;
- F2 firmware ring records are dumped after panel idle, so their internal device times must be joined to the host timeline using anchors or optical transition edges;
- F0 has no firmware ring;
- raw Printalyzer channels are not calibrated reflection density;
- neither serial source measures panel rails, VCOM, temperature at the panel, or current.

## Validation commands

```text
python3 -m py_compile scripts/29-capture-synchronized-serial.py
scripts/29-capture-synchronized-serial.py --check
scripts/29-capture-synchronized-serial.py --execute --no-firmware --dens-inventory --duration 1 ...
scripts/29-capture-synchronized-serial.py --execute --no-densitometer --duration 0.3 ...
scripts/30-test-synchronized-serial.py
```
