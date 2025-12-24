---
Title: Cardputer Synthesizer - Audio and Keyboard Integration
Ticket: 005-CARDPUTER-SYNTH
Status: active
Topics:
    - audio
    - speaker
    - keyboard
    - cardputer
    - esp32-s3
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/main/apps/app_record/app_record.cpp
      Note: Example usage showing speaker/microphone mutual exclusion and audio playback
    - Path: M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp
      Note: HAL initialization including speaker and keyboard setup
    - Path: M5Cardputer-UserDemo/main/hal/hal_cardputer.h
      Note: HAL interface definitions
    - Path: M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp
      Note: Keyboard implementation with GPIO matrix scanning algorithm
    - Path: M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h
      Note: Keyboard class header with key mapping and GPIO pin definitions
    - Path: M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.cpp
      Note: Speaker implementation with I2S setup
    - Path: M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.hpp
      Note: Speaker class header with API definitions and configuration structures
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Tutorial implementation with improved keyboard scanning including debouncing and autodetection
ExternalSources: []
Summary: "Ticket hub for building a Cardputer synth: how to read the keyboard (GPIO matrix) and make sound on the speaker (I2S via Speaker_Class)."
LastUpdated: 2025-12-22T22:19:02.303748941-05:00
WhatFor: "Central entry point and curated links for the Cardputer synth work: speaker output + keyboard input, grounded in the vendor UserDemo and tutorial 0007."
WhenToUse: "Start here when you’re working on Cardputer audio or keyboard input, or when you need to find the key implementation files quickly."
---


# Cardputer Synthesizer - Audio and Keyboard Integration

## Overview

This ticket collects the practical implementation details needed to build a small “synthesizer-like” application on the M5Stack Cardputer: **read key presses from the built-in keyboard** and **emit sound on the built-in speaker**.

The analysis is based on two “ground truth” sources in this repo:

- `M5Cardputer-UserDemo/` (vendor code): contains the actual `Speaker_Class` and `Keyboard` implementations used on Cardputer.
- `esp32-s3-m5/0007-cardputer-keyboard-serial/` (tutorial): reimplements the keyboard scan loop in pure ESP-IDF and adds debouncing + pin autodetect.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- **Main analysis**: `analysis/01-cardputer-speaker-and-keyboard-architecture-analysis.md`

## Status

Current status: **active**

## Topics

- audio
- speaker
- keyboard
- cardputer
- esp32-s3

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
