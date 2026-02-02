---
Title: zigctl architecture and data flow
Slug: zigctl-architecture-overview
Short: In-depth overview of zigctl's structure, how it maps to Zigbee2MQTT topics, and the request/response and streaming patterns it uses.
Topics:
  - zigbee
  - zigctl
  - architecture
Commands:
  - bridge
  - listen
  - mqtt
Flags:
  - --broker
  - --base-topic
  - --timeout
SectionType: GeneralTopic
IsTopLevel: true
ShowPerDefault: true
Order: 5
---

zigctl is a Glazed-based CLI that communicates with Zigbee2MQTT via MQTT. It is designed around small, composable verbs, each of which maps directly to a Zigbee2MQTT topic pattern. This document explains the architecture, data flow, and how to reason about command behavior.

## High-level architecture

zigctl has three layers:

1. **Command layer (Glazed + Cobra)**
   - Glazed owns parsing, schema, and structured output.
   - Cobra only registers commands and invokes Glazed.

2. **Zigbee layer (shared settings + MQTT helpers)**
   - A custom Glazed layer provides flags like `--broker`, `--base-topic`, `--tls`, and `--timeout`.
   - Shared helpers connect to MQTT and implement request/response patterns.

3. **Zigbee2MQTT backend**
   - The actual Zigbee coordinator and network run inside Zigbee2MQTT.
   - zigctl never touches the radio directly; everything is MQTT topics.

## Command groups and responsibilities

- **bridge/**: Request/response operations against Zigbee2MQTT bridge topics (info, devices, permit-join).
- **listen/**: Streaming subscriptions for device state and raw topics.
- **mqtt/**: Raw publish/subscribe helpers, used as a low-level escape hatch.

## Topic patterns

Zigbee2MQTT topics follow a predictable structure:

- **Bridge requests**: `zigbee2mqtt/bridge/request/<command>`
- **Bridge responses**:
  - Some commands respond on `zigbee2mqtt/bridge/response/<command>`
  - Others publish to state topics like `zigbee2mqtt/bridge/info`
- **Device state**: `zigbee2mqtt/<friendly_name>`
- **Device commands**: `zigbee2mqtt/<friendly_name>/set`

zigctl commands are simple wrappers around these patterns.

## Request/response flow (bridge commands)

A typical bridge request flow:

1. Subscribe to the response topic.
2. Publish the request payload.
3. Wait for one response message.
4. Render the response into structured output.

This pattern appears in:
- `bridge info`
- `bridge devices`
- `bridge permit-join`

## Streaming flow (listen commands)

Streaming commands subscribe to a topic and emit a new structured row for each message:

- `listen raw`: subscribes to arbitrary topics (`zigbee2mqtt/#` by default in most examples).
- `listen state`: subscribes to a device topic and extracts the device name from the MQTT topic.

These commands are intentionally long-lived and are typically run with a timeout in scripts.

## Output format philosophy

Glazed output provides:
- Consistent JSON/YAML/CSV/tabular formats.
- Programmatic pipelines using `--fields`, `--sort-columns`, and `--output`.

This is why zigctl never prints ad-hoc text for operational commands; it always emits structured rows.

## Configuration and defaults

zigctl defaults:
- Broker: `mqtt://localhost:1883`
- Base topic: `zigbee2mqtt`
- Timeout: `10s`

In this repository's test stack, the host broker is mapped to **1884**, so examples set `--broker mqtt://localhost:1884`.

## Safety model

- `permit-join` is time-bounded and must be explicitly enabled.
- Destructive actions (device removal, resets) are not yet implemented; they should be gated by explicit confirmation when added.

## Extending zigctl

When adding new commands:
- Create one file per verb.
- Place it under the appropriate group directory.
- Add a LongDescription with examples.
- Use the Zigbee Glazed layer (`zigbee.NewZigbeeLayer`) to inherit broker flags.

This keeps the CLI discoverable and consistent.
