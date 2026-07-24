---
Title: PULP OS device authorization with embedded tiny-idp Go demo and WebSocket sensor plot
Ticket: ESP-54-PULP-DEVICE-AUTH
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - architecture
    - eink
    - wifi
    - websocket
    - webserver
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Implemented RFC 8628 device authorization, an embedded tiny-idp Go host, protected REST/WebSocket APIs, QR-assisted approval, and an e-ink sensor chart in PULP OS; final fault and soak acceptance remains active.
LastUpdated: 2026-07-23T20:36:50.762963432-04:00
WhatFor: Tracking the implementation and evidence for authenticated PULP OS network applications.
WhenToUse: Read the design guide before implementing server or firmware phases; read the diary before resuming work.
---

# PULP OS device authorization with embedded tiny-idp Go demo and WebSocket sensor plot

## Overview

ESP-54 extends the completed ESP-53 connectivity stack with a native RFC 8628 device-authorization client, bearer-protected REST and WebSocket transports, a Go demo service embedding tiny-idp without modifying it, and an e-ink-aware realtime sensor plot. The primary design document is an intern-oriented implementation guide; the diary records evidence and decisions chronologically.

Current phase: Go service and firmware implementation are committed and hardware-proven. The PaperS3 completes device authorization, protected REST calls, authenticated WSS streaming, QR-assisted approval, and bounded e-ink plotting. Final fault-path probes, sleep/reconnect validation, and the planned 30-minute soak remain open.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active** — core implementation is complete; final hardening and acceptance evidence remain.

## Topics

- papers3
- esp32s3
- microquickjs
- architecture
- eink
- wifi
- websocket
- webserver

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
