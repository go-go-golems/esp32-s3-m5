---
Title: 'QEMU ESP32-S3 exploration: networking + graphics project'
Ticket: 0017-QEMU-NET-GFX
Status: active
Topics:
    - esp-idf
    - esp32s3
    - qemu
    - networking
    - graphics
    - emulation
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/build.sh
      Note: Known-good wrapper to run idf.py commands (including qemu monitor) reproducibly
    - Path: esp32-s3-m5/imports/qemu_storage_repl.txt
      Note: Reference QEMU boot log (socket://localhost:5555)
    - Path: esp32-s3-m5/imports/test_storage_repl.py
      Note: Direct TCP client to localhost:5555 (bypass idf_monitor for RX tests)
    - Path: esp32-s3-m5/ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/index.md
      Note: Existing QEMU serial RX/input investigation
    - Path: esp32-s3-m5/ttmp/2025/12/29/0016-SPIFFS-AUTOLOAD--bug-spiffs-first-boot-format-autoload-js-parse-errors/index.md
      Note: Existing QEMU filesystem/partition behavior investigation
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T14:07:24.72416192-05:00
WhatFor: ""
WhenToUse: ""
---


# QEMU ESP32-S3 exploration: networking + graphics project

## Overview

Create a new ESP-IDF project whose primary purpose is to explore **ESP32-S3 QEMU** support for:

- **Networking** (LwIP, sockets, HTTP client/server behavior, netif quirks, timeouts)
- **Graphics-ish output** (anything we can render/inspect on host while running the firmware under QEMU)

This ticket is intentionally **not about JS**. It exists to build a repeatable QEMU workflow and learn what QEMU can and can’t emulate well, so we can make informed engineering choices later.

## What we know so far (from this repo)

- `idf.py qemu monitor` typically runs QEMU with `-serial tcp::5555,server`, and `idf_monitor` connects to `socket://localhost:5555`.
- Installing the QEMU binary is usually done via ESP-IDF tools:
  - `python "$IDF_PATH/tools/idf_tools.py" install qemu-xtensa`
- A common pitfall is **environment drift**: if `ESP_IDF_VERSION` is already set, wrapper scripts may skip sourcing `export.sh`, so newly installed tools may not be on `PATH`.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp-idf
- esp32s3
- qemu
- networking
- graphics
- emulation

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
