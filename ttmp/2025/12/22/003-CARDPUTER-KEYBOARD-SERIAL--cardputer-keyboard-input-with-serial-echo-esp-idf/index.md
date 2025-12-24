---
Title: 'Cardputer: on-screen typewriter (ESP-IDF)'
Ticket: 003-CARDPUTER-KEYBOARD-SERIAL
Status: closed
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - keyboard
    - display
    - m5gfx
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp
      Note: Vendor Cardputer display autodetect + ST7789/backlight config
    - Path: ../../../../../../M5Cardputer-UserDemo/main/apps/app_chat/app_chat.cpp
      Note: Vendor input panel + blinking cursor implementation
    - Path: ../../../../../../M5Cardputer-UserDemo/main/apps/app_texteditor/app_texteditor.cpp
      Note: Vendor on-device text editor patterns (enter/backspace/cursor)
    - Path: ../../../../../../M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp
      Note: Vendor keyboard scan + key state derivation
    - Path: ../../../../../../esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Known-good keyboard matrix scan and key mapping
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T22:06:49.592029323-05:00
WhatFor: ""
WhenToUse: ""
---



# Cardputer: on-screen typewriter (ESP-IDF)

## Overview

Build a small “typewriter” demo for **M5Cardputer** under **ESP-IDF**:

- Read keys from the built-in **keyboard matrix**
- Render the typed text on the **Cardputer screen** in real time
- Support basic editing: **backspace**, **enter/newline**, **wrapping/scrolling**, and a blinking **cursor**

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
