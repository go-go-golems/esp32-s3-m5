---
Title: 'AtomS3R: serial console controlled GIF display (flash-bundled animations)'
Ticket: 008-ATOMS3R-GIF-CONSOLE
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - m5gfx
    - animation
    - gif
    - assets
    - serial
    - usb-serial-jtag
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ATOMS3R-CAM-M12-UserDemo/bootloader_components/boot_hooks/boot_hooks.c
      Note: Bootloader hook that suppresses USB Serial/JTAG enumeration (gotcha if we want serial console)
    - Path: ATOMS3R-CAM-M12-UserDemo/main/utils/assets/assets.cpp
      Note: Example generating AssetPool.bin blob (bundle assets)
    - Path: ATOMS3R-CAM-M12-UserDemo/partitions.csv
      Note: Example custom assetpool partition layout (2MB)
    - Path: ATOMS3R-CAM-M12-UserDemo/upload_asset_pool.sh
      Note: Example parttool.py write_partition workflow
    - Path: echo-base--openai-realtime-embedded-sdk/components/esp-protocols/components/console_simple_init/console_simple_init.c
      Note: esp_console REPL init over USB Serial/JTAG
    - Path: esp32-s3-m5/0008-atoms3r-display-animation/main/hello_world_main.c
      Note: AtomS3R esp_lcd GC9107 bring-up and backlight control ground truth
    - Path: esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp
      Note: Known-good AtomS3R canvas animation + waitDMA pattern to reuse
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp
      Note: Phase A MVP implementation (mock animations + button + esp_console over USB Serial/JTAG)
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild
      Note: Kconfig knobs for button/backlight/display parameters used by the Phase A MVP
    - Path: esp32-s3-m5/0013-atoms3r-gif-console/sdkconfig.defaults
      Note: Tutorial defaults (enables the relevant console/backlight/display config)
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T15:00:42.411449594-05:00
WhatFor: ""
WhenToUse: ""
---


# AtomS3R: serial console controlled GIF display (flash-bundled animations)

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32s3
- esp-idf
- atoms3r
- display
- m5gfx
- animation
- gif
- assets
- serial
- usb-serial-jtag
- ui

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
