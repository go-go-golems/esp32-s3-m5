---
Title: GROVE serial debug (GPIO spikes + wiring/coupling analysis)
Ticket: 0013-GROVE-SERIAL-DEBUG
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - gpio
    - uart
    - debugging
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp
      Note: TX apply sequencing (reset + output enable + level) where enable-glitch can be introduced
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/manual_repl.cpp
      Note: Maps 'tx low' command to CtrlType::TxLow
    - Path: esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp
      Note: Defines apply ordering; resets pins and triggers gpio_tx_stop/gpio_tx_apply
    - Path: esp32-s3-m5/ttmp/2025/12/25/009-GROVE-GPIO-SIGNAL-TESTER--atoms3r-cardputer-grove-gpio1-gpio2-rx-tx-investigation/reference/01-diary.md
      Note: Prior investigation diary; contains 0016 architecture + related experiments
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-26T17:01:26.594804262-05:00
WhatFor: Investigate and explain small transient spikes observed on both GROVE signal wires (GPIO1/GPIO2) when commanding TX low in the 0016 signal tester; capture hypotheses, code-level causes, and verification experiments.
WhenToUse: When you observe unexpected spikes/edges during GROVE UART debugging, and want to correlate scope observations with 0016 firmware behavior (pin reset/apply semantics) and likely hardware coupling artifacts.
---


# GROVE serial debug (GPIO spikes + wiring/coupling analysis)

## Overview

This ticket focuses on a specific observation while using `0016-atoms3r-grove-gpio-signal-tester`: when selecting GPIO2 as the active TX pin and issuing `tx low`, a small transient spike can be observed on **both GPIO2 and GPIO1**.

The goal is to explain the most likely causes (firmware sequencing vs physical coupling vs measurement artifact), and to define quick experiments that distinguish them.

## Key Links

- Analysis: [Why does tx low on GPIO2 create a spike on GPIO1+GPIO2?](./analysis/01-why-does-tx-low-on-gpio2-create-a-spike-on-gpio1-gpio2-0016-signal-tester-hardware-coupling.md)
- Diary: [Diary](./reference/01-diary.md)
- Design: [0016 UART TX/RX modes](./design-doc/01-0016-add-uart-tx-rx-modes-for-grove-uart-functionality-speed-testing.md)
- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32s3
- esp-idf
- atoms3r
- gpio
- uart
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
