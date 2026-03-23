---
Title: WAMR Loader Debug Snapshots
Ticket: ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL
Status: active
Topics:
    - papers3
    - wasm
    - debugging
    - snapshots
DocType: note
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Tracked copies of ignored local WAMR loader source edits used during ESP-42."
LastUpdated: 2026-03-23T12:14:57-04:00
WhatFor: "Preserve ignored WAMR loader debug edits in tracked ticket history."
WhenToUse: "Use when reviewing or syncing the ESP-42 local WAMR loader instrumentation."
---

# WAMR Loader Debug Snapshots

These files are tracked copies of the local `managed_components/espressif__wasm-micro-runtime`
loader sources modified during `ESP-42`.

They exist because the main repo ignores `managed_components/`, so normal git history does not
preserve those edits directly.

Use `../check_wamr_loader_snapshot_sync.sh` to verify that the tracked snapshots still match the
live local WAMR sources being built by `0082`.
