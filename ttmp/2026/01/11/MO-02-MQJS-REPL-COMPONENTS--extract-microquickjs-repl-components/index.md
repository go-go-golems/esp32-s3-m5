---
Title: Extract MicroQuickJS REPL Components
Ticket: MO-02-MQJS-REPL-COMPONENTS
Status: active
Topics:
    - esp32s3
    - console
    - tooling
    - serial
    - cardputer
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Plan to extract ReplLoop, JsEvaluator, and Spiffs into reusable ESP-IDF components with usage guidance."
LastUpdated: 2026-01-11T18:58:40.672331322-05:00
WhatFor: ""
WhenToUse: ""
---

# Extract MicroQuickJS REPL Components

## Overview

Define reusable components for MicroQuickJS REPL plumbing so future firmware can adopt a consistent, tested REPL stack without copy/paste. Current work focuses on inventorying prior art and specifying the extraction plan and usage patterns.

## Key Links

- `analysis/01-existing-components-and-prior-art.md`
- `design-doc/01-component-extraction-plan-replloop-jsevaluator-spiffs.md`
- `reference/01-diary.md`
- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32s3
- console
- tooling
- serial
- cardputer

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
