---
Title: M5Dial browser-to-device command path
Ticket: ESP-29-M5DIAL-BROWSER-COMMANDS
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
RelatedFiles: []
ExternalSources: []
Summary: Browser-originated `ui_command` frames now route through the Go server to the M5Dial firmware, with request-correlated acknowledgements and a React command UI.
LastUpdated: 2026-03-11T19:34:02.752286437-04:00
WhatFor: Capture the implementation and validation details for the browser-to-device command path added to the M5Dial web remote stack.
WhenToUse: Use when reviewing or extending browser-driven device commands, websocket routing, or firmware acknowledgements for project `0074-m5dial-web-remote`.
---

# M5Dial browser-to-device command path

## Overview

This ticket covers the second half of the M5Dial web remote architecture: commands that originate in the browser and are applied on the dial. The work is implemented and validated on live hardware.

Current behavior:

- React sends `ui_command` frames over `/ws/browser`.
- The Go hub validates the target `device_id`, forwards the payload to the active device socket, and replies to the browser with `ui_command_result`.
- The firmware parses recognized commands on the websocket client, applies them on the app task, and emits `ui_command_ack` with the original `request_id`.
- The server records the ack in history and rebroadcasts it to browser clients as a normal device event.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

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
