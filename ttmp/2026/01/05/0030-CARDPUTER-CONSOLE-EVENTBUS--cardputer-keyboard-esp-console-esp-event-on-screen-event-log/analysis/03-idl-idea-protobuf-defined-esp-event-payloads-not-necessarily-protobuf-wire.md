---
Title: 'IDL idea: protobuf-defined esp_event payloads (not necessarily protobuf wire)'
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
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Current hand-written event IDs/payload structs used as baseline for proto-as-IDL codegen.
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T08:13:59.555666176-05:00
WhatFor: ""
WhenToUse: ""
---


# IDL idea: protobuf-defined `esp_event` payloads (without committing to protobuf wire)

Goal: use **`.proto` files as the single source of truth** for event IDs and payload schemas, then generate:

- **Embedded** (C/C++): strongly typed event payload structs + helpers compatible with `esp_event_post_to(...)`.
- **Host/UI** (TypeScript): strongly typed structures for WebSocket/SSE clients.
- **Bridge tooling**: optional encoders/formatters that can forward events over WebSocket (protobuf wire, JSON, CBOR, or a custom compact format), all driven from the same schema.

This is about **schema + codegen** first; the **wire format is a separate choice**.

## Why protobuf works well as an IDL even if you don't use protobuf wire

`.proto` gives you:

- A standard schema language (well-known tooling).
- A stable type system (enums, nested messages, `oneof`, optional fields, repeated fields).
- A canonical place to document fields and evolution rules.
- A simple way to get a machine-readable descriptor (`FileDescriptorSet`) that custom compilers can consume.

If you later decide to use protobuf wire for WebSocket, you can—without changing the schema.

## The key constraint: `esp_event` expects POD-ish payloads

`esp_event_post_to(loop, base, id, data, data_size, ...)` copies `data_size` bytes into the loop queue.

So payloads should be:

- **fixed-size and self-contained** (no pointers requiring deep-copy),
- **small enough** for the queue and stack discipline,
- **stable layout** across producers/consumers.

That impacts how we map proto features:

- `string` / `bytes` usually need **bounded storage** (fixed arrays) or an owned buffer strategy.
- `repeated` needs **bounded arrays** or a separate “arena”/pool.
- `map<...>` is awkward without dynamic allocation; typically disallowed or bounded.

This is the main reason to treat protobuf as **IDL only**: you can generate “embedded-friendly” structs with explicit bounds even if proto is more flexible.

## Proposed schema style

### Option A: “One base, one envelope message”

Use a single `.proto` for a domain and define:

- an enum of event IDs
- one message per payload type
- an envelope that carries `id` + a `oneof payload`

Example:

```proto
syntax = "proto3";
package demo.bus.v1;

enum EventId {
  EVENT_ID_UNSPECIFIED = 0;
  EVENT_ID_KB_KEY = 1;
  EVENT_ID_CONSOLE_POST = 2;
  EVENT_ID_HEARTBEAT = 3;
}

message KbKey {
  int64 ts_us = 1;
  uint32 keynum = 2;
  uint32 modifiers = 3;
}

message ConsolePost {
  int64 ts_us = 1;
  string msg = 2; // needs bounds for embedded; see “custom options”
}

message Heartbeat {
  int64 ts_us = 1;
  uint32 heap_free = 2;
  uint32 dma_free = 3;
}

message Event {
  EventId id = 1;
  oneof payload {
    KbKey kb_key = 10;
    ConsolePost console_post = 11;
    Heartbeat heartbeat = 12;
  }
}
```

Advantages:
- Great for “forward everything over WebSocket” because the envelope is already there.
- The TypeScript side gets a nice discriminated union (`oneof`).

Trade-off:
- Embedded side still likely posts the specific payload struct by ID (more efficient), and the envelope is used mainly for bridging.

### Option B: “No envelope; ID + per-message definitions”

Keep messages separate and treat proto as “declares payload types”, but define event IDs elsewhere (or via an enum).
This can reduce envelope overhead in embedded code, but makes bridging code do more work.

## Embedded mapping: generating C structs that are safe for `esp_event`

### Core idea

We compile `.proto` → generated headers:

