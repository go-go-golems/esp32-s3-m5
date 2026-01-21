---
Title: 'Design: Protocol PatternConfig → WS281x Patterns (0049 + 0044 integration)'
Ticket: 0057-MLED-NODE-LED
Status: active
Topics:
    - esp32c6
    - esp-idf
    - animation
    - wifi
    - console
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_patterns.h
      Note: Internal pattern config structs/ranges (mapping target)
    - Path: 0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.c
      Note: Queue-owned LED owner task (desired integration target)
    - Path: 0049-xiao-esp32c6-mled-node/main/led_task.c
      Note: Single-owner LED task that receives pattern updates
    - Path: 0049-xiao-esp32c6-mled-node/main/mled_effect_led.c
      Note: |-
        Adapter: PatternConfig -> led_pattern_cfg_t + led_task_send + PONG status updates
        Wire mapping + enqueue into led_task + update node PONG status
    - Path: 0049-xiao-esp32c6-mled-node/tools/mled_smoke.py
      Note: |-
        Host-driven BEACON+PREPARE+FIRE smoke harness we extend to choose patterns
        Host smoke tool now supports selecting all patterns
    - Path: components/mled_node/include/mled_protocol.h
      Note: |-
        Protocol pattern_type IDs and PatternConfig layout
        Protocol PatternConfig layout and pattern_type IDs
    - Path: components/mled_node/src/mled_node.c
      Note: |-
        Node cue apply path (calls the registered on-apply callback)
        Node apply callback entrypoint
ExternalSources: []
Summary: Implement real WS281x LED output in the 0049 MLED node by integrating 0044's led_task engine and mapping protocol PatternConfig (type + data[12]) into led_pattern_cfg_t.
LastUpdated: 2026-01-21T15:37:51.627768515-05:00
WhatFor: Provide an implementable blueprint for mapping protocol cues to WS281x patterns and for wiring node cue scheduling into the existing queue-owned LED engine.
WhenToUse: Use when implementing the mapping, reviewing correctness (ranges, clamping, idempotency), or extending host tests to validate visual behavior.
---


# Design: Protocol PatternConfig → WS281x Patterns (0049 + 0044 integration)

## Executive Summary

We will make the `0049-xiao-esp32c6-mled-node` firmware actually drive WS281x LEDs by reusing `0044`’s existing pattern engine (`led_task`, `led_patterns`, `led_ws281x`, encoder). The node already receives `CUE_PREPARE` and schedules `CUE_FIRE`; on `APPLY cue=...` we will:

1) Decode `mled_pattern_config_t` (protocol wire struct) into `led_pattern_cfg_t` (0044 engine config).
2) Send a queue message (`led_task_send`) to update the active pattern and global brightness.
3) Keep `PONG` status fields consistent by reading from `led_task_get_status()` instead of the node’s own “log-only” fields.

The integration is intentionally console-free for “node stuff”: Wi‑Fi remains provisioned via `wifi ...` only. LED driver config (gpio/count/order) will initially be Kconfig defaults; later we can reintroduce a `led ...` console if needed.

## Problem Statement

Today, the MLED node behaves correctly on the network but has no physical output: it sets `current_pattern` in RAM and prints `APPLY` logs. We want the protocol’s `pattern_type` and parameter bytes (`data[12]`) to drive real WS281x patterns (rainbow/chase/breathing/sparkle) on an ESP32‑C6.

Hard constraints:
- We must avoid concurrency bugs: the WS281x driver and animation state should remain single-owner (task-owned).
- The mapping must be stable and deterministic, matching the protocol documents and host tooling.
- The mapping must be robust to bad inputs (range checks/clamping).

## Proposed Solution

### Architecture

Reuse the existing separation:
- **Node task** owns sockets + protocol state and decides “apply cue X now”.
- **LED task** owns WS281x hardware and pattern state, and receives messages via a FreeRTOS queue.

This matches the design philosophy in `0044` and preserves a single owner for hardware.

### Where mapping lives

Add a small adapter module (inside `0049-xiao-esp32c6-mled-node/main/`):

- `mled_effect_led.[ch]` (name bikeshed): implements:
  - `mled_effect_start()` → starts `led_task_start()` with Kconfig defaults
  - `mled_effect_apply_pattern(const mled_pattern_config_t*)` → translate + `led_task_send()`
  - `mled_effect_get_status(...)` → reads `led_task_get_status()`

