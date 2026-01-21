---
Title: Diary
Ticket: 0057-MLED-NODE-LED
Status: active
Topics:
    - esp32c6
    - esp-idf
    - animation
    - wifi
    - console
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0049-xiao-esp32c6-mled-node/main/mled_effect_led.c
      Note: Diary Step 3 mapping + integration
    - Path: 0049-xiao-esp32c6-mled-node/tools/mled_smoke.py
      Note: Diary Step 4 host pattern selector
ExternalSources: []
Summary: Work log for integrating WS281x LED patterns into the 0049 MLED node (protocol PatternConfig mapping + host smoke tests).
LastUpdated: 2026-01-21T15:37:51.361584652-05:00
WhatFor: Track implementation steps, commits, failures, and test results while wiring 0044's WS281x pattern engine into the 0049 MLED node.
WhenToUse: Use when continuing the work, reviewing decisions, or reproducing build/flash/host test procedures.
---


# Diary

## Goal

Capture the end-to-end work log for ticket `0057-MLED-NODE-LED`: implementing real WS281x LED pattern output in the `0049-xiao-esp32c6-mled-node` firmware by integrating `0044`’s LED engine and mapping protocol `PatternConfig` to on-device patterns, plus writing a host-side playbook to drive and verify patterns.

## Context

Baseline status before this ticket:
- `0049-xiao-esp32c6-mled-node` already implements MLED/1 networking and cue scheduling, but cue application is **log-only**.
- `0044-xiao-esp32c6-ws281x-patterns-console` already implements a robust WS281x driver + pattern engine + queue-owned `led_task`.

This ticket is the “bridge”: on `APPLY cue=...`, we translate the protocol’s fixed-size `PatternConfig` into `led_pattern_cfg_t` and push it into the `led_task` owner task.

## Step 1: Create ticket workspace + initial task list

Created the `docmgr` workspace and wrote a concrete checklist for the LED integration work (copy engine, implement mapping, wire cue apply, extend host tests, write playbook).

### What I did
- Created ticket `0057-MLED-NODE-LED`.
- Added documents:
  - `reference/01-diary.md` (this diary)
  - `design-doc/01-design-protocol-patternconfig-ws281x-patterns-0049-0044-integration.md`
  - `playbook/01-playbook-flash-0049-node-drive-led-cues-from-host.md`
- Replaced `tasks.md` with a real implementation checklist.

### Why
- This integration touches both firmware and host test tooling; a dedicated ticket keeps the scope clean and makes review/test sequencing explicit.

### What worked
- Ticket + docs created under `ttmp/2026/01/21/0057-MLED-NODE-LED--mled-node-ws281x-patterns-protocol-mapping/`.

### What didn't work
- N/A

### What should be done next
- Draft the design doc (mapping tables + integration approach) and then start landing code changes in small commits.

## Step 2: Write a concrete host test playbook (pre-implementation)

This step turns the previously empty playbook stub into a runnable, copy/paste sequence for flashing `0049` and driving cues from the host. Even before the LED integration lands, this is useful: it validates the protocol loop (PING/PONG discovery, BEACON sync, CUE_PREPARE ACKs, and cue scheduling) and establishes an “exit criteria” checklist for when real LED output is wired in.

### What I did
- Filled out `playbook/01-playbook-flash-0049-node-drive-led-cues-from-host.md` with:
  - build/flash/monitor steps
  - Wi‑Fi provisioning commands (`wifi scan/join/status`)
  - host commands for `tools/mled_ping.py` and `tools/mled_smoke.py`
  - explicit exit criteria for both “log-only” and “LED implemented” states
- Checked off the “write playbook” task in `tasks.md`.

### Why
- Having a repeatable host-driven procedure makes it much easier to validate the eventual `PatternConfig → led_pattern_cfg_t` mapping without guessing whether failures are network/protocol vs LED driver/pattern engine.

### What worked
- The existing host script `tools/mled_smoke.py` already exercises the key protocol flow and sends a `PatternConfig` with `pattern_type=1` (RAINBOW), which will be a good first end-to-end visual test once the mapping is implemented.

