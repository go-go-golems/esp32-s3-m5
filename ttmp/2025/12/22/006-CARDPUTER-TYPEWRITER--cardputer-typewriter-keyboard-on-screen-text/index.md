---
Title: 'Cardputer: typewriter (keyboard -> on-screen text)'
Ticket: 006-CARDPUTER-TYPEWRITER
Status: closed
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - keyboard
    - display
    - m5gfx
    - ui
    - text
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp
      Note: M5GFX autodetect ground truth for Cardputer display/backlight
    - Path: M5Cardputer-UserDemo/main/apps/app_texteditor/app_texteditor.cpp
      Note: Vendor text editor behavior (scroll/cursor/backspace)
    - Path: M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp
      Note: Vendor keyboard scan implementation (multi-key + modifiers)
    - Path: M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h
      Note: Vendor key map (4x14 labels + pin lists)
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/005-CARDPUTER-SYNTH--cardputer-synthesizer-audio-and-keyboard-integration/analysis/01-cardputer-speaker-and-keyboard-architecture-analysis.md
      Note: Keyboard matrix scan ground truth (pins/mapping) from vendor + tutorial
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/005-CARDPUTER-SYNTH--cardputer-synthesizer-audio-and-keyboard-integration/analysis/02-cardputer-graphics-with-m5gfx-text-sprites-ui-patterns.md
      Note: M5GFX text/sprite UI patterns; vendor text editor reference
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Known-good ESP-IDF keyboard scan + debounce/autodetect
    - Path: esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: Known-good M5GFX autodetect + full-screen canvas present path
    - Path: esp32-s3-m5/0012-cardputer-typewriter/README.md
      Note: Build/flash instructions (by-id) and expected behavior
    - Path: esp32-s3-m5/0012-cardputer-typewriter/main/Kconfig.projbuild
      Note: 0012 Kconfig surface for keyboard/typewriter knobs
    - Path: esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp
      Note: Typewriter implementation (keyboard scan + buffer + redraw)
    - Path: esp32-s3-m5/0012-cardputer-typewriter/sdkconfig.defaults
      Note: Reproducible Cardputer defaults for 0012
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-02T09:33:44.49508917-05:00
WhatFor: ""
WhenToUse: ""
---





# Cardputer: typewriter (keyboard -> on-screen text)

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
- cardputer
- keyboard
- display
- m5gfx
- ui
- text

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
