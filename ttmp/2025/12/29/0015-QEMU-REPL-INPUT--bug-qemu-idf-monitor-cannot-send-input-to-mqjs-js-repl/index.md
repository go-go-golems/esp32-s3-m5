---
Title: 'Bug: QEMU idf_monitor cannot send input to mqjs JS REPL'
Ticket: 0015-QEMU-REPL-INPUT
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - uart
    - console
    - serial
    - microquickjs
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/main.c
      Note: UART0 REPL read loop (uart_read_bytes)
    - Path: imports/test_storage_repl.py
      Note: Direct TCP test client for port 5555 (bypass idf_monitor)
    - Path: ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/analysis/01-bug-report-qemu-monitor-input-not-delivered-to-uart-repl.md
      Note: Primary bug report
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T14:00:32.611760447-05:00
WhatFor: ""
WhenToUse: ""
---


# Bug: QEMU idf_monitor cannot send input to mqjs JS REPL

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
- uart
- console
- serial
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
