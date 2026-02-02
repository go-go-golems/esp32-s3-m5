---
Title: Bug Report: JS Watch Missing Join Events
Ticket: 0070-ZIGBEE-JS-RUNTIME
Status: active
Topics:
  - zigbee
  - javascript
  - goja
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
  - Path: zigctl/pkg/jsruntime/zigctlmod/watch.go
    Note: Watch stream buffering and subscribe logic
  - Path: zigctl/pkg/jsruntime/zigctlmod/client.go
    Note: JS client watch + request logic
  - Path: zigctl/pkg/zigbee/mqtt.go
    Note: RequestOnce subscribe/publish logic
  - Path: ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/04-permit-join-watch-yaml.js
    Note: JS permit-join + watch script
  - Path: ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/05-confirm-run-permit-join.sh
    Note: plz-confirm gating for permit-join
Summary: Analysis of why JS watch did not surface join events while Zigbee2MQTT bridge logs indicated activity; includes architecture review, hypotheses, and design recommendations.
LastUpdated: 2026-02-02T00:00:00-05:00
WhatFor: Identify root causes and fix paths for missing join events in JS watch flows.
WhenToUse: Use when validating permit-join, troubleshooting JS event visibility, or redesigning event routing.
---

# Bug Report: JS Watch Missing Join Events

## Executive Summary
During permit-join validation, the JS watch stream showed bridge info/logging output but did not show device join events, even though Zigbee2MQTT logs indicated join-related activity. The most likely causes are (1) event timing and topic selection, (2) buffer drops in the JS watch stream during large bursts (notably `bridge/info` payloads), and (3) different event channels (bridge log vs bridge event vs device topics). The current JS watch architecture is simple and synchronous; it can drop messages under bursty load and does not expose retained/sequence metadata for debugging.

This report documents the MQTT-to-JS pipeline, the event architecture as implemented, concrete hypotheses, and a plan for reproductions and fixes. It also outlines a more robust event routing layer using Watermill to prevent drops, provide filtering, and improve observability.

## Observed Behavior
- JS permit-join script returned `status: ok` and streamed `bridge/state`, `bridge/info`, and `bridge/logging` output.
- No `bridge/event` join messages were seen in the JS stream during the window.
- Zigbee2MQTT logs showed ongoing device telemetry publishes (existing device state updates).

## Expected Behavior
- A device join during the permit-join window should result in:
  - `zigbee2mqtt/bridge/event` messages (e.g., device_joined/device_announce).
  - Potential device-specific publishes on `zigbee2mqtt/<friendly_name>`.
  - Additional `bridge/logging` messages indicating join and interview events.

## Evidence
- JS watch output included `bridge/info` (large payload), `bridge/logging`, and state messages.
- Permit-join response returned successfully (`status: ok`).
- No `bridge/event` messages seen during the same window.

## MQTT-to-JS Event Path (Current)
1. JS script calls `zigctl.connect({ broker, baseTopic, qos, timeout, debug })`.
2. JS script calls `client.watch({ topics, duration })`.
3. `client.Watch(...)` builds full topic strings with base topic and calls `newWatchStream`.
4. `newWatchStream`:
   - Subscribes to each topic using Paho MQTT with a per-topic handler.
   - Pushes messages into a buffered channel (`msgCh` size 64).
   - **Drops messages when the buffer is full.**
5. JS loop calls `stream.next()` synchronously and prints YAML.

### Key Implications
- **Backpressure**: The watch pipeline is “push → buffer → pull”. When JS printing is slow (large payloads), the buffer can fill and silently drop messages.
- **Burst risk**: Subscribing to `bridge/#` can trigger a burst (retained messages + large config schema) that floods the channel before JS can pull events.
- **Topic ambiguity**: Join info can appear in `bridge/event`, `bridge/logging`, and device topics. If the JS watch only listens to `bridge/event`, or if the join arrives during a burst and is dropped, it will be missed.

## Hypotheses (Ranked)

### H1: Join event dropped due to buffer overflow
- **Why likely**: `bridge/#` includes massive `bridge/info` payloads; printing them in JS is slow, and `msgCh` is only size 64.
- **Effect**: Join event arrives while the buffer is full and is dropped by the handler (`default` case in `select`).
- **Fix**: Increase buffer size, add `dropOldest` policy, or use a blocking send with timeouts to prevent silent loss.

### H2: Join published to a different topic than the JS is watching
- **Why likely**: Zigbee2MQTT publishes join-related info to `bridge/event` and may log to `bridge/logging`.
- **Effect**: Watching only `bridge/#` should include it, but if the script was previously set to `bridge/event` only (or uses `watchTopic` override), join could be missed.
- **Fix**: During diagnostics, watch both `bridge/#` and the device topic (e.g., `zigbee2mqtt/+/`), or run a parallel `mosquitto_sub` to confirm topic placement.

