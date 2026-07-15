---
Title: "Preregistration - EXP-20260715-012-factory-f2-manual-reset-density"
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - hardware-qualification
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Frozen manual-reset F2 serial ownership, density, ring, and alignment procedure."
LastUpdated: 2026-07-15T02:25:00Z
WhatFor: "Capture F2 ring and density without violating single-owner serial policy."
WhenToUse: "Use after the automatic F2 attempt stopped safely before flash."
---

# Preregistration

## Why manual reset

The automatic F2 attempt in experiment 011 stopped before flash because esptool requires exclusive USB Serial/JTAG ownership while pre-boot firmware capture also needs the same port. This protocol separates the phases: no-reset flash releases the port, then a safe read-only capture is armed, then the operator presses Reset exactly once.

## Fixed sequence

1. Stage exact F2 with `--after no_reset`; do not boot it yet.
2. Arm the 75-second combined density/read-only-firmware capture.
3. On explicit `capture_armed=yes`, operator presses PaperS3 Reset exactly once.
4. Do not move Printalyzer/table/cables or send PaperS3 input.
5. Wait for capture completion, then extract/validate ring.

## Decision rule

Require valid density capture and cleanup; ring dump begin/end; contiguous valid ring records; power/frame/idle events; and one reported reset. Host/device timing uses an approximate final-idle/dump-begin anchor only.
