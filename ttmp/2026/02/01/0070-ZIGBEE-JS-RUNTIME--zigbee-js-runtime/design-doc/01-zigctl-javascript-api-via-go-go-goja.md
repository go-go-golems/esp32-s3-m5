---
Title: Zigctl JavaScript API via go-go-goja
Ticket: 0070-ZIGBEE-JS-RUNTIME
Status: active
Topics:
    - zigbee
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../code/wesen/corporate-headquarters/go-go-goja/engine/runtime.go
      Note: goja runtime setup + module registry
    - Path: ../../../../../../../../../../code/wesen/corporate-headquarters/go-go-goja/modules/common.go
      Note: NativeModule interface and registry
    - Path: ../../../../../../../../../../code/wesen/corporate-headquarters/go-go-goja/modules/database/database.go
      Note: Stateful module pattern + docs
    - Path: ../../../../../../../../../../code/wesen/corporate-headquarters/go-go-goja/modules/exec/exec.go
      Note: Example Go->JS binding pattern
    - Path: ../../../../../../../../../../code/wesen/corporate-headquarters/go-go-goja/modules/fs/fs.go
      Note: Simple module pattern
    - Path: zigctl/cmd/js/repl.go
      Note: JS runtime REPL command
    - Path: zigctl/cmd/js/root.go
      Note: JS command group registration
    - Path: zigctl/cmd/js/run.go
      Note: JS runtime run command
    - Path: zigctl/pkg/jsruntime/runtime.go
      Note: JS runtime wrapper using go-go-goja
    - Path: zigctl/pkg/jsruntime/zigctlmod/client.go
      Note: MQTT client wrapper for JS
    - Path: zigctl/pkg/jsruntime/zigctlmod/config.go
      Note: JS config parsing helpers
    - Path: zigctl/pkg/jsruntime/zigctlmod/watch.go
      Note: Watch stream implementation
    - Path: zigctl/pkg/jsruntime/zigctlmod/zigctlmod.go
      Note: Zigctl JS module loader
    - Path: zigctl/pkg/zigbee/client.go
      Note: MQTT client helpers
    - Path: zigctl/pkg/zigbee/mqtt.go
      Note: Request/publish helpers
    - Path: zigctl/testdata/jsruntime/join-watch.js
      Note: JS join/watch example
    - Path: zigctl/testdata/jsruntime/plug-control.js
      Note: JS plug control example
ExternalSources: []
Summary: Research + design options for a JS scripting API over zigctl using go-go-goja runtime + native modules, with detailed tradeoffs and example APIs.
LastUpdated: 2026-02-01T21:56:55-05:00
WhatFor: Choose a concrete approach for JS scripting over Zigbee2MQTT/zigctl and provide implementation guidance.
WhenToUse: Use when adding JavaScript scripting to zigctl or building a JS API around Zigbee2MQTT.
---



# Zigctl JavaScript API via go-go-goja

## Executive Summary

We can expose zigctl functionality to JavaScript scripts by embedding the existing `go-go-goja` runtime **inside zigctl**. The strongest options are:

1) **Native goja module inside zigctl** wrapping `zigctl/pkg/zigbee` (best for performance, streaming, and ergonomic JS API).  
2) **JS API wrapper that shells out to `zigctl` via the `exec` module** (fastest to deliver, but brittle and slower).  
3) **Dedicated MQTT module + JS API built directly on Zigbee2MQTT topics** (flexible and decoupled, but more work to get right).  
4) **Long-running bridge service (HTTP/gRPC) + JS client** (clean separation and remote execution, but more infrastructure).

Recommendation: **Implement (1) as the long-term core inside zigctl**, optionally ship (2) as a temporary compatibility bridge while the native module matures. Use (3) if we want scripting without embedding zigctl’s Go packages.

---

## Problem Statement

We want a “great JS API” so power users can write scripts that control Zigbee2MQTT networks (permit join, device control, event listening, diagnostics) without learning Go or managing MQTT details. The JS runtime should be embeddable, safe-ish, and able to stream events in real time.

The challenge: design an API surface that is ergonomic for JS while still grounded in zigctl’s Go implementation and Zigbee2MQTT semantics.

---

## Requirements

### Functional