- `*_idl.h` containing:
  - `ESP_EVENT_DECLARE_BASE(...)` / `DEFINE_BASE(...)` (or just constant names)
  - `enum <domain>_event_id_t`
  - `typedef struct { ... } <payload>_t` for each message
  - compile-time size constants and bounds

And optionally:

- `*_idl_meta.c/.h` containing:
  - `const event_desc_t[]` mapping id → name, payload size, field metadata

### The missing piece: bounds (proto doesn’t specify them)

To make `string` and `repeated` usable without dynamic allocation, we need bounds.

Two approaches:

1) **Conventions**: decide that `string` always becomes `char msg[96]` in embedded and document that as a project rule.
2) **Custom proto options** (recommended): declare options like:

```proto
import "google/protobuf/descriptor.proto";

extend google.protobuf.FieldOptions {
  uint32 fixed_string_len = 50001;
  uint32 max_count = 50002;
}
```

Then annotate fields:

```proto
string msg = 2 [(fixed_string_len) = 96];
repeated uint32 keynums = 3 [(max_count) = 8];
```

Your custom compiler reads these options from the descriptor set and generates bounded storage in C.

### Generated C struct example

For `ConsolePost { int64 ts_us; string msg[(fixed_string_len)=96]; }`:

```c
typedef struct {
  int64_t ts_us;
  char msg[96];
} demo_console_post_t;
```

For a bounded repeated field:

```c
typedef struct {
  uint8_t n_keynums;
  uint32_t keynums[8];
} demo_keys_t;
```

This aligns with how `0030-cardputer-console-eventbus` already structures payloads for `esp_event`.

## Bridging events to WebSocket with the same schema

Once you have:

- known event IDs
- known payload layouts + field metadata

You can export events in multiple ways:

### 1) Protobuf wire (lowest friction)

The embedded device serializes `Event` (envelope) as protobuf bytes and sends on WebSocket.

Pros:
- Minimal custom tooling.
- TypeScript can use existing protobuf decoders.

Cons:
- Need an embedded protobuf encoder (nanopb/protobuf-c/custom).

### 2) JSON derived from proto (human-friendly)

Generator produces:
- TS types + JSON codecs (or zod schemas)
- Embedded “formatter” that turns payload → JSON object

Pros:
- Easy debugging.

Cons:
- Larger payloads and slower formatting.

### 3) Custom binary “flat” encoding, still driven by proto

Treat proto as IDL only and create your own wire:

- header: `{u32 schema_hash, u32 event_id, u32 payload_len}`
- payload: TLV or fixed-layout packing based on the generated struct

Pros:
- Very fast; no dependency on protobuf runtime.

Cons:
- You are effectively writing a protobuf-like format; must define compatibility rules and implement encoders/decoders.

The important part: **the schema stays the proto**, and both embedded + TS are generated from it.

## What a “custom compiler” could look like

### Inputs

- `.proto` sources
- `protoc --descriptor_set_out=...` output (`FileDescriptorSet`)

### Outputs (suggested)

- `gen/<pkg>_idl.h` (C structs + IDs)
- `gen/<pkg>_idl.ts` (TS types)
- `gen/<pkg>_idl_meta.json` (optional)
- optional encoders/decoders:
  - `gen/<pkg>_json.c/.h`
  - `gen/<pkg>_ws.c/.h`

### Schema evolution rules (to document)

- Never renumber fields.
- Only append new fields with higher numbers.
- Use `reserved` for removed fields.
- Bounded array/string size increases can increase `esp_event` queue pressure.

## What to extract into a reusable component

If we like this approach, a good next boundary is:

- `components/event_idl/`
  - build glue (`protoc` invocation + generator)
  - a small runtime helper library (optional):
    - schema hash/versioning helpers
    - metadata table utilities
    - generic JSON/custom encoders (if we go that route)

Each firmware would then define its `.proto`, include generated headers, and optionally enable a “bridge” module to forward events to WebSocket using generated codecs.

## How this ties back to 0030

0030 is already close to the “generated struct” model:

- event IDs are an enum
- payloads are fixed-size structs
- producers post to the bus
- UI dispatches the loop and renders

Proto-as-IDL would mainly:

- prevent drift between embedded payload structs and host/UI types,
- make adding events cheap (schema edit + regenerate),
- enable systematic bridging of all events to WebSocket without hand-written per-event glue.

