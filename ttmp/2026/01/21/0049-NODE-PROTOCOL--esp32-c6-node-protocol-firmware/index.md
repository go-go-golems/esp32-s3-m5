---
Title: ESP32-C6 node protocol + firmware
Ticket: 0049-NODE-PROTOCOL
Status: active
Topics:
    - esp32c6
    - wifi
    - console
    - esp-idf
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T14:45:57.398304931-05:00
WhatFor: ""
WhenToUse: ""
---

# ESP32-C6 node protocol + firmware

## Overview

This ticket produces a detailed, implementation-ready analysis of how to build an **ESP32‑C6** firmware that:

- provisions **Wi‑Fi STA** via `esp_console` (scan/join/save),
- joins a **UDP multicast** group and implements the **MLED/1** node protocol,
- presents itself on the network via `PING`/`PONG` discovery and status reporting,
- supports synchronized control via “two-phase cues” (`CUE_PREPARE` + `CUE_FIRE`) and show-time scheduling.

## Key Links

- Design (main deliverable): `design-doc/01-esp32-c6-node-firmware-as-a-network-node-protocol-design.md`
- Diary (detailed work log): `reference/01-diary.md`
- Protocol sources index (mled-web): `reference/02-protocol-sources-mled-web.md`

## Status

Current status: **active**

## Topics

- esp32c6
- wifi
- console
- esp-idf

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
