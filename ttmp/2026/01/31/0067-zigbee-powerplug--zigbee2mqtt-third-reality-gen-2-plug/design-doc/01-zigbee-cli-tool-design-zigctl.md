---
Title: Zigbee CLI tool design (zigctl)
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Design for a Go CLI (zigctl) that controls and listens to a Zigbee2MQTT network using Glazed+Cobra, with rich verbs, streaming listeners, and structured output."
LastUpdated: 2026-02-01T19:58:51-05:00
WhatFor: "Define CLI verbs, flags, architecture, and implementation plan for a Zigbee network interaction tool."
WhenToUse: "Use before implementing zigctl or when aligning CLI behavior with Zigbee2MQTT MQTT topics."
---

# Zigbee CLI tool design (zigctl)

## Executive Summary

Design a Go CLI tool, `zigctl`, that interacts with Zigbee2MQTT over MQTT to send commands, query state, and listen to streaming events. The CLI uses Glazed for command parsing + structured output (JSON/YAML/CSV/tables), with Cobra only used as the registration/entrypoint layer. It includes verbs for bridge operations, device and group management, raw MQTT access, and long-running listeners.

## Problem Statement

We need a robust, scriptable CLI for Zigbee2MQTT that can:
- Send bridge requests and device commands safely.
- Query and export network/device state in structured formats.
- Listen to ongoing traffic (state changes, events, logs) without a GUI.
- Support consistent output formatting and machine-friendly pipelines.
- Be explicit about topics and response channels to avoid confusion.

## Proposed Solution

Implement `zigctl` as a Go CLI built on:
- **Glazed** for command parsing, parameter layers, and structured output.
- **Cobra** only for registering commands and providing the root entrypoint.
- **MQTT client** (paho.mqtt.golang or eclipse/paho) to communicate with the broker.

The CLI follows a verb/noun structure with global flags for broker, base topic, auth, timeouts, and output formatting.

### Design Goals

- Provide a complete, explicit mapping to Zigbee2MQTT MQTT topics and request/response rules.
- Offer both single-shot commands and streaming listeners.
- Support structured output for automation; preserve human-readable defaults.
- Make safety boundaries obvious (permit_join windows, confirmations for destructive actions).

### Non-Goals

- Full Zigbee protocol capture or RF sniffing (out of scope for MQTT-based CLI).
- A GUI or web dashboard.
- Acting as a Zigbee coordinator (Zigbee2MQTT remains the source of truth).

## CLI Architecture

### Command topology (verbs and groups)

```
zigctl
  bridge     # bridge-level operations
  device     # device-level operations
  group      # group management
  listen     # streaming listeners
  mqtt       # raw MQTT tools
  config     # config/profile management
  export     # export state snapshots
  health     # health status helpers
```

### Code organization (repo layout)

Organization rules:
- One directory per group (bridge/, device/, group/, listen/, mqtt/, config/, export/, health/).
- One file per verb (e.g., bridge/info.go, device/set.go).
- Each group directory has a root.go that registers the group's commands with the Cobra root.

This keeps command discovery and help output aligned with the CLI verbs.

### Global flags

All global Zigbee connection flags live in a custom Glazed layer that is included on every command:
- `--broker` (default: `mqtt://localhost:1883`)
- `--base-topic` (default: `zigbee2mqtt`)
- `--tls`, `--cafile`, `--cert`, `--key`
- `--qos` (default: 0)
- `--timeout` (default: 10s)
- `--profile` (named broker profile)
- `--output` (glazed; json/yaml/csv/table)
- `--fields`, `--sort-columns` (glazed standard)
- `--raw` (print raw payloads without decode)
- `--verbose` / `--debug`

### Command categories and behavior

#### 1) Bridge commands

- `zigctl bridge info`
  - Publishes `bridge/request/info`, subscribes to `bridge/info`.
- `zigctl bridge devices`
  - Publishes `bridge/request/devices`, subscribes to `bridge/devices`.
- `zigctl bridge definitions`
  - Publishes `bridge/request/definitions`, subscribes to `bridge/definitions`.
- `zigctl bridge permit-join --seconds 60 [--device <friendly>]`
  - Publishes `bridge/request/permit_join`, listens for `bridge/response/permit_join`.
- `zigctl bridge restart`
  - Publishes `bridge/request/restart` and waits for confirmation.
- `zigctl bridge networkmap --type graphviz|raw|plantuml`
  - Publishes `bridge/request/networkmap`, reads `bridge/response/networkmap`.
- `zigctl bridge options set --path advanced.log_level --value info`
  - Publishes `bridge/request/options` with a JSON payload.

#### 2) Device commands

- `zigctl device list`
  - Uses `bridge/devices` payload and presents as rows.
- `zigctl device get <friendly> <property>`
  - Publishes `base/<friendly>/get` and waits for update on state topic.
- `zigctl device set <friendly> --state ON|OFF [--json '{...}']`
  - Publishes `base/<friendly>/set`.
- `zigctl device rename <from> <to>`
  - Publishes `bridge/request/device/rename`.
- `zigctl device remove <ieee> [--force]`
  - Publishes `bridge/request/device/remove`.
