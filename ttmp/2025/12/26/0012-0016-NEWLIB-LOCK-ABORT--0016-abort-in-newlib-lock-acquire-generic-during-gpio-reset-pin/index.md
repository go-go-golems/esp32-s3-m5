---
Title: '0016: abort in newlib lock_acquire_generic during gpio_reset_pin'
Ticket: 0012-0016-NEWLIB-LOCK-ABORT
Status: review
Topics:
    - atoms3r
    - debugging
    - esp-idf
    - esp32s3
    - gpio
    - usb-serial-jtag
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../esp/esp-idf-5.4.1/components/newlib/locks.c
      Note: lock_acquire_generic abort conditions when xPortCanYield() is false (ISR/critical)
    - Path: 0016-atoms3r-grove-gpio-signal-tester/main/hello_world_main.cpp
      Note: Calls tester_state_init during app_main
    - Path: 0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp
      Note: Crash occurs during tester_state_init()->safe_reset_pin()->gpio_reset_pin while in critical section
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-26T08:55:05.145328934-05:00
WhatFor: ""
WhenToUse: ""
---



# 0016: abort in newlib lock_acquire_generic during gpio_reset_pin

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- atoms3r
- debugging
- esp-idf
- esp32s3
- gpio
- usb-serial-jtag

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
