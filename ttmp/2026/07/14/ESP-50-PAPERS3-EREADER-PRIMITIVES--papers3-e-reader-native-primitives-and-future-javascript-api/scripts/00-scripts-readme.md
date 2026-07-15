---
Title: PaperS3 and Printalyzer Experiment Scripts
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
Summary: "Reproducible script inventory and rerun instructions for PaperS3/Printalyzer evidence tools."
LastUpdated: 2026-07-15T01:15:00Z
WhatFor: "Locate and safely rerun evidence calculations and guarded experiment tooling."
WhenToUse: "Before rerunning a ticket script or interpreting generated outputs."
---

# PaperS3 / Printalyzer experiment scripts

These scripts are evidence tools, not general-purpose panel drivers. Each physical action has a preregistered experiment directory and requires an explicit execute confirmation.

## Serial capture and raw-density reproduction

- `29-capture-synchronized-serial.py` — captures Printalyzer CDC and optionally PaperS3 serial on a common host monotonic timeline. Firmware serial is opened read-only with no modem-control ioctl. `--dens-raw-stream` is deliberately gated and performs remote-mode cleanup.
- `30-test-synchronized-serial.py` — pseudo-terminal integration test for raw stream command order, cleanup, and host density derivation.
- `32-analyze-printalyzer-static-captures.py` — replays static JSONL captures and prints placement/repeatability statistics plus mean deltas.
- `33-analyze-f0-dynamic-density.py` — converts exact F0 JSONL + host markers into CSV, JSON bins/candidates, Markdown analysis, and SVG.
- `34-build-f0-density-dashboard.py` — builds the self-contained retro monochrome HTML/JS F0 dashboard from the deterministic JSON outputs.

## Reproduce the current F0 dashboard

Run from the ticket root:

```bash
scripts/32-analyze-printalyzer-static-captures.py \
  --output-json scripts/output/32-printalyzer-static-calculations.json \
  --output-markdown scripts/output/32-printalyzer-static-calculations.md
scripts/33-analyze-f0-dynamic-density.py
scripts/34-build-f0-density-dashboard.py
```

Open:

```text
scripts/experiments/EXP-20260715-008-factory-f0-dynamic-density/dashboard.html
```

The dashboard is self-contained: it makes no network requests and embeds its data. It distinguishes host flash markers, candidate density activity, and derived density. Candidate activity is not an asserted semantic display phase.

## Exact F0 dynamic runner

`31-run-synchronized-f0-density.sh` starts a fixed 45-second Printalyzer stream, waits for it to become active, writes host markers, flashes the exact F0 release, retains the full flash log, and verifies cleanup evidence. It is intentionally single-use for experiment 008: output paths already exist and the script refuses replacement.

```bash
scripts/31-run-synchronized-f0-density.sh --check
# Physical action; only after a fresh experiment and explicit authorization:
scripts/31-run-synchronized-f0-density.sh --execute --confirm RUN-DENS-F0
```

Do not use the F0 runner to replay a completed experiment. Create a new experiment ID and update the runner/output locations first.

## Prepared F1 source-proxy runner

`35-run-synchronized-f1-density.sh` is the F1-specific counterpart for experiment 009. It checks the audited F1 trace-off artifact and refuses execution unless its own ledger exists and the literal F1 confirmation is supplied. It is prepared but has **not** been executed. F1 also requires the fixed-camera gate because the density stream cannot score spatial artifacts.

```bash
scripts/35-run-synchronized-f1-density.sh --check
# Physical action, only after camera recording is confirmed:
scripts/35-run-synchronized-f1-density.sh --execute --confirm RUN-DENS-F1
```