### What didn't work
- N/A

### What should be done next
- Land the actual firmware integration (port/copy the `0044` engine, implement mapping, and wire cue apply into the LED task).

## Step 3: Integrate WS281x engine into 0049 and apply patterns from protocol cues

This step turns the node’s “log-only APPLY” into real hardware output by embedding the proven `0044` WS281x engine (single-owner `led_task` + `led_patterns` + RMT driver) into the `0049-xiao-esp32c6-mled-node` firmware. The node registers an `on_apply` callback with the shared `mled_node` component; when a cue fires, we translate the protocol’s fixed-size `PatternConfig` into `led_pattern_cfg_t` and push it into the LED task queue with a non-blocking send.

**Commit (code):** b191c40 — "0049: drive WS281x patterns from MLED PatternConfig"

### What I did
- Ported/copied the required 0044 sources into `0049-xiao-esp32c6-mled-node/main/`:
  - `led_task.[ch]`, `led_patterns.[ch]`, `led_ws281x.[ch]`, `ws281x_encoder.[ch]`
- Added `mled_effect_led.[ch]` as the adapter that:
  - starts the LED task with Kconfig defaults (GPIO, LED count, timing, frame_ms)
  - maps `mled_pattern_config_t` to `led_pattern_cfg_t`
  - enqueues updates via `led_task_send(..., timeout_ms=0)` to keep the node task non-blocking
  - updates node-visible PONG fields via `mled_node_set_effect_status()`
- Wired the node callback:
  - `mled_node_set_on_apply(...)` in `app_main.c`

### Why
- Keeping WS281x driving in a single owner task avoids race conditions and makes timing predictable.
- Mapping is centralized in one small adapter module, keeping the node networking component generic and reusable.

### What worked
- `idf.py -C 0049-xiao-esp32c6-mled-node build` succeeded with the new LED engine sources linked in.

### What didn't work
- N/A (the main sharp edge was ensuring new sources were tracked; otherwise the build system can’t see them reliably).

### What I learned
- The “node callback → enqueue work” seam is the right place to integrate hardware effects without contaminating the protocol component.

### What was tricky to build
- The mapping has to be robust to invalid/odd values; for now we clamp the key fields (brightness, per-pattern ranges) to match the existing 0044 console semantics.

### What warrants a second pair of eyes
- Confirm that the protocol `data[12]` field packing for CHASE/BREATHING/SPARKLE matches the intended spec (byte layout and ranges).

### What should be done in the future
- If deterministic sparkle is needed, expose a “set RNG seed” control in the LED engine and honor protocol `seed`.

### Code review instructions
- Start at `0049-xiao-esp32c6-mled-node/main/mled_effect_led.c`.
- Follow the apply path:
  - node cue APPLY → callback → `led_task_send(LED_MSG_SET_PATTERN_CFG)`
- Build:
  - `source ~/esp/esp-idf-5.4.1/export.sh`
  - `idf.py -C 0049-xiao-esp32c6-mled-node build`

## Step 4: Extend host smoke tool to select patterns

This step extends the host-side smoke harness so you can drive each pattern type with explicit parameters (and confirm the device-side mapping visually). It keeps the protocol flow the same (discover → BEACON → TIME_RESP → PREPARE/ACK → FIRE) but adds `--pattern ...` and per-pattern argument flags.

**Commit (code):** b93231a — "tools: add multi-pattern support to mled_smoke.py"

### What I did
- Added `--pattern {off,rainbow,chase,breathing,sparkle}` plus per-pattern parameter flags.
- Implemented the `PatternConfig.data[12]` packing for each pattern.

### Why
- Once LEDs are real, the fastest validation is “single command per pattern” from the host.

### What worked
- `python3 -m py_compile tools/mled_smoke.py` succeeded after the change.

### What didn't work
- N/A

### What should be done next
- Flash and run the playbook to verify the patterns visually, then capture “what you saw” + logs into this diary.

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
