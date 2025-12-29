---
Title: 'Track C: Minimal UART echo firmware to isolate QEMU UART RX'
Ticket: 0018-QEMU-UART-ECHO
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - uart
    - serial
    - debugging
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0018-qemu-uart-echo-firmware/README.md
      Note: How to build and run the echo firmware under QEMU
    - Path: 0018-qemu-uart-echo-firmware/main/uart_echo_main.c
      Note: Track C minimal UART0 RX/TX echo loop with heartbeat
    - Path: 0018-qemu-uart-echo-firmware/sdkconfig.defaults
      Note: QEMU-friendly console settings (disable USB-Serial-JTAG secondary console)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/main.c
      Note: Original REPL firmware UART RX loop using uart_read_bytes(UART_NUM_0,...)
    - Path: imports/test_storage_repl.py
      Note: Existing raw TCP client for QEMU tcp serial ports (used for bypass testing)
    - Path: ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/playbooks/01-debugging-plan-qemu-repl-input.md
      Note: Parent investigation playbook (Track C is extracted into this ticket)
    - Path: ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/reference/01-diary.md
      Note: Prior investigation diary (Track A broken; Track B blocked by serial backend mismatch)
    - Path: ttmp/2025/12/29/0015-QEMU-REPL-INPUT--bug-qemu-idf-monitor-cannot-send-input-to-mqjs-js-repl/sources/track-b-bypass-20251229-150439.txt
      Note: 'Evidence: QEMU running with -serial mon:stdio, so localhost:5555 bypass tests fail by design'
ExternalSources:
    - https://github.com/espressif/esp-idf/issues/9369
    - https://nuttx.apache.org/docs/latest/platforms/xtensa/esp32s3/index.html
Summary: ""
LastUpdated: 2025-12-29T15:09:28.464945816-05:00
WhatFor: ""
WhenToUse: ""
---


# Track C: Minimal UART echo firmware to isolate QEMU UART RX

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
- serial
- debugging

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