- Connect to Zigbee2MQTT brokers (TLS, base topic, QoS, timeouts).
- Permit join and watch events in the same script.
- Control devices (on/off, power-on behavior, timers, raw payloads).
- Read device state and bridge info.
- Stream events from the bridge and device topics.
- Provide helper utilities (JSON/YAML decode, payload validation, retries).

### Non-functional

- Scripts should be deterministic and reproducible.
- Errors should be JS exceptions or structured error objects.
- Must support long-running scripts (watch loops) without blocking the host.
- Should be testable without real hardware (mock broker / recorded events).

---

## Observations from go-go-goja (local source)

### Runtime and module model

The runtime is created via `engine.New()` and uses `goja_nodejs/require` with a registry of native modules:

```
engine.New()
  -> require.NewRegistry()
  -> modules.EnableAll(reg)
  -> reg.Enable(vm)
```

Modules are registered by implementing `modules.NativeModule` and calling `modules.Register(...)` in `init()`.

Example modules:
- `fs` (simple file IO)
- `exec` (shell out to external commands)
- `database` (sqlite via `database/sql`)

This means zigctl can add a new `zigctl` module by following the same pattern and registering it before runtime creation.

### Relevant sources

- `go-go-goja/engine/runtime.go` — runtime setup and module registration.
- `go-go-goja/modules/common.go` — module interface and registry.
- `go-go-goja/modules/exec` — minimal pattern for binding Go functions to JS exports.

---

## Architecture options

### Option A — Native `zigctl` goja module in zigctl (recommended)

**Idea**: Create a new module **inside zigctl** (e.g. `zigctl/pkg/jsruntime/zigctlmod`) that directly wraps zigctl’s Go packages (`zigctl/pkg/zigbee`), exposing a clean JS API. This yields strong performance and rich streaming without shelling out.

#### High-level API design

```js
const zigctl = require('zigctl');

const client = zigctl.connect({
  broker: 'mqtt://localhost:1884',
  baseTopic: 'zigbee2mqtt',
  qos: 0,
  timeout: '10s',
});

// one-shot
client.bridgeInfo();
client.devices();
client.permitJoin({ seconds: 60, device: '' });

// streaming
const stream = client.watch({
  topics: ['bridge/event', 'device/+/state'],
  duration: 60,
});
stream.on('event', (evt) => console.log(evt));
stream.on('error', (err) => console.error(err));
stream.on('end', () => console.log('done'));
```

#### API components

- `zigctl.connect(config)` -> `Client`
- `Client.bridgeInfo()` -> object
- `Client.devices()` -> array
- `Client.permitJoin({seconds, device, watch?})` -> result + optional stream
- `Client.publish(topic, payload)`
- `Client.subscribe(topics, handler)`
- `Client.watch({topics, duration})` -> EventStream

#### Go bindings (sketch)

```go
func (m *Module) Loader(vm *goja.Runtime, moduleObj *goja.Object) {
  exports := moduleObj.Get("exports").(*goja.Object)
  _ = exports.Set("connect", func(cfg map[string]interface{}) (*goja.Object, error) {
    client := newClientFromConfig(cfg)
    return newJSClient(vm, client), nil
  })
}
```

#### Streaming model

Option A1: **Callback-based** (simpler)

```js
client.watch({ topics, duration }, function(evt) { ... })
```

Option A2: **Emitter-like** (event emitter)

```js
const stream = client.watch(...)
stream.on('event', handler)
stream.on('end', handler)
```

Option A3: **Async iterator** (ergonomic but harder in goja)

```js
for await (const evt of client.watchAsync({topics, duration})) { ... }
```

> goja does not provide native async/await without extra glue, so A1/A2 are safest.

#### Pros

- Tight integration with zigctl (no CLI parsing).
- Streaming events are real-time and structured.
- Errors map cleanly to JS exceptions.
- No shell-out or temp files.

#### Cons

- More Go work upfront.
- Must maintain API surface and docs.
- Requires careful lifecycle management (subscribe/unsubscribe, disconnects).

#### Implementation considerations

- Reuse `zigctl/pkg/zigbee` for MQTT + request helpers.
- Add a small session manager in Go to prevent leaks.
- Provide JSON/YAML convenience functions for payloads.

