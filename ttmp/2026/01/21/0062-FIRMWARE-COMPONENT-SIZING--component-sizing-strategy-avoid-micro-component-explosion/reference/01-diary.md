---
Title: Diary
Ticket: 0062-FIRMWARE-COMPONENT-SIZING
Status: active
Topics:
    - esp-idf
    - architecture
    - documentation
    - firmware
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T19:57:13.95103636-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Keep a step-by-step record of decisions and follow-up changes made while defining the repo-wide component sizing strategy (fat components, thin APIs) and aligning extraction tickets.

## Context

- This ticket exists because we started extracting reusable pieces (`mqjs_vm`, `mqjs_service`, `httpd_ws_hub`, etc.) and the next natural question is “what’s next?”.
- Without a sizing strategy, we risk turning every `.cpp` file into its own component, which is expensive in ESP-IDF.

## Step 1: Write sizing strategy + update tickets

This step writes a design document that defines when to create a new component vs keep code as an internal module, establishes dependency direction rules (base → feature → integration → firmware), and proposes a small set of next components (`encoder_service`, `webui_server`, `mqjs_integrations`) rather than many micro-components.

It also updates the existing extraction plan for `encoder_service` to explicitly avoid direct web/JS dependencies inside the component (use sink callbacks, with per-firmware sinks), and creates two new tickets to track the remaining “fat components”: `0063-WEBUI-SERVER-COMPONENT` and `0064-MQJS-INTEGRATIONS-COMPONENT`.

### What I did
- Wrote sizing/design rules in:
  - `ttmp/2026/01/21/0062-FIRMWARE-COMPONENT-SIZING--component-sizing-strategy-avoid-micro-component-explosion/design-doc/01-component-sizing-dependency-rules.md`
- Created the follow-up extraction tickets:
  - `0063-WEBUI-SERVER-COMPONENT`
  - `0064-MQJS-INTEGRATIONS-COMPONENT`
- Updated `0056-ENCODER-SERVICE-COMPONENT` tasks to keep `encoder_service` decoupled (sinks live in firmware/integration code).

### Why
- Component boundaries are expensive in ESP-IDF; we want reuse without fragility.
- The “fat component” approach keeps stable invariants in components and product-specific wiring in integration layers.

### What worked
- `docmgr` ticket/task plumbing captured the new plan (and tasks are now aligned with the strategy).

### What didn't work
- N/A (documentation/planning step).

### What I learned
- A clear dependency direction rule eliminates most circular dependency pressure.

### What was tricky to build
- N/A.

### What warrants a second pair of eyes
- Sanity-check the proposed component boundaries before implementation starts, to avoid redoing APIs after code lands.

### What should be done in the future
- Start implementing `encoder_service` under `0056`, then `webui_server` and `mqjs_integrations` under `0063`/`0064`.

### Code review instructions
- Review the strategy doc first:
  - `ttmp/2026/01/21/0062-FIRMWARE-COMPONENT-SIZING--component-sizing-strategy-avoid-micro-component-explosion/design-doc/01-component-sizing-dependency-rules.md`
- Then review the updated task breakdown:
  - `ttmp/2026/01/21/0056-ENCODER-SERVICE-COMPONENT--reusable-encoder-service-driver-interface-coalescing-sinks/tasks.md`
  - `ttmp/2026/01/21/0063-WEBUI-SERVER-COMPONENT--reusable-webui-server-esp-http-server-assets-ws-hub/tasks.md`
  - `ttmp/2026/01/21/0064-MQJS-INTEGRATIONS-COMPONENT--reusable-mqjs-integrations-console-command-http-eval-routes/tasks.md`

### Technical details
- Dependency direction: `base` → `feature` → `integration` → `firmware`.
- “Sinks not deps”: data producers (encoder) should emit events to callbacks; consumers (WS/JS) live at integration boundaries.

## Quick Reference

N/A (this is an implementation diary; the design doc contains the copy/paste-ready API sketches).

## Usage Examples

N/A.

## Related

- `ttmp/2026/01/21/0062-FIRMWARE-COMPONENT-SIZING--component-sizing-strategy-avoid-micro-component-explosion/design-doc/01-component-sizing-dependency-rules.md`