- `zigctl device interview <ieee>`
  - Publishes `bridge/request/device/interview`.
- `zigctl device configure <ieee>`
  - Publishes `bridge/request/device/configure`.
- `zigctl device options <ieee> --json '{...}'`
  - Publishes `bridge/request/device/options`.
- `zigctl device bind|unbind --from <device|group> --to <device|group> --cluster genOnOff`

#### 3) Group commands

- `zigctl group add --name living_room --id 1`
- `zigctl group remove --id 1`
- `zigctl group rename --from old --to new`
- `zigctl group members add --group living_room --device lamp`
- `zigctl group members remove --group living_room --device lamp`
- `zigctl group members remove-all --group living_room`

#### 4) Listen commands (streaming)

- `zigctl listen state [--device <friendly>] [--jsonpath <expr>]`
  - Subscribes to `base/#` or `base/<friendly>` and streams decoded JSON.
- `zigctl listen events`
  - Subscribes to `bridge/event` and `bridge/logging`.
- `zigctl listen health`
  - Subscribes to `bridge/health`.
- `zigctl listen raw --topic <topic>`
  - Raw MQTT subscription (passthrough).

Listening commands are long-running and default to human-readable line output. They optionally support glazed streaming (row-per-message) when `--output` is provided.

#### 5) MQTT raw tools

- `zigctl mqtt pub --topic <t> --message <json>`
- `zigctl mqtt sub --topic <t>`

#### 6) Export commands

- `zigctl export devices --out devices.json`
- `zigctl export networkmap --type graphviz --out map.dot`
- `zigctl export config --out bridge-info.json`

## Output and Data Model

### Structured Output (Glazed)

Use Glazed patterns from `glaze help build-first-command`:
- Define settings structs with `glazed.parameter` tags.
- Decode values with `values.DecodeSectionInto`.
- Emit rows with `types.Row` + `types.MRP`.
- Support dual-mode commands for human text vs structured output.

### Command help requirements

Every command must include a LongDescription with concrete examples. This ensures the CLI is self-documenting and makes it easy to discover the correct topics/flags at runtime.

Key result: every single-shot command can output JSON/YAML/CSV without extra code.

### Streaming Output

Streaming commands can:
- Emit line-delimited JSON (default).
- Emit glazed rows for filtered properties (e.g., device, state, time).
- Support `--fields` to select columns from JSON payloads.

## Configuration

### Config file

`~/.config/zigctl/config.yaml`:

```yaml
profiles:
  default:
    broker: mqtt://localhost:1883
    base_topic: zigbee2mqtt
    user: ""
    password: ""
    tls: false
  lab:
    broker: mqtts://zigbee-broker.local:8883
    user: "zigbee"
    password: "***"
```

### Environment variables

- `ZIGCTL_BROKER`, `ZIGCTL_BASE_TOPIC`, `ZIGCTL_USER`, `ZIGCTL_PASSWORD`

## Error Handling and Safety

- Require confirmation for destructive commands (`device remove`, `bridge restart`) unless `--yes` is set.
- Enforce max permit-join duration (default 60s) unless `--force`.
- Always include the response topic in error messages when timeouts occur.
- Provide `--timeout` to control waiting for responses.

## Design Decisions

1. **Glazed + Cobra**: Glazed handles command parsing and structured output; Cobra only registers commands. This keeps CLI behavior consistent with Glazed schemas while preserving Cobra's command tree integration.
2. **Dual-mode commands**: Many commands benefit from human output by default while still providing machine output when needed.
3. **Bridge vs state topics**: We explicitly track which commands respond on response topics vs state topics to prevent confusion.
4. **Streaming support**: Listening is essential for debugging automation; it should be a first-class verb.

## Alternatives Considered

- **Pure Cobra + manual JSON**: More boilerplate and inconsistent output formatting.
- **TUI only**: Not scriptable; hard to integrate with automation.
- **Direct Zigbee stack**: Too complex; Zigbee2MQTT already provides a stable MQTT API.

## Implementation Plan

1. Bootstrap CLI with Glazed schemas + Cobra registration, config loader, and a shared MQTT client package.
2. Create a custom Zigbee Glazed layer (broker, base-topic, tls, cafile, cert, key, qos, timeout) and include it in every command.
3. Establish repo layout: one directory per group, one file per verb, and root.go per group to register commands.
4. Implement `bridge` and `device` commands with LongDescription examples for each verb.
5. Add `listen` streaming infrastructure (shared subscription code) and LongDescription examples.
6. Implement `group`, `mqtt`, `export`, and `health` commands, each with LongDescription examples.
7. Add tests for:
   - Request/response timeouts
   - Topic selection correctness
   - JSON parsing for state payloads
8. Document usage with examples and manpage-like help (LongDescription already required per command).

## Open Questions

- Confirm which bridge commands reliably respond via `bridge/response/*` in current Zigbee2MQTT versions.
- Decide on MQTT client library (paho vs others).
- Determine whether to store state cache locally for faster queries.

## Related

- `reference/03-zigbee2mqtt-mqtt-command-compendium.md`
- `reference/05-bridge-request-verification-report-step-9.md`
- `/tmp/glaze-build-first-command.txt` (glaze help output used for patterns)
