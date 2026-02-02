---
Title: Watermill-based Event Routing for JS Subscriptions
Ticket: 0073-PERMIT-JOIN-BUGFIX
Status: active
Topics:
  - zigbee
  - javascript
  - mqtt
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
  - Path: zigctl/pkg/jsruntime/zigctlmod/watch.go
    Note: Current JS watch stream implementation
  - Path: zigctl/pkg/zigbee/mqtt.go
    Note: MQTT request/response helper
Summary: Design for routing MQTT events through Watermill to provide robust JS subscriptions with backpressure control and structured filtering.
LastUpdated: 2026-02-02T00:00:00-05:00
WhatFor: Establish a resilient event routing layer for JS clients.
WhenToUse: Use when redesigning the event pipeline for Zigbee2MQTT subscriptions.
---

# Watermill-based Event Routing for JS Subscriptions

## Executive Summary
The current JS watch stream subscribes directly to MQTT topics and buffers messages in a small, in-memory channel. Under bursty traffic (`bridge/#`), messages can be dropped before JS consumes them. This design proposes a Watermill-based event router inside zigctl, providing structured routing, backpressure control, and optional persistence before events are delivered to JS. The goal is to improve reliability, make event filtering explicit, and simplify JS consumption.

## Problem Statement
JS scripts occasionally miss join events even when Zigbee2MQTT logs show activity. The current pipeline has these constraints:
- Fixed-size channel buffer and silent drops when full.
- No event normalization or metadata (retained/ts/source).
- No filtering or routing; JS must subscribe to raw MQTT topics.
- Shared MQTT client between request/response and watch flow.

We need a more robust event architecture that can handle bursts, provide structured channels, and support diagnostics.

## Proposed Solution
Introduce a Watermill-based routing layer in zigctl:

1. **Ingest** MQTT messages via a Watermill Subscriber (or a thin adapter around Paho).
2. **Normalize** payload + metadata into a consistent event struct.
3. **Route** messages to typed channels (bridge events, logs, device state, raw).
4. **Deliver** events to JS via a pull-based stream API with configurable buffering and drop policies.

### High-Level Architecture
```
MQTT Subscriber (Paho)
  -> Watermill Router
      -> bridge.event
      -> bridge.logging
      -> bridge.info
      -> device.state
      -> device.raw

JS Runtime
  -> subscribe({routes, buffer, dropPolicy})
  -> next() -> {route, topic, payload, retained, ts}
```

## Design Decisions
1. **Pull-based JS stream**: Keep `next()` to avoid unsafe callbacks into goja.
2. **Normalized event envelope**: Include `route`, `topic`, `payload`, `retained`, `timestamp`, and `source`.
3. **Configurable buffering**: Allow JS to request buffer size and drop policy.
4. **Optional persistence**: Watermill allows a persistent store (e.g., SQLite) if we need replay.

## Alternatives Considered
1. **Increase buffer only**
   - Pros: minimal change
   - Cons: still silent drops under sustained load; no routing/metadata
2. **Use separate MQTT client per JS stream**
   - Pros: fewer cross-effects with request/response
   - Cons: still lacks routing and structured filtering
3. **Watermill-based router (recommended)**
   - Pros: structured, observable, extensible; supports multiple sinks
   - Cons: additional dependency + integration work

## Implementation Plan
1. Add an internal `events` package with event envelope types and route definitions.
2. Add a Watermill router + MQTT subscriber (or adapter) in zigctl.
3. Replace direct `watch()` MQTT subscription with a JS subscription API to the router.
4. Add buffering and drop policies per JS subscription.
5. Add optional logging and debug counters (drops, lag, queue size).
6. Update JS scripts/playbooks to use the new route-based subscribe API.

## Open Questions
- Should we persist events (SQLite) for replay during debugging?
- Do we allow multiple JS streams, or share a single event bus?
- How should drop policies be expressed in JS (string enum vs numeric)?

## Success Criteria
- JS stream reliably captures join events seen in raw MQTT logs.
- Observable drop counts and queue size metrics.
- Operators can select event routes without subscribing to raw topics.