The node’s `apply_cue()` calls `mled_effect_apply_pattern(&entry->pattern)` and uses effect status for PONG.

### Mapping tables (wire → engine)

Protocol `pattern_type` values already match `0044`’s `led_pattern_type_t` ordering:

| Protocol | Value | Engine |
|---|---:|---|
| OFF | 0 | `LED_PATTERN_OFF` |
| RAINBOW | 1 | `LED_PATTERN_RAINBOW` |
| CHASE | 2 | `LED_PATTERN_CHASE` |
| BREATHING | 3 | `LED_PATTERN_BREATHING` |
| SPARKLE | 4 | `LED_PATTERN_SPARKLE` |

Parameter mapping uses the “wire mapping tables” from the protocol design (see 0049 ticket design doc section 6.12); we will implement identical logic in C, clamping to the ranges enforced by `0044` console.

#### Rainbow (`data[0..2]`)

- `data[0]` → speed `0..20`
- `data[1]` → saturation `0..100`
- `data[2]` → spread_x10 `1..50`

#### Chase (`data[0..11]`)

- speed, tail_len, gap_len, trains
- fg RGB, bg RGB
- dir, fade_tail

#### Breathing (`data[0..6]`)

- speed `0..20`
- color RGB
- min/max bri `0..255`
- curve `0..2`

#### Sparkle (`data[0..9]`)

- speed `0..20`
- color RGB
- density `0..100`
- fade `1..255`
- mode `0..2`
- background RGB

### Global brightness

Protocol `brightness_pct` is used as the engine’s `global_brightness_pct` (clamped to `1..100`).

### Seeds (optional)

Protocol includes `seed` and a `seed_valid` flag. `0044` does not currently expose a “seed set” message, so initial integration ignores seed. If deterministic sparkle is required, we will add a message type to set the pattern RNG seed and implement it in `led_patterns`.

## Design Decisions

1) **Reuse 0044 engine instead of reimplementing patterns**
   - It already has correct time-based semantics, stable ranges, and a safe ownership model.
2) **Keep mapping in one function**
   - Centralizes the wire contract and avoids “partial mapping in many places”.
3) **Clamp inputs rather than reject**
   - More operator-friendly and resilient to small host bugs; still log warnings when clamping occurs.
4) **Keep LED config Kconfig-only initially**
   - Simpler for first integration; we can add a `led ...` console later if needed.

## Alternatives Considered

1) Implement WS281x patterns directly inside the node task
   - Rejected: violates single-owner hardware model, increases concurrency risk.
2) Parse the protocol into `led_msg_t` directly
   - Rejected: too coupled; better to map into `led_pattern_cfg_t` and use existing `LED_MSG_SET_PATTERN_CFG` semantics.

## Implementation Plan

1) Copy required `0044` sources into `0049-xiao-esp32c6-mled-node/main/`:
   - `led_task.[ch]`, `led_patterns.[ch]`, `led_ws281x.[ch]`, `ws281x_encoder.c`
2) Add `mled_effect_led.[ch]` wrapper and start the LED task during boot.
3) Implement the mapping function (`mled_pattern_config_t` → `led_pattern_cfg_t`).
4) Change node `apply_cue()` to call effect apply rather than only updating local fields.
5) Update `PONG` status fields to reflect LED task status.
6) Extend host test script to choose pattern type and parameters, and update playbook with “what you should see”.
7) Build + flash + run `mled_smoke.py` to validate all patterns.

## Open Questions

1) Which GPIO/LED count should be the default for XIAO ESP32C6 in this repo? (0043 uses D1 wiring; we should match that.)
2) Do we want a `led` console in this firmware, or keep LED config strictly compile-time?
3) Should we implement dedup of repeated `CUE_FIRE` before integrating visuals (to avoid multiple redundant applies)?

## References

- 0049 protocol design: `ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/design-doc/01-esp32-c6-node-firmware-as-a-network-node-protocol-design.md`
- 0044 LED engine design: `ttmp/2026/01/20/MO-032-ESP32C6-LED-PATTERNS-CONSOLE--esp32-c6-led-patterns-console/design-doc/01-led-pattern-engine-esp-console-repl-ws281x.md`