---

### Option B — JS API shelling out to `zigctl` (fast prototype)

**Idea**: Build a JS module that uses `require('exec').run(...)` to call the zigctl CLI, parse its output, and return results to JS.

#### Example

```js
const exec = require('exec');

function bridgeInfo(cfg) {
  const args = [
    'run', './', 'bridge', 'info',
    '--broker', cfg.broker,
    '--base-topic', cfg.baseTopic,
    '--output', 'json',
  ];
  const out = exec.run('go', args);
  return JSON.parse(out);
}
```

#### Pros

- Very low engineering effort.
- Reuses zigctl’s CLI behavior immediately.

#### Cons

- Slow (process per call).
- Harder to stream events cleanly.
- brittle to CLI output changes.
- error handling is poor (string parsing).

#### When to use

- Quick prototyping or bridging until native module exists.

---

### Option C — MQTT-first JS API (no zigctl dependency)

**Idea**: Expose an MQTT module in goja and implement Zigbee2MQTT topic conventions in JS. This bypasses zigctl entirely.

#### Example

```js
const mqtt = require('mqtt');

const client = mqtt.connect({ broker: 'mqtt://localhost:1884' });
client.publish('zigbee2mqtt/bridge/request/permit_join', { value: true, time: 60 });
client.subscribe('zigbee2mqtt/bridge/event', (msg) => console.log(msg));
```

#### Pros

- Pure JS usage of Zigbee2MQTT semantics.
- Decoupled from zigctl.

#### Cons

- Must re-implement Zigbee2MQTT topic semantics in JS.
- Duplicate logic already in zigctl.
- No typed/validated payloads unless we add schema.

#### Implementation details

- Build a `mqtt` goja module (paho). The API should handle reconnects, QoS, subscriptions, and JSON payload encoding.
- Provide helper JS utilities for standard zigbee2mqtt request/response flows.

---

### Option D — Service bridge (HTTP/gRPC)

**Idea**: Run a Go service that exposes zigctl functionality over HTTP or gRPC. JS scripts call it via HTTP client module.

#### Pros

- Clean separation between JS and Go.
- Allows remote execution and centralized access control.

#### Cons

- Requires long-running service and auth.
- More infrastructure and deployment complexity.

---

## Recommended API surface (Option A baseline)

### Module structure

```
require('zigctl')
  .connect(config) -> Client

Client
  .bridgeInfo() -> object
  .devices() -> array
  .permitJoin({seconds, device?, watch?}) -> result
  .publish(topic, payload)
  .request(topic, payload, responseTopic, timeout?)
  .watch({topics, duration, filter?}) -> Stream
  .close()

Stream
  .on('event', fn)
  .on('error', fn)
  .on('end', fn)
  .stop()
```

### Example script: join + watch

```js
const zigctl = require('zigctl');

const client = zigctl.connect({
  broker: 'mqtt://localhost:1884',
  baseTopic: 'zigbee2mqtt',
  qos: 0,
});

const stream = client.watch({ topics: ['bridge/event'], duration: 60 });
stream.on('event', (evt) => {
  if (evt.type === 'device_joined') {
    console.log('joined', evt.data.friendly_name || evt.data.ieee_address);
  }
});

client.permitJoin({ seconds: 60 });
```

### Example: control a plug

```js
client.publish('zigbee2mqtt/office_plug/set', { state: 'ON' });
client.publish('zigbee2mqtt/office_plug/set', { countdown_to_turn_off: 300 });
```

---

## Design decisions

- **Expose a connection object rather than global functions** to keep config scoped.
- **Use event emitter semantics for streaming** (most JS-friendly without async/await support).
- **Return plain JS objects** for payloads; avoid exposing Go types directly.
- **Prefer Zigbee2MQTT semantic helpers** (permitJoin, devices) but still allow raw publish for power users.

---

## Alternatives considered (summary)

- **Shelling out to CLI**: fastest to prototype, not great for streaming.
- **MQTT-only JS API**: clean separation but duplicates zigctl logic.
- **Service bridge**: scalable but heavy-weight for local scripting.

---

## Implementation plan (Option A, detailed)

### Phase 1 — Module scaffolding + wiring

