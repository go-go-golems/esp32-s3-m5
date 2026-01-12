---
Title: Cardputer-ADV MicroJS GPIO Exercizer
Ticket: MO-01-JS-GPIO-EXERCIZER
Status: active
Topics:
    - esp32s3
    - cardputer
    - gpio
    - serial
    - console
    - esp-idf
    - tooling
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Ticket workspace for a Cardputer-ADV MicroQuickJS GPIO/protocol signal exercizer (REPL-driven)."
LastUpdated: 2026-01-11T18:35:17.077332491-05:00
WhatFor: ""
WhenToUse: ""
---

# Cardputer-ADV MicroJS GPIO Exercizer

## Overview

Build a Cardputer-ADV firmware that runs MicroQuickJS with a REPL and can generate repeatable GPIO, UART, and I2C signals to validate logic analyzer decoding. Current work focuses on surveying reusable firmware patterns and brainstorming the JS API/runtime architecture.

## Key Links

- `analysis/01-existing-firmware-analysis-and-reuse-map.md`
- `design-doc/01-js-api-and-runtime-brainstorm.md`
- `design-doc/02-high-frequency-gpio-pattern-vm-brainstorm.md`
- `design-doc/03-concrete-implementation-plan-js-gpio-exercizer-gpio-i2c.md`
- `reference/01-diary.md`
- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32s3
- cardputer
- gpio
- serial
- console
- esp-idf
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
