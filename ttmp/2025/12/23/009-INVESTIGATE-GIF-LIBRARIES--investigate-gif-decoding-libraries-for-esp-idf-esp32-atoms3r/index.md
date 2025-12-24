---
Title: Investigate GIF decoding libraries for ESP-IDF/ESP32 (AtomS3R)
Ticket: 009-INVESTIGATE-GIF-LIBRARIES
Status: complete
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - animation
    - gif
    - assets
    - serial
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp
      Note: AssetPool mmap injection flow for flash-mapped input
    - Path: ATOMS3R-CAM-M12-UserDemo/main/utils/assets/images/types.h
      Note: AssetPool struct layout and field-size coupling gotchas
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/0010-PORT-ANIMATED-GIF--port-animatedgif-bitbank2-into-esp-idf-project-as-a-component/index.md
      Note: Follow-on implementation ticket (port AnimatedGIF into ESP-IDF)
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md
      Note: Parent ticket architecture constraints and prior decisions
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/reference/01-asset-pipeline-gif-flash-bundled-animation-playback.md
      Note: Storage/pipeline assumptions (flash vs FS
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/009-INVESTIGATE-GIF-LIBRARIES--investigate-gif-decoding-libraries-for-esp-idf-esp32-atoms3r/sources/research-gif-decoder.md
      Note: Final decision record (AnimatedGIF recommended)
    - Path: esp32-s3-m5/0008-atoms3r-display-animation/main/hello_world_main.c
      Note: GC9107/backlight ground truth (esp_lcd path)
    - Path: esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp
      Note: Known-good AtomS3R canvas present + waitDMA pattern
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T17:25:37.653726867-05:00
WhatFor: ""
WhenToUse: ""
---




# Investigate GIF decoding libraries for ESP-IDF/ESP32 (AtomS3R)

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
- animation
- gif
- assets
- serial

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
