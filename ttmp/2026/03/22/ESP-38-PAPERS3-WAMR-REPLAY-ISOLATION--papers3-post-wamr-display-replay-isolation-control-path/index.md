---
Title: PaperS3 post-WAMR display replay isolation control path
Ticket: ESP-38-PAPERS3-WAMR-REPLAY-ISOLATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - display
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp
      Note: Console surface where the control-path command will be added
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp
      Note: Existing host command queue and replay implementation under test
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp
      Note: Current WAMR-backed execution path for comparison
    - Path: 0079-papers3-wamr-assemblyscript-console/wasm-src/hello-frame/assembly/index.ts
      Note: Guest program whose behavior will be mirrored by the control path
ExternalSources: []
Summary: Follow-up investigation to isolate whether the PaperS3 replay crash is caused by WAMR state or by the queued display replay path itself.
LastUpdated: 2026-03-22T19:42:00-04:00
WhatFor: Plan and track the control-path experiment that replays the hello-frame drawing sequence without invoking WAMR.
WhenToUse: Read this before changing the replay-isolation experiment, validating hardware results, or deciding whether WAMR integration is still the main suspect.
---

# PaperS3 post-WAMR display replay isolation control path

## Overview

This ticket isolates the current PaperS3 crash by removing WAMR from the experiment. The existing `ESP-37` ticket proved that the failure has moved from WAMR-side import callbacks to queued host-command replay. The next step is to run the same replay path from a non-Wasm control command so we can determine whether the display path crashes on its own.

The narrow goal is to answer one question decisively:

- if `hello-frame`-equivalent replay crashes without WAMR, the bug is in the replay/display path
- if it only crashes after a WAMR run, WAMR or its teardown path is leaving the system in a bad state

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- **Implementation Plan**: See `design/01-replay-isolation-implementation-plan.md`
- **Postmortem Report**: See `design/02-wamr-replay-isolation-postmortem-report.md`
- **Project Report**: See `design/03-project-report-and-field-guide.md`
- **Diary**: See `reference/01-diary.md`

## Status

Current status: **active**

Current outcome: the ticket now contains the control-path implementation record, a detailed postmortem, and a long-form project report that preserves the full architecture and debugging story.

Immediate next milestone: decide whether to spend one bounded experiment on the official Espressif WAMR component or to pause runtime work and restructure the demo around a safer execution model.

## Topics

- papers3
- wasm
- firmware
- esp-idf
- display

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
