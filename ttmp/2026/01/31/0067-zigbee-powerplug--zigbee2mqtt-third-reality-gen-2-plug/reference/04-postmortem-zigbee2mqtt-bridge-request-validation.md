---
Title: 'Postmortem: Zigbee2MQTT bridge request validation'
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Postmortem of the tmux-based Zigbee2MQTT + Mosquitto validation run, including port conflict resolution and bridge request results."
LastUpdated: 2026-01-31T12:20:26-05:00
WhatFor: "Capture outcomes, failures, and lessons from the bridge request/response validation run."
WhenToUse: "Use when repeating the test run or diagnosing why certain bridge requests do not respond." 
---

# Postmortem: Zigbee2MQTT bridge request validation

## Goal

Document the real-world validation run for Zigbee2MQTT bridge requests (tmux + Docker), including the port-conflict failure, the workaround, and the observed request/response behaviors.

## Context

This run used a Sonoff Zigbee 3.0 USB Dongle Plus on `/dev/ttyUSB0` with Zigbee2MQTT 2.7.2. Services were started in tmux with Docker Compose under `/tmp/zigbee2mqtt-test`. The host already had something bound to port 1883, forcing Mosquitto to be re-mapped to host port 1884.

## Quick Reference

### Outcomes Summary

| Request | Request Topic | Response Topic Observed | Result |
| --- | --- | --- | --- |
| Permit join | `zigbee2mqtt/bridge/request/permit_join` | `zigbee2mqtt/bridge/response/permit_join` | **OK** -- `{"data":{"time":1},"status":"ok"}` |
| Info | `zigbee2mqtt/bridge/request/info` | `zigbee2mqtt/bridge/info` | **OK** -- config + metadata payload (version 2.7.2, channel 11) |
| Devices | `zigbee2mqtt/bridge/request/devices` | `zigbee2mqtt/bridge/devices` | **OK** -- Coordinator only |
| Definitions | `zigbee2mqtt/bridge/request/definitions` | `zigbee2mqtt/bridge/definitions` | **OK** -- large JSON definitions payload |
| Health check | `zigbee2mqtt/bridge/request/health_check` | none observed | **No response** (timeout) |
| Logging | `zigbee2mqtt/bridge/request/logging` | none observed | **No response** (timeout) |

### Key Logs (abbreviated)

- Zigbee2MQTT started successfully and connected to MQTT on `mqtt://mosquitto:1883`.
- Coordinator detected as ZStack3x0 with channel 11.
- `bridge/info` request returned a full config schema and runtime snapshot.

## Timeline

- **T0**: tmux session created (`z2m-test`), `docker compose up` launched.
- **T+1m**: Mosquitto failed to bind on 1883 (`address already in use`).
- **T+3m**: Host port mapping switched to `1884:1883`, Mosquitto started.
- **T+5m**: Zigbee2MQTT connected to MQTT and started frontend.
- **T+7m**: Bridge request tests executed (permit_join, info, devices, definitions).
- **T+10m**: health_check/logging requests produced no responses.

## Root Cause

**Primary failure:** Host port 1883 was already bound by another service, preventing Mosquitto from starting.

## Resolution

- Changed Mosquitto host binding to `1884:1883` in `/tmp/zigbee2mqtt-test/docker-compose.yml`.
- Ran all mosquitto_pub/sub tests against port **1884** on localhost.

## Impact

- Delayed validation by ~3-5 minutes due to port reconfiguration.
- No functional impact on Zigbee2MQTT once Mosquitto was reachable.

## What Worked Well

- tmux session with two panes made service logs easy to inspect.
- Zigbee2MQTT successfully resumed, connected to MQTT, and served the frontend.
- Bridge requests returned expected payloads for permit_join, info, devices, and definitions.

## What Didn't Work

- `bridge/request/health_check` produced no response on either `bridge/response/health_check` or `bridge/health`.
- `bridge/request/logging` produced no response on `bridge/response/logging` or `bridge/logging`.

## Corrective / Preventative Actions

- **Add port preflight:** check `ss -ltnp | rg ':1883'` before launching Mosquitto.
- **Document response topics:** note which requests publish to state topics vs response topics.
- **Verify unsupported commands:** confirm health_check/logging support in current Zigbee2MQTT version.

## Open Questions

- Are `health_check` and `logging` request handlers disabled by config or removed in 2.7.2?
- Should the request/response behavior be normalized in docs or explicitly noted as version-dependent?

## Usage Examples

- Use the playbook to reproduce the exact run and compare outputs.
- Reference the outcomes table to select commands that are known to respond on this setup.

## Related

- `playbook/02-bridge-request-response-validation-tmux-docker.md`
- `reference/03-zigbee2mqtt-mqtt-command-compendium.md`
