---
Title: PULP OS device authorization with embedded tiny-idp Go demo and WebSocket sensor plot
Ticket: ESP-54-PULP-DEVICE-AUTH
Status: complete
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
Summary: Completed RFC 8628 device authorization, an embedded tiny-idp Go host, protected REST/WebSocket APIs, QR-assisted approval, bounded e-ink plotting, fault validation, and hardware soak acceptance in PULP OS.
LastUpdated: 2026-07-23T23:58:40.012874938-04:00
WhatFor: Tracking the implementation and evidence for authenticated PULP OS network applications.
WhenToUse: Read the design guide before implementing server or firmware phases; read the diary before resuming work.
---


# PULP OS device authorization with embedded tiny-idp Go demo and WebSocket sensor plot

## Overview

ESP-54 extends the completed ESP-53 connectivity stack with a native RFC 8628 device-authorization client, bearer-protected REST and WebSocket transports, a Go demo service embedding tiny-idp without modifying it, and an e-ink-aware realtime sensor plot. The primary design document is an intern-oriented implementation guide; the diary records evidence and decisions chronologically. A post-completion maintainer report analyzes tiny-idp integration friction and proposes public API, tooling, documentation, example, and onboarding improvements.

Current phase: implementation and acceptance complete. The PaperS3 completes device authorization, QR-assisted approval, protected REST, authenticated WSS streaming, bounded e-ink plotting, denial/expiry/sleep/reconnect handling, parser fault batteries, and the final 30-minute SENSOR LINK panel soak.

## Key Links

- [Device authorization and realtime demo implementation guide](./design-doc/01-device-authorization-and-realtime-demo-analysis-design-and-intern-implementation-guide.md)
- [Tiny-IDP and ESP32 integration friction analysis and maintainer improvement guide](./design-doc/02-tiny-idp-and-esp32-integration-friction-analysis-and-maintainer-improvement-guide.md)
- [Investigation diary](./reference/01-investigation-diary.md)
- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **complete** — implementation, hardware validation, final soak, documentation, and focused commits are complete.

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