1) Create `zigctl/pkg/jsruntime/zigctlmod` with the standard module shape:
   - `zigctlmod.go` implements `modules.NativeModule` (`Name`, `Doc`, `Loader`).
   - `client.go` contains the Go wrapper around zigctl MQTT logic.
2) Create `zigctl/pkg/jsruntime/runtime.go` that:
   - blank-imports `zigctlmod` so it registers,
   - calls `engine.New()` from go-go-goja to get a runtime with `require()` enabled.
3) Add module documentation string and a minimal JS usage snippet in `Doc()`.

### Phase 2 — Config + connection primitives

4) Define JS-facing config shape with defaults:
   - `broker`, `baseTopic`, `tls`, `cafile`, `cert`, `key`, `qos`, `timeout`
5) Map JS config -> `zigctl/pkg/zigbee.Settings` (or `Config`) and connect using `zigbee.Connect`.
6) Create a `Client` wrapper with explicit lifecycle:
   - `Close()` disconnects MQTT and tears down subscriptions.
   - Guard against double-close.

### Phase 3 — Core API surface (non-streaming)

7) Implement `bridgeInfo()`, `devices()`, `permitJoin({seconds, device})` using the existing request/response helpers:
   - Use `zigbee.RequestOnce` with response topics where available.
8) Implement `publish(topic, payload)` and `request(topic, payload, responseTopic, timeout)` for power users.
9) Validate payloads and return structured JS objects (no Go types).

### Phase 4 — Streaming and watch support

10) Implement `watch({topics, duration, filter?})`:
    - Subscribe with QoS from client settings.
    - Forward messages to JS callbacks as decoded JSON when possible.
    - Gracefully exit after `duration` seconds (or when `stop()` is called).
11) Provide an event-emitter-like object:
    - `.on('event', fn)`, `.on('error', fn)`, `.on('end', fn)`
12) Document the streaming model (buffering, backpressure, and safe callback invocation).

### Phase 5 — Examples + documentation

13) Add example JS scripts under `zigctl/testdata/zigctl/` (or `zigctl/testdata/jsruntime/`):
    - `join-watch.js`
    - `plug-control.js`
14) Optionally add a REPL helper or CLI example to preload the module.

### Phase 6 — Build/test scaffolding

15) Add a temporary `replace` in **zigctl** `go.mod` if go-go-goja needs a local path override (only if required).
16) Add a basic integration smoke test script (manual run) that:
    - Connects
    - Requests bridge info
    - Subscribes and receives an event

### Deliverables

- `zigctl/pkg/jsruntime/zigctlmod/*` (native module + client wrapper)
- `zigctl/pkg/jsruntime/runtime.go` (runtime construction + module registration)
- JS examples in `testdata/zigctl/`
- Documentation + help strings

---

## Risks and mitigations

- **Resource leaks**: ensure `unsubscribe` + `Disconnect()` on `Client.close()`.
- **Blocking callbacks**: keep JS callback invocation controlled; consider buffering.
- **Compatibility drift**: keep zigctl and module in the same repo or pinned version.

---

## Diagram (Option A)

```
+---------------------------+         +-----------------------------+
| JS Script (goja runtime)  |         | Zigbee2MQTT (MQTT broker)   |
|                           |         |                             |
| require('zigctl')         |         |  zigbee2mqtt/bridge/...     |
|   -> native module        |  MQTT   |  zigbee2mqtt/<device>/...   |
|                           |<------->|                             |
+-------------+-------------+         +-----------------------------+
              |
              | Go bindings
              v
+---------------------------+
| zigctl/pkg/zigbee client  |
|  - Connect()              |
|  - RequestOnce()          |
|  - Subscribe()            |
+---------------------------+
```

---

## Open questions

- Should we support async/await or keep callback-based streaming?
- Should the JS API handle automatic reconnection (like MQTT clients) or leave it explicit?
- Should there be a higher-level “device” abstraction (device object with methods)?

---

## References (local source)

- `go-go-goja/engine/runtime.go`
- `go-go-goja/modules/common.go`
- `go-go-goja/modules/exec/exec.go`
- `go-go-goja/modules/database/database.go`
- `go-go-goja/modules/fs/fs.go`
- `zigctl/pkg/zigbee` (client + request helpers)
