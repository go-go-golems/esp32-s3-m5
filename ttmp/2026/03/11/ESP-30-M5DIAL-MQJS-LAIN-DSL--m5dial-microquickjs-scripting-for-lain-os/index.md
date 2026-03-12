---
Title: M5Dial microquickjs scripting for Lain OS
Ticket: ESP-30-M5DIAL-MQJS-LAIN-DSL
Status: active
Topics:
  - esp32-s3
  - esp32s3
  - firmware
  - javascript
  - ui
  - websocket
  - webserver
  - http
  - wifi
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Detailed design and implementation plan for adding a MicroQuickJS service and browser-delivered JavaScript DSL to the 0074 M5Dial Lain OS stack.
LastUpdated: 2026-03-11T21:18:35-04:00
WhatFor: Explain how to extend the current Lain-themed M5Dial firmware, Go server, and React UI with an on-device JavaScript runtime and a safe DSL over radio/device primitives.
WhenToUse: Use when implementing or reviewing script execution, DSL bindings, websocket script transport, or app-state ownership in 0074-m5dial-web-remote.
---

# M5Dial microquickjs scripting for Lain OS

## Overview

This ticket covers the next architectural step after the Lain OS radio UI: run `microquickjs` on the M5Dial itself, expose a JavaScript API for the Lain radio/device primitives, and deliver scripts from the browser to the device over the existing websocket topology.

The deliverable is documentation-first. It maps the current 0074 runtime, identifies the reusable `0048-cardputer-js-web` QuickJS service pattern, and proposes a phased implementation that preserves the existing rule that the application task owns visible device state.

## Key Links

- Primary guide: `design-doc/01-analysis-design-and-implementation-guide.md`
- Diary: `reference/01-diary.md`
- Related Files: see frontmatter

## Status

Current status: **active**

Research and design package complete. Implementation has not started yet.

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.
