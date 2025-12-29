---
Title: Diary
Ticket: 0015-QEMU-REPL-INPUT
Status: active
Topics:
    - esp32s3
    - esp-idf
    - qemu
    - uart
    - console
    - serial
    - microquickjs
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T14:00:36.913079214-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Track the investigation of why QEMU + `idf_monitor` reaches the `js>` prompt but does not deliver interactive input to the REPL.

## Step 1: Spin off bug ticket and capture initial evidence

Closed the original Cardputer port ticket as blocked because we hit a QEMU/serial interaction issue that deserves a clean, focused debugging session. This step captures the most important evidence (boot transcript + key observation) so we can resume after sleep without re-deriving state.

### What I did

- Closed `0014-CARDPUTER-JS` as **blocked** and checked off the “QEMU boots” task.
- Created a new ticket `0015-QEMU-REPL-INPUT`.
- Captured the current tmux/idf_monitor transcript into `sources/mqjs-qemu-monitor.txt`.
- Wrote a bug report with repro steps, hypotheses, and next steps in `analysis/01-bug-report-...`.

### Why

- We got the firmware to boot in QEMU (banner + `js>`), but couldn’t reliably feed characters into the REPL.
- This is exactly the kind of “needs a fresh brain tomorrow” issue, and the fastest way to progress is to snapshot the evidence and isolate the problem.

### What worked

- QEMU boot reaches the REPL prompt (`js>`), so UART TX and monitor attachment work.
- `idf_monitor` hotkeys work (e.g. Ctrl+T Ctrl+H prints monitor help), so our tmux interaction does reach `idf_monitor`.

### What didn’t work

- Normal characters (e.g. `1+2<enter>`) do not echo or produce output in the firmware REPL, suggesting UART RX isn’t reaching `uart_read_bytes()` in `main.c`.

### What I learned

- The earlier SPIFFS “partition not found” issue was a separate prerequisite problem; once storage exists, we still hit the input/RX problem.
- Capturing a deterministic transcript is more valuable than “trying one more random fix” late in the day.

### What should be done next

- Attach to tmux and type manually to confirm whether the problem is specific to programmatic send-keys or applies to real typing.
- Bypass `idf_monitor` by using `imports/test_storage_repl.py` to send bytes directly to `localhost:5555`.

## Context

<!-- Provide background context needed to use this reference -->

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
