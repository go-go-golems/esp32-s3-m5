---
Title: 'Mock Zigbee hub: HTTP API + esp_event bus + virtual devices'
Ticket: 0029-HTTP-EVENT-MOCK-ZIGBEE
Status: active
Topics:
    - esp-idf
    - esp32s3
    - http
    - zigbee
    - backend
    - websocket
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T00:08:55.896097047-05:00
WhatFor: "Ticket hub for a mock Zigbee coordinator-style HTTP + esp_event demo (virtual devices, scenes, event stream)."
WhenToUse: "Use when implementing the mock hub firmware or reviewing the API/event-bus design before swapping in a real Zigbee driver."
---

# Mock Zigbee hub: HTTP API + esp_event bus + virtual devices

## Overview

Design/implementation ticket for a “Mock Home Hub” firmware:
- exposes an HTTP API (`esp_http_server`)
- internally routes everything through an application `esp_event` bus
- simulates Zigbee-like join/interview/command/report flows using virtual devices
- provides an SSE/WebSocket stream to observe bus traffic from a browser

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- Design doc: `design-doc/01-mock-zigbee-hub-over-http-event-driven-architecture.md`

## Status

Current status: **active**

## Topics

- esp-idf
- esp32s3
- http
- zigbee
- backend
- websocket

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
