---
Title: 'Bug: SPIFFS first-boot format + autoload JS parse errors'
Ticket: 0016-SPIFFS-AUTOLOAD
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - spiffs
    - flash
    - filesystem
    - javascript
    - microquickjs
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/main.c
      Note: SPIFFS init + autoload JS loading + example library generation
    - Path: imports/qemu_storage_repl.txt
      Note: Reference log showing SPIFFS formatting and JS parse errors
    - Path: ttmp/2025/12/29/0016-SPIFFS-AUTOLOAD--bug-spiffs-first-boot-format-autoload-js-parse-errors/analysis/01-bug-report-spiffs-mount-format-autoload-js-parse-errors.md
      Note: Primary bug report
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T14:04:59.821872848-05:00
WhatFor: ""
WhenToUse: ""
---


# Bug: SPIFFS first-boot format + autoload JS parse errors

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32s3
- esp-idf
- qemu
- spiffs
- flash
- filesystem
- javascript
- microquickjs

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
