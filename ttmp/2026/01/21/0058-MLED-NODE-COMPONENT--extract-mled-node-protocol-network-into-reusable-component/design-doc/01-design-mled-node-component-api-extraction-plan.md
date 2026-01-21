---
Title: 'Design: mled_node component API + extraction plan'
Ticket: 0058-MLED-NODE-COMPONENT
Status: active
Topics:
    - esp32c6
    - esp-idf
    - networking
    - protocol
    - tooling
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0049-xiao-esp32c6-mled-node/main/mled_node.c
      Note: Current node implementation to extract
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/echo_gif/CMakeLists.txt
      Note: Example component layout and dependency declaration style
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0049-xiao-esp32c6-mled-node/main/CMakeLists.txt
      Note: Main component CMake where we will switch to PRIV_REQUIRES mled_node
ExternalSources: []
Summary: "Define a reusable ESP-IDF component (`components/mled_node`) that provides MLED/1 wire structs, wrap-safe show clock helpers, and a UDP multicast node loop with callbacks for cue application and status reporting."
LastUpdated: 2026-01-21T15:40:49.670939422-05:00
WhatFor: "Provide an implementable plan and API contract for extracting the node protocol stack into a shared component."
WhenToUse: "Use when performing the extraction, updating downstream firmwares, or reviewing the API boundaries for effect integration."
---

# Design: mled_node component API + extraction plan

## Executive Summary

Extract the `0049-xiao-esp32c6-mled-node` protocol/network code into a reusable component `esp32-s3-m5/components/mled_node`. The component provides:

- protocol wire structs + pack/unpack helpers (`mled_protocol`)
- time helpers for `u32` show clocks (`mled_time`)
- a node runtime that joins multicast, handles BEACON/PING/CUE/TIME messages, and schedules fires (`mled_node`)

To remain reusable, the component exposes a small callback API so a firmware can plug in its own “effect engine” (LED patterns, display, etc) without copying the UDP protocol stack.

## Problem Statement

Today, every firmware that wants to be an MLED/1 node would need to copy/paste:
- multicast socket setup and membership,
- strict wire parsing,
- epoch gating logic,
- show-time offset logic,
- cue store and due scheduling.

Copy/paste makes it harder to:
- keep behavior consistent across firmwares,
- fix protocol bugs in one place,
- share host-side tooling expectations.

## Proposed Solution

### Component layout

```
components/mled_node/
  include/
    mled_node.h
    mled_protocol.h
    mled_time.h
  src/
    mled_node.c
    mled_protocol.c
    mled_time.c
  CMakeLists.txt
  Kconfig
```

### API surface (minimal, reusable)

```c
typedef void (*mled_node_on_apply_fn)(uint32_t cue_id, const mled_pattern_config_t *pattern, void *ctx);

typedef struct {
    uint8_t pattern_type;      // protocol pattern_type
    uint8_t brightness_pct;    // 1..100
    uint16_t frame_ms;         // optional; default 20
} mled_node_effect_status_t;

void mled_node_set_on_apply(mled_node_on_apply_fn fn, void *ctx);
void mled_node_set_effect_status(const mled_node_effect_status_t *st);

void mled_node_start(void);
void mled_node_stop(void);
uint32_t mled_node_id(void);
```

Rules:
- `mled_node_on_apply_fn` must be non-blocking; typically it enqueues work to an owner task (e.g., WS281x `led_task`).
- `mled_node_set_effect_status()` allows the app to override PONG fields like `frame_ms` or brightness if its effect engine differs from the protocol’s brightness semantics.

### Kconfig naming

Component-level Kconfig uses a unique prefix:
- `MLED_NODE_*` (not `MLEDNODE_*`) to avoid per-project naming and allow shared defaults.

Example options:
- `MLED_NODE_NAME` (string, default `c6-node`)
- `MLED_NODE_TIME_REQ_ENABLE` (bool, default y)

### Integration with 0049 tutorial

- Remove `mled_node.[ch]`, `mled_protocol.[ch]`, `mled_time.[ch]` from `0049-xiao-esp32c6-mled-node/main/`.
- Add `PRIV_REQUIRES mled_node` to `0049.../main/CMakeLists.txt`.
- Keep the user-visible behavior unchanged (still starts node on got-IP, still responds to host scripts).

## Design Decisions

1) **Callback-based effect integration**: keeps the node runtime reusable across different output modalities.
2) **Component-owned PING/PONG**: keeps discovery semantics consistent; apps only fill the “effect status” inputs.
3) **Kconfig in component**: avoids duplicating config across every firmware that consumes the component.

## Alternatives Considered

1) “Just export protocol pack/unpack”
   - Rejected: the hard part is the node loop (epoch gating + scheduling + multicast membership), not just structs.
2) “Make a generic transport layer and keep node logic in apps”
   - Rejected: would still duplicate most behavioral code; harder to validate.

## Implementation Plan

1) Create `components/mled_node` skeleton (CMake + Kconfig + headers).
2) Copy sources from `0049.../main/` into the component and adjust includes/Kconfig names.
3) Update `0049` to consume the component and delete the duplicated sources.
4) Build `idf.py -C 0049... build`.
5) Flash and verify `mled_ping.py` and `mled_smoke.py` still work.

## Open Questions

1) Should socket lifecycle be tied to got-IP/lost-IP events inside the component (requires event integration), or should the app manage start/stop as it does today?
2) Should the component expose cue-store introspection for debugging (counts, last epoch), or keep the API minimal for now?

## References

- Protocol design: `ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/design-doc/01-esp32-c6-node-firmware-as-a-network-node-protocol-design.md`
- Host smoke scripts: `esp32-s3-m5/0049-xiao-esp32c6-mled-node/tools/mled_ping.py` and `.../tools/mled_smoke.py`
