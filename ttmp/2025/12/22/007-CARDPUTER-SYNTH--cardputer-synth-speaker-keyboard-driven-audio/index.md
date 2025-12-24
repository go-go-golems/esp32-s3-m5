---
Title: 'Cardputer: synth + speaker (keyboard-driven audio)'
Ticket: 007-CARDPUTER-SYNTH
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - audio
    - speaker
    - i2s
    - keyboard
    - synth
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp
      Note: Speaker/mic pin mapping + I2S port selection ground truth
    - Path: M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.cpp
      Note: Speaker I2S setup + task loop + resampler/mixer internals
    - Path: M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.hpp
      Note: Speaker public API (tone/playRaw/playWav) and config knobs
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/005-CARDPUTER-SYNTH--cardputer-synthesizer-audio-and-keyboard-integration/analysis/01-cardputer-speaker-and-keyboard-architecture-analysis.md
      Note: Prior audio+keyboard architecture analysis to reuse
    - Path: esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp
      Note: Known-good keyboard scan + event-driven redraw loop pattern
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T23:13:35.466038168-05:00
WhatFor: ""
WhenToUse: ""
---


# Cardputer: synth + speaker (keyboard-driven audio)

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
- audio
- speaker
- i2s
- keyboard
- synth

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
