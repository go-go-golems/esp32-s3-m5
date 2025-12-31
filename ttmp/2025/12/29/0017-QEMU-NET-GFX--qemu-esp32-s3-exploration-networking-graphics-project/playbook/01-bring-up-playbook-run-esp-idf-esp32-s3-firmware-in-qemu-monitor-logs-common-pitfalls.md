---
Title: 'Bring-up playbook: run ESP-IDF ESP32-S3 firmware in QEMU (monitor, logs, common pitfalls)'
Ticket: 0017-QEMU-NET-GFX
Status: active
Topics:
    - esp-idf
    - esp32s3
    - qemu
    - networking
    - graphics
    - emulation
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/build.sh
      Note: Wrapper command entrypoint for idf.py qemu monitor
    - Path: esp32-s3-m5/imports/qemu_storage_repl.txt
      Note: Example of successful QEMU run output and expected prompts
    - Path: esp32-s3-m5/imports/test_storage_repl.py
      Note: Raw TCP approach to validate serial RX without idf_monitor
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T14:07:24.839092266-05:00
WhatFor: ""
WhenToUse: ""
---


# Bring-up playbook: run ESP-IDF ESP32-S3 firmware in QEMU (monitor, logs, common pitfalls)

## Purpose

Bring up **ESP32-S3 QEMU** for an ESP-IDF project in this repo, attach `idf_monitor`, and capture logs in a reproducible way. This is the “do this first” playbook before we attempt networking or graphics exploration.

## Environment Assumptions

- ESP-IDF is installed locally (this repo commonly uses `~/esp/esp-idf-5.4.1`).
- You can run `idf.py` in a shell after sourcing `export.sh`.
- You have (or can install) the ESP-IDF-managed QEMU binary: `qemu-system-xtensa` (via `idf_tools.py install qemu-xtensa`).
- Linux host (commands assume bash/zsh + standard utilities).

## Choose a project to run in QEMU

This repo already contains a known QEMU-friendly example project (even though it’s not “networking + graphics”):

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl`

We’ll use it as the baseline “QEMU works at all” validation.

## Commands

```bash
# 1) Ensure ESP-IDF environment is loaded (fresh shell recommended)
. "$HOME/esp/esp-idf-5.4.1/export.sh" >/dev/null

# 2) Install QEMU for Xtensa via ESP-IDF tool manager (if missing)
python "$IDF_PATH/tools/idf_tools.py" install qemu-xtensa

# 3) Validate the binary is discoverable
command -v qemu-system-xtensa && qemu-system-xtensa --version | head -n 1

# 4) Run the baseline project under QEMU + attach monitor
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl && \
unset ESP_IDF_VERSION && \
./build.sh qemu monitor
```

## Exit Criteria

- `idf.py qemu monitor` launches QEMU and prints something like:
  - `Running qemu on socket://localhost:5555`
  - `--- esp-idf-monitor ... on socket://localhost:5555 115200`
- You see ESP-IDF boot logs (CPU start, app_main, etc.) in the monitor output.

Optional but useful:

- Confirm `Generating flash image: .../build/qemu_flash.bin` appears (shows QEMU image generation is working).

## Notes

### Common pitfalls (seen in this repo)

- **Pitfall: “qemu-system-xtensa is not installed” even after installing**
  - Cause: environment drift (your shell has `ESP_IDF_VERSION` set so wrappers skip sourcing `export.sh`).
  - Fix: run in a fresh shell, or `unset ESP_IDF_VERSION` before using wrapper scripts.

- **Pitfall: partition table mismatch causes runtime failures (filesystem partitions missing)**
  - Symptom: `SPIFFS partition could not be found` (or similar FS mount failure).
  - Typical cause: project is configured for `CONFIG_PARTITION_TABLE_SINGLE_APP=y` so your custom `partitions.csv` is ignored.
  - Fix: set `CONFIG_PARTITION_TABLE_CUSTOM=y` and `CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"` (then rebuild and re-run).

- **Pitfall: monitor input doesn’t reach the app**
  - Symptom: you can see logs (TX works), but typing doesn’t trigger app UART RX reads.
  - Workarounds:
    - Try bypassing `idf_monitor` and send bytes directly to `localhost:5555` (there’s a reference script in this repo: `imports/test_storage_repl.py`).
    - Run QEMU+monitor inside `tmux` so you can attach/detach and test interactive typing.

### Capturing logs (recommended)

Run QEMU in a dedicated terminal and copy/paste the full boot log into a `sources/` text file in the ticket. This becomes your “golden run” reference when debugging regressions.
