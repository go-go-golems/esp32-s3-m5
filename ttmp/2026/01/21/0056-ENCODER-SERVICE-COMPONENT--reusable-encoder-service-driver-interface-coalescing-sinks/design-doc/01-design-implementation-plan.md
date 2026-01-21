---
Title: Design + implementation plan
Ticket: 0056-ENCODER-SERVICE-COMPONENT
Status: active
Topics:
    - esp-idf
    - cardputer
    - uart
    - gpio
    - websocket
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/chain_encoder_uart.cpp
      Note: Current driver implementation
    - Path: 0048-cardputer-js-web/main/encoder_telemetry.cpp
      Note: Current mixed driver+policy+sinks implementation
    - Path: 0048-cardputer-js-web/main/http_server.h
      Note: WS sink integration
    - Path: 0048-cardputer-js-web/main/js_service.h
      Note: JS sink integration
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:36:22.664359804-05:00
WhatFor: ""
WhenToUse: ""
---


# Design + implementation plan

## Executive Summary

Extract a reusable “encoder service” component that separates:

- the hardware driver (how we read delta/click),
- the policy (coalesce deltas, publish events),
- the sinks (send to WebSocket, enqueue to JS callbacks, etc.).

Primary consumer is `0048-cardputer-js-web`, whose current `encoder_telemetry.cpp` mixes all three.

## Problem Statement

`esp32-s3-m5/0048-cardputer-js-web/main/encoder_telemetry.cpp` currently:

- instantiates a specific driver (`ChainEncoderUart`)
- performs coalescing and position tracking
- decides when to broadcast WS frames and in what schema
- forwards click/delta to the JS service

This makes reuse hard:

- “encoder driver” and “telemetry policy” cannot be reused independently.
- adding a new encoder backend (GPIO rotary, different UART protocol) requires copying the whole file.

## Proposed Solution

Create:

- `esp32-s3-m5/components/encoder_service/`
  - `include/encoder_service.h`
  - `encoder_service.cpp`
  - depends on FreeRTOS

Define a driver vtable (C-friendly) and a service loop:

```c
typedef struct {
  bool (*init)(void* drv);
  int32_t (*take_delta)(void* drv);      // returns accumulated delta since last call
  int (*take_click_kind)(void* drv);     // returns -1 if none; otherwise click kind
} encoder_driver_vtable_t;

typedef struct {
  void* drv;
  encoder_driver_vtable_t vtable;

  uint32_t poll_ms;
  uint32_t ws_broadcast_ms;

  // sinks (optional):
  void (*on_delta)(int32_t pos, int32_t delta, uint32_t ts_ms, void* ctx);
  void* on_delta_ctx;
  void (*on_click)(int kind, uint32_t ts_ms, void* ctx);
  void* on_click_ctx;
} encoder_service_config_t;

esp_err_t encoder_service_start(const encoder_service_config_t* cfg);
```

Pseudocode loop:

```text
pos = 0; last_broadcast_pos = 0; seq=0
every poll_ms:
  d = take_delta()
  if d != 0:
    pos += d
    if on_delta: on_delta(pos, d, now_ms)
  k = take_click_kind()
  if k >= 0:
    if on_click: on_click(k, now_ms)
  if pos != last_broadcast_pos and time_for_broadcast:
    last_broadcast_pos = pos
    (sink decides schema; encoder_service stays neutral)
```

## Design Decisions

1) Keep the service neutral about JSON schemas and WS specifics.
   - The service calls sink callbacks; the firmware decides transport.

2) Keep driver interface minimal (delta + click).
   - It matches existing Chain Encoder semantics and is easy to implement for other backends.

## Alternatives Considered

1) Keep encoder telemetry per firmware.
   - Rejected: this is core “hardware → events” glue that repeats across UI/WS tickets.

## Implementation Plan

1) Implement `encoder_service` component and driver interface.
2) Migrate 0048:
   - Turn `ChainEncoderUart` into a driver implementation (keep class, add vtable wrappers).
   - Replace the task loop in:
     - `esp32-s3-m5/0048-cardputer-js-web/main/encoder_telemetry.cpp`
     with:
     - `encoder_service_start(...)` configured with sinks:
       - WS sink uses `http_server_ws_broadcast_text(...)`
       - JS sink uses `js_service_update_encoder_delta(...)` and `js_service_post_encoder_click(...)`

3) Validate on hardware:
   - delta updates still coalesce and only broadcast on change,
   - click events still reach JS callbacks and WS.

## Open Questions

1) Do we need “press-and-hold” or debouncing semantics in the service?
   - For now: no; driver may implement if needed, and sinks can layer semantics.

## References

- Current implementation to split:
  - `esp32-s3-m5/0048-cardputer-js-web/main/encoder_telemetry.cpp`
- Driver used today:
  - `esp32-s3-m5/0048-cardputer-js-web/main/chain_encoder_uart.cpp`
