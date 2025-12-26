---
Title: 'Bring-up: build/flash/monitor the 0016 signal tester (AtomS3R)'
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - cardputer
    - gpio
    - uart
    - serial
    - usb-serial-jtag
    - debugging
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/README.md
      Note: Project-level usage and command list
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/manual_repl.cpp
      Note: Manual REPL over USB Serial/JTAG (what “the console” really is)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/sdkconfig.defaults
      Note: Default configuration for reproducible bring-up
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/reference/02-cli-contract-0016-signal-tester.md
      Note: Stable command contract (what to type at the prompt)
ExternalSources: []
Summary: "Repeatable bring-up procedure for building/flashing/monitoring the AtomS3R `0016` signal tester and reaching the manual REPL prompt over USB Serial/JTAG."
LastUpdated: 2025-12-26T08:29:13.772349574-05:00
WhatFor: "Get a known-good `0016` firmware on an AtomS3R and confirm the USB Serial/JTAG control plane is responsive."
WhenToUse: "When flashing a new AtomS3R, after updating `0016`, or when the host terminal connection is flaky."
---

# Bring-up: build/flash/monitor the 0016 signal tester (AtomS3R)

## Purpose

Build and flash `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester` onto an **AtomS3R**, then connect to the **USB Serial/JTAG** console and verify you reach the manual prompt (`sig> `) and can run `help` / `status`.

## Environment Assumptions

- Host has **ESP-IDF 5.4.1** installed at `/home/manuel/esp/esp-idf-5.4.1/`.
- AtomS3R is connected over USB; Linux exposes a `ttyACM*` device for USB Serial/JTAG.
- You can run `idf.py` and your user has permission to access the serial device (or you’ll use `sudo` for flashing/monitor).

## Commands

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester && \
idf.py set-target esp32s3 && \
idf.py build
```

### Flash + monitor

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester && \
idf.py flash monitor
```

If you need to specify a port explicitly:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester && \
idf.py -p /dev/ttyACM0 flash monitor
```

## Exit Criteria

- `idf.py build` completes successfully.
- `idf.py flash monitor` shows boot logs and you see a line like:
  - `ready: type 'help' at the USB Serial/JTAG console`
- At the prompt you can type:
  - `help` → prints the command list
  - `status` → prints a one-line status summary
- The AtomS3R LCD updates periodically with mode/pin/tx/rx stats.

## Notes

- **This tester intentionally does not use GROVE UART for the control plane.** All interaction is via USB Serial/JTAG.
- If your terminal behaves oddly (no echo / CRLF weirdness), try a different terminal program or ensure it’s set to “send CRLF” on Enter; the manual REPL accepts `CR`, `LF`, and `CRLF`.
