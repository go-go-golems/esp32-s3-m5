---
Title: 'Design: protobuf event payloads + websocket bridge for 0030'
Ticket: 0030-CARDPUTER-CONSOLE-EVENTBUS
Status: complete
Topics:
    - cardputer
    - keyboard
    - console
    - esp-idf
    - esp32s3
    - display
    - esp-event
    - ui
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0029-mock-zigbee-http-hub/main/hub_http.c
      Note: Reference for esp_http_server + WebSocket plumbing patterns.
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Defines the current bus IDs and fixed-size payload structs that the protobuf schema will mirror.
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T08:40:38.530213909-05:00
WhatFor: ""
WhenToUse: ""
---


# Design: protobuf event payloads + WebSocket bridge for 0030

## Executive Summary

This design introduces a `.proto` schema for the `0030-cardputer-console-eventbus` event bus and uses standard protobuf tooling to serialize events for forwarding (e.g. over WebSocket), while keeping the internal `esp_event` bus fast and POD-friendly.

Recommended path:

- Keep internal bus payloads as **fixed-size C structs** (as they are today).
- Add a **bridge** that subscribes to bus events and emits a **protobuf envelope** over WebSocket (binary frames).
- Generate TypeScript types/decoders from the same `.proto` so web clients share the schema.

This uses **`protobuf-c`** on the device (ESP-IDF already ships a `protobuf-c` component).

## Problem Statement

`0030` defines event IDs and payloads directly in C++:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp`

That is great for embedded correctness and performance, but it becomes painful as soon as we want external tooling:

- A WebSocket/SSE client needs the same payload types in TypeScript.
- Without an IDL, we risk drift between embedded structs and host/UI definitions.
- Each new event requires hand-written bridge code and hand-written TS types.

We want a **single source of truth** for event payloads that supports:

- embedded producers/consumers
- WebSocket clients
- future tooling (schema/version checks, dumps, replay)

## Proposed Solution

### 1) Define a bus schema in protobuf

Add a `.proto` file for the 0030 bus, for example:

- `0030-cardputer-console-eventbus/main/idl/demo_bus.proto` (new)

Schema shape:

- `enum EventId` with stable numeric IDs.
- `message` per payload type.
- `message Event` envelope containing `EventId id` + `oneof payload` (+ optional `schema_version`).

Sketch:

```proto
syntax = "proto3";
package demo.bus.v1;

enum EventId {
  EVENT_ID_UNSPECIFIED = 0;
  EVENT_ID_KB_KEY = 1;
  EVENT_ID_KB_ACTION = 2;
  EVENT_ID_CONSOLE_POST = 3;
  EVENT_ID_HEARTBEAT = 4;
  EVENT_ID_RAND = 5;
}

message KbKey { int64 ts_us = 1; uint32 keynum = 2; uint32 modifiers = 3; }
message Heartbeat { int64 ts_us = 1; uint32 heap_free = 2; uint32 dma_free = 3; }
message ConsolePost { int64 ts_us = 1; string msg = 2; }

message Event {
  uint32 schema_version = 1; // e.g. 1
  EventId id = 2;
  oneof payload {
    KbKey kb_key = 10;
    Heartbeat heartbeat = 11;
    ConsolePost console_post = 12;
  }
}
```

### 2) Use `protobuf-c` to encode the envelope on-device

Targeted usage:

- Embedded runtime: ESP-IDF `protobuf-c` component
- Codegen: `protoc` + `protoc-gen-c` (protobuf-c compiler plugin)

Encoding (bridge side) looks like:

```c
Demo__Bus__V1__Event ev = DEMO__BUS__V1__EVENT__INIT;
ev.schema_version = 1;
ev.id = DEMO__BUS__V1__EVENT_ID__EVENT_ID_HEARTBEAT;

Demo__Bus__V1__Heartbeat hb = DEMO__BUS__V1__HEARTBEAT__INIT;
hb.ts_us = ts_us;
hb.heap_free = heap;
hb.dma_free = dma;
ev.heartbeat = &hb;