### H3: Join never occurred during the window
- **Why plausible**: If the device was not in pairing mode at the right time, only existing device telemetry appears.
- **Effect**: No join event would be present to catch.
- **Fix**: Use an operator-gated prompt immediately before permit-join, instructing to start pairing; add explicit “pair now” pause.

### H4: Event ordering or subscription timing
- **Why plausible**: If the join event happens before the subscribe is active, it is missed.
- **Effect**: `bridge/event` is not retained, so late subscribers won’t see it.
- **Fix**: Ensure watch subscription is established before enabling permit-join (already done), and consider using retained or persisted event logs.

### H5: MQTT client shared between RequestOnce and Watch
- **Why plausible**: The same client handles request/response and watch subscriptions; unsubscribe calls could affect shared state.
- **Effect**: Unlikely in current code, but could interfere in edge cases.
- **Fix**: Use a separate MQTT client for the watch stream to isolate subscriptions.

## Reproduction & Diagnostics Plan

### A) Compare JS vs mosquitto_sub
1. Start JS watch: `bridge/#`.
2. In parallel: `mosquitto_sub -h localhost -t 'zigbee2mqtt/bridge/#' -v`.
3. Trigger join during permit-join window.
4. Compare outputs:
   - If mosquitto_sub shows join and JS doesn’t → watch drops or JS logic issue.
   - If both miss → join did not occur or topic is different.

### B) Reduce payload burst
- Temporarily narrow to `bridge/event` to avoid huge config bursts.
- If join appears here, the previous miss is likely a buffer/drop issue.

### C) Increase JS watch buffer
- Add `bufferSize` option in JS watch config; set to 512 or 1024.
- Repeat the join test.

### D) Use retained metadata
- Add `retained` flag and `timestamp` to JS message output for debugging.
- Confirm whether messages are retained (which can crowd the initial buffer).

## Bugfix Suggestions (Near-Term)
1. **Buffer control**: Add `bufferSize` to watch options and grow default beyond 64.
2. **Drop policy**: Add `dropPolicy` (dropNewest vs dropOldest vs block-with-timeout).
3. **Retained metadata**: Include `msg.Retained()` in the JS output for diagnostics.
4. **Separate watch client**: Use a dedicated MQTT connection for watch streams to avoid side effects from request/response operations.
5. **Split “diag mode”**: Provide a “diagnostic watch” preset that subscribes to `bridge/event`, `bridge/logging`, and a device topic without the heavy `bridge/info` payload.

## Event Architecture Review

### Current Architecture (implicit)
- MQTT is the event bus; JS subscribes directly via `watch()`.
- No internal routing, filtering, or queueing beyond a small in-memory buffer.
- Event loss can occur under bursty load.

### Proposed Architecture (Watermill-based)
Introduce a structured event pipeline with Watermill:

1. **Ingest**: Use Watermill MQTT subscriber (or adapter) to receive raw MQTT messages.
2. **Normalize**: Decode payload, attach metadata (topic, retained, timestamp, baseTopic, source client).
3. **Route**: Watermill Router routes events into logical channels:
   - `bridge.event`
   - `bridge.logging`
   - `device.state`
   - `device.l2` (raw)
4. **Deliver**: JS runtime subscribes to a Go-managed event stream with backpressure + filtering.

### Watermill Design Sketch
```
MQTT Subscriber
  -> Watermill Router
      -> (route) bridge.event
      -> (route) bridge.logging
      -> (route) device.state
      -> (route) device.raw
            
JS Stream API (pull):
  - subscribe(filters, bufferSize, dropPolicy)
  - next() returns {topic, payload, retained, ts, route}
```

Benefits:
- Structured routing and consistent metadata.
- Backpressure can be managed per subscription.
- Easier to add logging, tracing, or persistence (e.g., SQLite or file sink).

## Suggested Tests (Targeted)
- **Test 1**: Use JS watch with `bridge/event` only; verify join appears.
- **Test 2**: Watch `bridge/#` with large buffer and compare to mosquitto_sub.
- **Test 3**: Use separate MQTT client for watch and verify join visibility.
- **Test 4**: Add `retained` output to detect retained bursts that saturate the buffer.
- **Test 5**: Trigger rapid publishes to simulate a burst and confirm drop behavior.

## Conclusion
The missing join event is most plausibly a combination of burst-induced drops in the JS watch buffer and the high-volume `bridge/#` topic selection. The current architecture is functional but fragile under load and not optimized for high-signal event capture. A Watermill-based routing layer would provide reliable, structured event delivery into the JS runtime while enabling backpressure and richer debugging.
