---
Title: 'Serial Monitor: structured parsing + multiplexed capabilities'
Ticket: ESP-01-SERIAL-MONITOR
Status: active
Topics:
    - serial
    - console
    - tooling
    - esp-idf
    - usb-serial-jtag
    - debugging
    - display
    - esp32s3
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper
      Note: Implementation repo
    - Path: esper/firmware/esp32s3-test/main/main.c
      Note: Test firmware
    - Path: ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/design-doc/01-serial-monitor-as-stream-processor-parsing-multiplexing-screenshots.md
      Note: Main design doc
    - Path: ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/design-doc/02-go-serial-monitor-idf-py-compatible-design-implementation.md
      Note: Go reimplementation design+implementation doc
    - Path: ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/reference/01-diary.md
      Note: |-
        Research diary
        Diary steps with esper commit hashes
    - Path: ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/01-upload-to-remarkable.sh
      Note: Upload bundle to reMarkable
    - Path: ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/02-capture-screenshot-png.py
      Note: Unified PNG capture tool
    - Path: ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/04-upload-go-monitor-design-doc-to-remarkable.sh
      Note: Upload only this doc to reMarkable
    - Path: ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/scripts/06-tmux-exercise-esper.sh
      Note: tmux exercise harness
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T14:05:23.238981834-05:00
WhatFor: ""
WhenToUse: ""
---




# Serial Monitor: structured parsing + multiplexed capabilities

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- serial
- console
- tooling
- esp-idf
- usb-serial-jtag
- debugging
- display
- esp32s3

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