size_t n = demo__bus__v1__event__get_packed_size(&ev);
demo__bus__v1__event__pack(&ev, buf);
```

### 3) Bridge bus events to WebSocket binary frames

Add a module that:

- registers an `esp_event` handler for `CARDPUTER_BUS_EVENT, ESP_EVENT_ANY_ID`
- translates `BUS_EVT_*` payload structs into protobuf payload messages
- packs the envelope into a bounded buffer
- sends binary frames to all connected WS clients (or enqueues to a WS sender task)

This keeps internal bus payloads POD/fixed-size, which is ideal for `esp_event_post_to()`.

### 4) Generate TypeScript types/decoders from the same `.proto`

On the web side, decode WebSocket binary frames using a TS protobuf runtime.

Two viable approaches:

- `protobufjs` (load `.proto` at runtime; fast iteration)
- codegen (`ts-proto`, protobufjs static module, etc.; better type safety/perf)

## Design Decisions

### Keep internal bus payloads as fixed-size structs (recommended)

Rationale:

- `esp_event_post_to()` copies raw bytes; fixed-size structs are ideal.
- `protobuf-c` message structs use pointers for strings/repeated fields and are not safe to memcpy through an `esp_event` queue.

### Encode to protobuf at the boundary (WS), not as the in-bus representation

Rationale:

- Keeps event handlers fast and predictable.
- Avoids forcing decode in all internal consumers.
- Lets us keep smooth UI updates in the “UI dispatches the loop” pattern.

### Use `protobuf-c` on the device

Rationale:

- ESP-IDF includes `protobuf-c`.
- The generated code + runtime are small enough for this class of demo.

## Alternatives Considered

### Alternative A: “protobuf everywhere” (bus payload is packed protobuf bytes)

Structure:

- All posts use either a single `BUS_EVT_PROTO` or keep IDs but payload is:
  - `{ uint16_t len; uint8_t bytes[MAX]; }`
- Producers pack protobuf and post bytes.
- Consumers decode bytes to access fields.

Pros:
- One representation everywhere; forwarding is trivial.

Cons:
- More CPU overhead (encode/decode) and RAM for buffers.
- Requires enforcing a `MAX` payload size and handling “too large” events.
- Handlers lose trivial field access unless they decode.

### Alternative B: nanopb

Also a standard protobuf approach for embedded.

Pros:
- Often smaller than protobuf-c.

Cons:
- Not currently present in this repo’s toolchain; would add another generator/runtime dependency.

## Implementation Plan

1) Add `main/idl/demo_bus.proto` for the 0030 bus.
2) Decide whether generated files are checked in (recommended initially).
3) Add a generator script:
   - `0030-cardputer-console-eventbus/scripts/gen_proto.sh` running:
     - `protoc --c_out=... --proto_path=... demo_bus.proto`
4) Update firmware build:
   - Add `protobuf-c` to `0030-cardputer-console-eventbus/main/CMakeLists.txt`
   - Compile generated `demo_bus.pb-c.c`
5) Add `esp_http_server` WS endpoint (reuse patterns from `0029-mock-zigbee-http-hub`).
6) Add a bus→protobuf→WS bridge module and enable it behind a Kconfig flag.
7) Add a minimal web client and TS decoder driven by the same `.proto`.

## Open Questions

1) **TypeScript tooling preference**
   - Do you prefer `protobufjs` runtime loading or a codegen approach (`ts-proto`, etc.)?

2) **Host tooling availability**
   - This environment has `protoc`, but does not currently have `protoc-gen-c` (`protobuf-c` compiler plugin).
   - If you want, you can research which package you prefer for dev/CI on your OS, and I’ll wire the scripts accordingly.

## References

- Current demo bus implementation:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp`
- Proto-as-IDL (format-agnostic) discussion:
  - `ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/analysis/03-idl-idea-protobuf-defined-esp-event-payloads-not-necessarily-protobuf-wire.md`
