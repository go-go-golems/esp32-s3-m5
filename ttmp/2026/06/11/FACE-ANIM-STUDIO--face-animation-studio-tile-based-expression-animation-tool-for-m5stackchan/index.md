---
Title: Face Animation Studio — Tile-Based Expression Animation Tool for M5StackChan
Ticket: FACE-ANIM-STUDIO
Status: active
Topics:
    - m5stackchan
    - animation
    - frontend
    - esp32
    - tooling
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: face-animation-studio/assets/sheets/sheet1.png
      Note: Sprite sheet 1 — 4x4 grid
    - Path: face-animation-studio/assets/sheets/sheet2.png
      Note: Sprite sheet 2 — 4x4 grid
    - Path: face-animation-studio/assets/sheets/sheet3.png
      Note: Sprite sheet 3 — 4x4 grid
    - Path: face-animation-studio/assets/sheets_clock/clock_red.png
      Note: Red clock expression sprite sheet (blink frame pair)
    - Path: face-animation-studio/assets/sheets_clock/clock_white.png
      Note: White clock expression sprite sheet
    - Path: face-animation-studio/scripts/05-extract_sprite_pairs.py
      Note: Weighted cross-correlation sprite pair alignment script
    - Path: face-animation-studio/scripts/normalize_tiles.sh
      Note: Reproducible tile normalization pipeline (crop→clean→trim→scale→bottom-align)
    - Path: face-animation-studio/src/animation-engine.js
      Note: Play/pause/stop state machine with requestAnimationFrame
    - Path: face-animation-studio/src/app.js
      Note: Entry point
    - Path: face-animation-studio/src/index.html
      Note: Main HTML with 3-panel layout
ExternalSources: []
Summary: ""
LastUpdated: 2026-06-11T19:17:43.67316667-04:00
WhatFor: ""
WhenToUse: ""
---





# Face Animation Studio — Tile-Based Expression Animation Tool for M5StackChan

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- m5stackchan
- animation
- frontend
- esp32
- tooling

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
