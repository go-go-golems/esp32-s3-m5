---
Title: 'Plan: Port AnimatedGIF into ESP-IDF (component + AtomS3R playback harness)'
Ticket: 0010-PORT-ANIMATED-GIF
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - animation
    - gif
    - m5gfx
    - assets
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T17:23:03.254329681-05:00
WhatFor: ""
WhenToUse: ""
---

## Executive summary

Ticket 009 concluded that **AnimatedGIF** is the best fit for our AtomS3R GIF playback project because it supports **memory-backed GIF sources** and a **scanline draw callback** that exposes an **RGB565 palette** (a clean match for an M5GFX RGB565 canvas).

This ticket scopes the concrete work required to make that practical in our repo:

- vendor/pin AnimatedGIF at a known upstream revision
- package it as a normal **ESP-IDF component**
- implement a minimal AtomS3R harness proving: mmap → decode → RGB565 canvas → `pushSprite()` + `waitDMA()`
- write down the recommended configuration trade-offs (scanline vs cooked+framebuffer)

## Inputs (starting references)

- Primary research write-up:
  - `009-INVESTIGATE-GIF-LIBRARIES/sources/research-gif-decoder.md`
- Parent architecture constraints:
  - `008-ATOMS3R-GIF-CONSOLE/analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md`
- Known-good display/present pattern:
  - `esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp`

## Non-goals (for this ticket)

- Building the full serial console UX (`esp_console`) and animation directory (that belongs to ticket 008)
- Building the host asset pipeline (also ticket 008)

## Deliverables

- **ESP-IDF component**: `components/animatedgif/` with a minimal, documented port surface
- **AtomS3R harness**: a tiny app that can play a test GIF from memory (and ideally from `esp_partition_mmap`)
- **Porting notes**: what we changed, why, and how to update the pinned upstream revision

