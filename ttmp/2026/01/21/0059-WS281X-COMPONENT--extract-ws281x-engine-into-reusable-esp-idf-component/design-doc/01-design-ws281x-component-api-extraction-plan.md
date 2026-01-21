---
Title: 'Design: ws281x component API + extraction plan'
Ticket: 0059-WS281X-COMPONENT
Status: active
Topics:
    - esp32c6
    - esp-idf
    - leds
    - rmt
    - component
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T16:46:38.831584954-05:00
WhatFor: ""
WhenToUse: ""
---

# Design: ws281x component API + extraction plan

## Executive Summary

Extract the WS281x engine used by `0049-xiao-esp32c6-mled-node` into a reusable ESP-IDF component `components/ws281x`. The component will provide the same C API as today (`led_task_start`, `led_task_send`, `led_task_get_status`, plus the underlying pattern/driver headers), and `0049` will consume it via `EXTRA_COMPONENT_DIRS` (scoped narrowly to avoid component fan-out), removing local copies of the sources.

## Problem Statement

We currently have at least one firmware (`0049`) that embeds a copy of the WS281x engine sources (driver + encoder + pattern renderer + owner task). Copy/paste modules tend to diverge across firmwares, making bug fixes and behavior changes hard to propagate. We want a single shared implementation that multiple projects can depend on, without contaminating the node protocol component (`components/mled_node`) with LED specifics.

## Proposed Solution

### Component name and scope

- **Component directory:** `esp32-s3-m5/components/ws281x/`
- **Component name:** `ws281x` (derived from directory name; used in `PRIV_REQUIRES ws281x`)
- **Scope:** only the WS281x engine:
  - `led_ws281x.*` + `ws281x_encoder.*` (RMT TX driver + WS281x encoder)
  - `led_patterns.*` (pattern implementations)
  - `led_task.*` (single-owner task + queue + status snapshot)

The component will *not* include any MLED protocol code or Wi‑Fi logic.

### Public API (unchanged)

The public interface remains the existing headers (to minimize downstream changes):
- `led_task.h` (primary control surface)
- `led_patterns.h` (pattern config types)
- `led_ws281x.h` + `ws281x_encoder.h` (driver/encoder types)

### Consumer responsibility

Consumer firmware remains responsible for:
- choosing GPIO/LED count/timing/frame rate (via Kconfig or runtime UI)
- mapping higher-level protocol/config (like MLED `PatternConfig`) into `led_pattern_cfg_t`

For `0049`, that mapping remains in `main/mled_effect_led.c`, which calls `led_task_send(LED_MSG_SET_PATTERN_CFG, ...)`.

## Design Decisions

1) **Keep the existing header names and API**
   - Minimizes churn; downstream code continues to `#include "led_task.h"` etc.
2) **No component-level Kconfig required**
   - The engine is runtime-configured by its `*_cfg_t` structs; Kconfig belongs in the consuming firmware.
3) **Narrow `EXTRA_COMPONENT_DIRS` per project**
   - Prevent accidental discovery of unrelated components under `esp32-s3-m5/components/`.

## Alternatives Considered

1) **Put WS281x code inside `components/mled_node`**
   - Rejected: node protocol should remain hardware-agnostic; LED engines are optional.
2) **Create multiple micro-components (`ws281x_driver`, `ws281x_patterns`, `ws281x_task`)**
   - Rejected for now: more CMake surface area with little benefit at this stage.
3) **Leave code duplicated in each firmware**
   - Rejected: increases maintenance cost and risks behavior drift.

## Implementation Plan

1) Create `components/ws281x/` skeleton:
   - `CMakeLists.txt` with `idf_component_register(SRCS ... INCLUDE_DIRS include PRIV_REQUIRES esp_driver_rmt esp_timer log)`
   - `include/` and `src/` folders
2) Move the WS281x engine sources from `0049-xiao-esp32c6-mled-node/main/` into the component:
   - headers → `components/ws281x/include/`
   - sources → `components/ws281x/src/`
3) Update `0049`:
   - add `${CMAKE_CURRENT_LIST_DIR}/../components/ws281x` to `EXTRA_COMPONENT_DIRS`
   - add `ws281x` to `main/CMakeLists.txt` `PRIV_REQUIRES`
   - delete the local WS281x engine copies from `main/`
4) Validate:
   - `idf.py -C 0049-xiao-esp32c6-mled-node build`
   - flash + host pattern smoke test (`tools/mled_smoke.py --pattern ...`)
5) Update ticket docs (tasks/diary/changelog) with exact commands and commit hashes.

## Open Questions

1) Should we also migrate `0044` (and `0046`) to the shared component immediately, or keep the initial scope to `0049` only?
2) Do we want a future “seed control” API extension (to honor protocol `seed`) to live in this component?

## References

- Engine origin: `esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/`
- Current consumer: `esp32-s3-m5/0049-xiao-esp32c6-mled-node/main/`
- Node protocol component extraction precedent: `esp32-s3-m5/components/mled_node/`
