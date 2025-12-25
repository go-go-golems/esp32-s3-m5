---
Title: "Tasks: GROVE GPIO signal tester (GPIO1/GPIO2)"
Ticket: 009-GROVE-GPIO-SIGNAL-TESTER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - gpio
    - uart
    - debugging
DocType: tasks
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Implementation checklist for the GROVE GPIO signal tester tool."
LastUpdated: 2025-12-25T00:00:00.000000000+00:00
WhatFor: "Concrete tasks to build and validate the signal tester firmware."
WhenToUse: "As the authoritative list of work items for this ticket."
---

## Milestone A — Define the tool contract (no code)

- [ ] Write a stable CLI command contract (command names, args, examples)
- [ ] Write a wiring/pin mapping reference table (AtomS3R and Cardputer GROVE pin labels → GPIO numbers)
- [ ] Define the test matrix (cable orientation, pin swap cases, expected observations)

## Milestone B — Build the firmware skeleton (two-board strategy)

We have two viable approaches:

- **Option 1 (recommended)**: two tiny projects
  - `00xx-atoms3r-grove-signal-tester/`
  - `00xx-cardputer-grove-signal-tester/`
  Each can have a board-specific LCD/UI implementation but share the same CLI contract.

- **Option 2**: one project with `menuconfig` board selection and conditional build.

Tasks:

- [ ] Decide Option 1 vs Option 2 (and record rationale in `changelog.md`)
- [ ] Create the ESP-IDF project(s) with minimal dependencies
- [ ] Bring up `esp_console` over USB Serial/JTAG (control plane)
- [ ] Add a periodic LCD “status screen” (mode/pin/state/counters)

## Milestone C — GPIO transmit modes (TX)

- [ ] Set pin to output and drive steady high/low
- [ ] Generate square wave at selectable frequency (software toggle, timer-based, or RMT)
- [ ] Generate a “pulse train” pattern useful for scope/logic-analyzer trigger
- [ ] Add “inverted” option (helpful when debugging swapped pins)

## Milestone D — GPIO receive modes (RX)

- [ ] Set pin to input with selectable pull (none/pullup/pulldown)
- [ ] Count edges via GPIO ISR (rising/falling/both)
- [ ] Record last-edge timestamp and estimate frequency (coarse)
- [ ] Optional: capture short bursts (ring buffer of timestamps) for deeper inspection

## Milestone E — UART-specific investigations (optional)

These tasks are only needed if GPIO-level tests prove the physical path is correct but UART still fails.

- [ ] Add a “UART loopback” mode (configure UART peripheral on chosen pins and echo bytes)
- [ ] Add explicit newline/echo behaviors to match `esp_console` expectations

## Milestone F — Validation playbooks + evidence capture

- [ ] Write “bring-up” playbook: build/flash/monitor steps for both boards
- [ ] Write “cable mapping” playbook: how to determine if GROVE is pin-to-pin and whether G1/G2 are swapped
- [ ] Collect evidence: scope screenshots, measured frequencies, observed edge counts
- [ ] Summarize root cause hypotheses and which are ruled in/out


