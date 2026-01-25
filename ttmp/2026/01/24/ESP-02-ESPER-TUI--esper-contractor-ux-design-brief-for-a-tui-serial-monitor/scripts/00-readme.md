---
Title: 'ESP-02-ESPER-TUI scripts'
Ticket: ESP-02-ESPER-TUI
Status: active
Topics:
    - serial
    - console
    - tooling
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Runnable scripts for reproducing hardware and CLI test steps for ESP-02-ESPER-TUI."
LastUpdated: 2026-01-25T00:15:00-05:00
WhatFor: "Capture exact command sequences as executable scripts for repeatability and audit."
WhenToUse: "Use when reproducing or updating hardware/CLI smoke tests for esper."
---

# ESP-02-ESPER-TUI scripts

These scripts capture the exact, repeatable command sequences used for hardware testing and TUI smoke tests.

## Usage

Run from the repo root:

- Detect port: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/01-detect-port.sh`
- Build firmware: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/02-build-firmware.sh`
- Flash firmware: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/03-flash-firmware.sh`
- Scan ports: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/04-esper-scan.sh`
- Run Esper TUI: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/05-run-esper-tui.sh`
- Run Esper tail (non-TUI): `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/06-run-esper-tail.sh`
- Run Esper tail (bidirectional raw): `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/07-run-esper-tail-stdin-raw.sh`
- Automated pty smoke test for stdin-raw: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/08-pty-smoke-test-esper-tail-stdin-raw.py`
- Capture TUI screenshots in tmux: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`
- Upload UX compare docs to reMarkable: `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/10-upload-remarkable-ux-compare.sh`

## Environment overrides

- Set `ESPPORT` to override auto-detection:
  - `ESPPORT=/dev/ttyACM0 ./.../03-flash-firmware.sh`
- Set `ESPIDF_EXPORT` to override ESP-IDF export script:
  - `ESPIDF_EXPORT=$HOME/esp/esp-idf-5.4.1/export.sh`
- Set `ESPER_ELF` to override the ELF used by Esper for decoding:
  - `ESPER_ELF=$PWD/esper/firmware/esp32s3-test/build/esp32s3-test.elf`
- Set `TOOLCHAIN_PREFIX` to override addr2line prefix:
  - `TOOLCHAIN_PREFIX=xtensa-esp32s3-elf-`
