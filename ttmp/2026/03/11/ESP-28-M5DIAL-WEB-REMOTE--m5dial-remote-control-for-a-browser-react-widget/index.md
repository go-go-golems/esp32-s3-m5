---
Title: M5Dial remote control for a browser React widget
Ticket: ESP-28-M5DIAL-WEB-REMOTE
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - ui
    - websocket
    - webserver
    - http
    - wifi
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/03/11/ESP-28-M5DIAL-WEB-REMOTE--m5dial-remote-control-for-a-browser-react-widget/design-doc/01-implementation-plan-and-intern-guide.md
      Note: Primary ticket deliverable
    - Path: ttmp/2026/03/11/ESP-28-M5DIAL-WEB-REMOTE--m5dial-remote-control-for-a-browser-react-widget/reference/01-diary.md
      Note: Chronological research log
    - Path: ttmp/2026/03/11/ESP-28-M5DIAL-WEB-REMOTE--m5dial-remote-control-for-a-browser-react-widget/sources/local/01-esp32-knob-web.md
      Note: Imported source note
ExternalSources:
    - local:esp32-knob-web.md
Summary: Ticket workspace for a M5Dial firmware project that connects to an external web server, streams normalized dial input upstream, and controls a React widget served by that server.
LastUpdated: 2026-03-11T23:20:00-04:00
WhatFor: Research, design, and implementation planning for a M5Dial firmware client that controls a server-hosted browser widget.
WhenToUse: Use when implementing the new M5Dial browser-remote project or reviewing the firmware-to-server deployment model and protocol.
---



# M5Dial remote control for a browser React widget

## Overview

This ticket captures the design work for a new M5Dial firmware project that will control a browser-hosted React widget through an external web server. The current design assumes the dial is a client to that server, not the host of the React app.

The work in this ticket is documentation-first. It studies the existing M5Dial firmware (`0072`, `0073`) and the imported server-oriented source note, then translates those findings into a phased implementation guide for a new firmware client project.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- **Primary Design Doc**: `design-doc/01-implementation-plan-and-intern-guide.md`
- **Research Diary**: `reference/01-diary.md`

## Status

Current status: **active**

Current ticket state:

- research complete
- design/implementation guide written
- diary written
- `docmgr doctor` passes cleanly
- previous reMarkable bundle exists; redesign clarification may require a refreshed upload

## Topics

- esp32-s3
- esp32s3
- firmware
- m5stack
- ui
- websocket
- webserver
- http
- wifi

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

## Deliverables

- `design-doc/01-implementation-plan-and-intern-guide.md`
  - detailed intern-facing analysis, architecture, protocol, and phase plan
- `reference/01-diary.md`
  - chronological record of the research and documentation process
- `sources/local/01-esp32-knob-web.md`
  - imported source note that motivated the initial event-pipeline direction
