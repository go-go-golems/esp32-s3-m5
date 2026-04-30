---
Title: Almanach Render Service — Go Headless Chrome API Server
Ticket: ALMANACH-RENDER-SERVICE
Status: active
Topics:
    - go
    - chrome-headless
    - api-server
    - almanach
    - stoms3r
    - rendering
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: stoms3r/cmd/almanach-render-service/bitmap.go
      Note: PNG → 1-bit MSB-first bitmap conversion
    - Path: stoms3r/cmd/almanach-render-service/layout.go
      Note: All 15 block type structs
    - Path: stoms3r/cmd/almanach-render-service/main.go
      Note: Entry point — HTTP server
    - Path: stoms3r/cmd/almanach-render-service/printer.go
      Note: ESP32 /api/print/bitmap HTTP client
    - Path: stoms3r/cmd/almanach-render-service/renderer.go
      Note: Chrome headless render orchestration — chromedp allocator
    - Path: stoms3r/main/index.html
      Note: SToMS3R printer web UI — has Floyd-Steinberg dithering and bitmap packing JS code
    - Path: stoms3r/main/printer_drv.h
      Note: Printer driver API — printer_drv_print_bitmap
    - Path: stoms3r/main/web_server.c
      Note: ESP32 HTTP server — has the /api/print/bitmap endpoint and serves the SPA
    - Path: stoms3r/web/almanach/esbuild.mjs
      Note: Build pipeline — produces IIFE bundle from JSX
    - Path: stoms3r/web/almanach/src/almanach-studio.jsx
      Note: The React SPA component (~2200 lines) that renders the almanac page — this is what Chrome headless will load and screenshot
    - Path: stoms3r/web/almanach/src/index.jsx
      Note: Entry point that mounts the React component
ExternalSources: []
Summary: ""
LastUpdated: 2026-04-29T18:37:30.160666577-04:00
WhatFor: ""
WhenToUse: ""
---



# Almanach Render Service — Go Headless Chrome API Server

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- go
- chrome-headless
- api-server
- almanach
- stoms3r
- rendering

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
