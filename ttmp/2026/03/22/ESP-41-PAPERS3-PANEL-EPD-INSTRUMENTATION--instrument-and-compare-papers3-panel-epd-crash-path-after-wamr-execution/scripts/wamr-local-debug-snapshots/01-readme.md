---
Title: WAMR Local Debug Snapshots
Ticket: ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Tracked copies of ignored local WAMR source files used for the current PaperS3 debug slice.
LastUpdated: 2026-03-23T11:45:00-04:00
WhatFor: Preserve ignored managed-component source edits in a tracked ticket location.
WhenToUse: Open this note before reviewing or reconstructing the current local WAMR debug state.
---

# WAMR Local Debug Snapshots

These files are tracked copies of the locally modified WAMR sources that the `0079` build is currently compiling from `managed_components/`.

They exist because `managed_components/` is ignored in the main repo, so changes under:

- `0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/...`

do not appear in normal git status or commits.

Current captured files:

- `wasm_runtime_common.c`
- `wasm_runtime.c`
- `wasm_memory.c`

Use these snapshots to review or reconstruct the exact local debug state before the next hardware probe. They are not pristine upstream files; they are copies of the current local working versions.
