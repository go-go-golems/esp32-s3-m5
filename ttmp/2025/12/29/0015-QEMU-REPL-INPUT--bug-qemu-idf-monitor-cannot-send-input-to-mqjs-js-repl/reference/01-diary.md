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

## Step 2: Review Manus deliverables and reconcile “QEMU tested” vs evidence

This step was about reducing uncertainty: several Manus markdown deliverables in `imports/` claim a “working REPL” and “QEMU tested”, but our ticket’s observed bug is precisely “boot reaches `js>` but RX never arrives”. The most valuable outcome here is a crisp gap statement: **we have boot logs and a prompt, but we do not yet have a captured interactive transcript proving RX works**.

### What I did

- Read Manus deliverables in `esp32-s3-m5/imports/`:
  - `FINAL_DELIVERY_README.md`
  - `DELIVERY_SUMMARY.md`
  - `ESP32_MicroQuickJS_Playbook.md`
  - `MicroQuickJS_ESP32_Complete_Guide.md`
- Inspected Manus “golden” QEMU log: `imports/qemu_storage_repl.txt`.
- Confirmed the repo already contains a bypass tool: `imports/test_storage_repl.py` (raw TCP to port 5555).

### Why

- We need to know whether “QEMU tested” means “prints `js>`” (TX only) or “interactive eval works” (RX works).
- If Manus already had a working RX path, their exact commands/environment could save a lot of time.

### What worked

- We found all the relevant Manus artifacts and linked them into this ticket’s `index.md` as context.
- We found that `imports/qemu_storage_repl.txt` clearly demonstrates the boot-to-prompt path we also have in our own transcript.

### What didn’t work

- None of the Manus docs include a concrete “proof transcript” like:
  - `js> 1+2` → `3`
- The “golden log” still doesn’t show interactive input being echoed/evaluated (it ends at a prompt and normal logs).

### What I learned

- Treat “QEMU tested” as **TX validated** until we capture an interactive RX transcript ourselves.
- The included `test_storage_repl.py` is an important next step: it bypasses `idf_monitor` and can cleanly separate “monitor input path” issues from “QEMU UART RX” issues.

### What was tricky to build

- Avoiding confirmation bias: several docs describe a “working REPL”, but the only concrete captured evidence is still “boot prints `js>`”.

### What warrants a second pair of eyes

- Confirm whether `test_storage_repl.py` *actually* produces eval results against `localhost:5555` on this machine, and if so, capture the output as a new `sources/` artifact.

### What should be done in the future

- Add a “golden interactive transcript” artifact under `sources/` once we get *any* RX working (so this doesn’t regress silently).

### Code review instructions

- Start with `imports/qemu_storage_repl.txt` and compare it with `sources/mqjs-qemu-monitor.txt`.
- Read `imports/test_storage_repl.py` to understand the bypass path (raw TCP to 5555).

## Step 3: Expand hypothesis space (storage regression + minimal UART echo)

This step captures two hypotheses worth testing early because they can dramatically narrow the search space: (1) adding SPIFFS/autoload “storage stuff” might have changed behavior (timing/resource/stdio) in a way that breaks RX, and (2) a minimal UART echo firmware can prove whether QEMU UART RX works at all before we blame MicroQuickJS/REPL logic.

### What I did

- Added explicit tasks for:
  - “storage broke RX” A/B test (SPIFFS vs no SPIFFS)
  - minimal UART echo firmware
- Created a dedicated debugging plan playbook:
  - `playbooks/01-debugging-plan-qemu-repl-input.md`

### Why

- If a minimal echo firmware cannot receive bytes, we should stop digging into the JS REPL and focus on QEMU/serial wiring.
- If no-SPIFFS works but SPIFFS build fails, the regression surface becomes much smaller and more actionable.

### What I learned

- The current build system for `imports/esp32-mqjs-repl/mqjs-repl/main/` only compiles `main.c` (so “no-storage” likely means a deliberate variant, not an existing alternate entrypoint).

### What warrants a second pair of eyes

- Review the playbook’s decision tree: are we missing a crucial “obvious” test for the serial path (e.g. alternative terminal tools or a different UART instance mapping)?

## Context

<!-- Provide background context needed to use this reference -->

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
