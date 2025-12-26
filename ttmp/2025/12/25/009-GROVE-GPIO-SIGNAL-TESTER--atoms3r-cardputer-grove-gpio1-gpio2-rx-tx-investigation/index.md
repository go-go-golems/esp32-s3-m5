---
Title: 'GROVE GPIO signal tester (GPIO1/GPIO2): investigate AtomS3R RX vs Cardputer TX'
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - cardputer
    - gpio
    - uart
    - serial
    - usb-serial-jtag
    - debugging
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0016-atoms3r-grove-gpio-signal-tester/README.md
      Note: How to build/flash + REPL commands for the new AtomS3R signal tester
    - Path: 0016-atoms3r-grove-gpio-signal-tester/main/console_repl.cpp
      Note: USB Serial/JTAG esp_console commands -> CtrlEvent
    - Path: 0016-atoms3r-grove-gpio-signal-tester/main/gpio_rx.cpp
      Note: GPIO ISR edge counter (RX stats)
    - Path: 0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp
      Note: TX generator (esp_timer square/pulse)
    - Path: 0016-atoms3r-grove-gpio-signal-tester/main/hello_world_main.cpp
      Note: App orchestrator (display+backlight init
    - Path: 0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp
      Note: Single-writer state machine that applies GPIO TX/RX config
ExternalSources: []
Summary: A purpose-built firmware tool to generate/observe signals on GPIO1/GPIO2 (GROVE G1/G2) with a USB Serial/JTAG esp_console control plane and an LCD status UI, to debug why AtomS3R appears not to receive Cardputer TX.
LastUpdated: 2025-12-25T00:00:00Z
WhatFor: Stop guessing about GROVE pin mappings and RX path behavior by building a deterministic, controllable signal source/sink on AtomS3R (with any peer device as the test counterpart).
WhenToUse: When UART TX looks good on a scope but the receiving board reports no RX activity, or when GROVE G1/G2 mapping is uncertain across boards/cables.
---


# GROVE GPIO signal tester (GPIO1/GPIO2): investigate AtomS3R RX vs Cardputer TX

## Overview

We have a real-world debugging problem: **Cardputer can visibly transmit on GROVE G2 (GPIO2)** (confirmed on a scope), but **AtomS3R appears not to observe edges on its expected RX pin (GPIO2 / GROVE G2)**. At this stage, “UART console doesn’t work” is an ambiguous symptom because it mixes:

- pin mapping uncertainty (is “G2” actually GPIO2 on both boards? is the cable pin-to-pin?),
- electrical reachability (does the signal reach the receiving header? is ground shared?),
- software binding (is UART RX routed to the intended pin?), and
- application-level expectations (line endings, echo behavior, REPL lifecycle).

This ticket defines a dedicated firmware tool to turn that ambiguity into a crisp set of yes/no tests.

## Key links

- **Spec / analysis**: see `analysis/01-spec-grove-gpio-signal-tester.md`
- **Tasks**: see `tasks.md`
- **Changelog**: see `changelog.md`
- **CLI contract (stable)**: `reference/02-cli-contract-0016-signal-tester.md`
- **Wiring + pin mapping (stable)**: `reference/03-wiring-pin-mapping-atoms3r-cardputer-grove-g1-g2.md`
- **Test matrix (stable)**: `reference/04-test-matrix-cable-mapping-contention-diagnosis.md`

## Status

Current status: **active** (spec + task plan + initial implementation present as `esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/`).

## Scope note (important)

Per latest clarification: **the firmware we’re building is for AtomS3R only**. The Cardputer (or any other device) is treated as an *external peer / signal source* to test against; we are not planning a Cardputer-side firmware as part of this ticket.

## Structure

- `analysis/` — architecture/spec documents
- `playbooks/` — step-by-step procedures for building/running/testing
- `reference/` — stable contracts (CLI command set, wiring tables, troubleshooting)
- `scripts/` — helper scripts (optional; only if needed)
- `tasks.md` — actionable work items
- `changelog.md` — decisions and changes over time


