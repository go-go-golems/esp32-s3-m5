---
Title: 'Cardputer: screenshot PNG to USB-Serial/JTAG can WDT when driver uninitialized'
Ticket: 0024-B3-SCREENSHOT-WDT
Status: review
Topics:
    - cardputer
    - m5gfx
    - screenshot
    - usb-serial-jtag
    - watchdog
    - esp-idf
    - debugging
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp
      Note: Buggy busy-loop write path used by screenshot feature
    - Path: esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md
      Note: Primary bug report + suspected root cause + validation
    - Path: esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/02-postmortem-screenshot-png-over-usb-serial-jtag-caused-wdt-root-cause-fix.md
      Note: New-developer postmortem (what/why/how fixed + future pitfalls)
ExternalSources: []
Summary: B3 screenshot-to-serial can spin forever when USB-Serial/JTAG driver is unavailable and trigger Task WDT.
LastUpdated: 2026-01-01T21:41:03.224600831-05:00
WhatFor: ""
WhenToUse: ""
---




# Cardputer: screenshot PNG to USB-Serial/JTAG can WDT when driver uninitialized

## Overview

The demo-suite’s screenshot feature (`P`) currently uses `usb_serial_jtag_write_bytes()` in a retry loop that can become an infinite busy-loop if the driver is not initialized, leading to log spam and Task WDT resets. This ticket tracks the investigation and a safe fix (fail fast + bounded retries + yield + explicit driver init/guard).

## Key Links

- Bug report: `analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md`
- Diary context: `esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/02-implementation-diary.md`
- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- cardputer
- m5gfx
- screenshot
- usb-serial-jtag
- watchdog
- esp-idf
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
