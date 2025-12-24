---
Title: 'Diary'
Ticket: 0011-HOST-DEBUG-COMPILE
Status: active
Topics:
    - esp-idf
    - linux
    - host
    - testing
    - ui
    - cardputer
    - keyboard
    - display
    - m5gfx
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-24T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Track the step-by-step work to make **typewriter behavior iterate fast on Linux host** (no flash), while keeping the **device adapter** (Cardputer keyboard scan + M5GFX rendering) as a separate, later validation step.

## Step 1: Ticket orientation + establish a baseline loop

This step is about getting the workspace “ready to work”: make sure there’s a durable place to record progress, summarize what’s already known, and define the next concrete command to run. The goal is to avoid drifting into random experiments without a stable trail of what we tried and why.

At this point, the ticket has a strong research doc laying out two viable strategies (ESP-IDF linux target vs plain host build) but it doesn’t yet have actionable tasks, a diary, or a playbook that says “run this now to iterate”.

### What I did
- Created this diary document.
- Reviewed the existing research doc and extracted the concrete decision points and missing artifacts (tasks, links, API boundary, UX plan).

### Why
- Without an always-updated diary and an explicit “baseline loop”, the work tends to splinter into unrepeatable one-off experiments (especially for host builds).

### What worked
- The existing analysis doc already suggests a clean architecture: `typewriter_core` + two adapters (device + host).

### What didn't work
- N/A (no build executed yet in this step)

### What I learned
- The ticket workspace is scaffolded but missing the operational docs (diary/playbook) that make it easy to continue.

### What was tricky to build
- N/A (documentation scaffolding only)

### What warrants a second pair of eyes
- The eventual decision between “ESP-IDF linux target” vs “plain CMake host runner” should be reviewed for long-term maintainability (preview tooling vs parallel build system).

### What should be done in the future
- N/A

### Code review instructions
- Start here: `ttmp/.../0011.../analysis/01-research-esp-idf-host-linux-builds-for-fast-iteration-typewriter.md`
- Then review the task list: `ttmp/.../0011.../tasks.md`

### Technical details
- Next concrete step is to collect official ESP-IDF docs links for:
  - Linux target (“host apps”, preview)
  - Host/unit testing on Linux
  - QEMU workflow (optional)

### What I'd do differently next time
- When creating a ticket, always create `reference/01-diary.md` immediately so work starts with a trace.


