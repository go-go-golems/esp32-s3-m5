---
Title: Port AnimatedGIF (bitbank2) into ESP-IDF project as a component
Ticket: 0010-PORT-ANIMATED-GIF
Status: complete
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - animation
    - gif
    - m5gfx
    - assets
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp
      Note: Reference for esp_partition_mmap (flash-mapped asset input)
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md
      Note: Parent system architecture and constraints
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/009-INVESTIGATE-GIF-LIBRARIES--investigate-gif-decoding-libraries-for-esp-idf-esp32-atoms3r/sources/research-gif-decoder.md
      Note: 'Decision record: choose AnimatedGIF and why'
    - Path: esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp
      Note: Known-good AtomS3R M5GFX canvas present + waitDMA pattern
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/0010-PORT-ANIMATED-GIF--port-animatedgif-bitbank2-into-esp-idf-project-as-a-component/playbooks/01-compile-gif-assets-to-flash-fatfs-and-load-on-device.md
      Note: Copy/paste workflow (crop/trim → fatfsgen → parttool → console playback) + troubleshooting kinks
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T19:10:07.702091657-05:00
WhatFor: ""
WhenToUse: ""
---



# Port AnimatedGIF (bitbank2) into ESP-IDF project as a component

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
- m5gfx
- assets

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
