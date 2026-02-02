---
Title: Bridge request verification report (Step 9)
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Verification of Step 9 bridge request behavior against Zigbee2MQTT docs, with corrections for logging and health check usage."
LastUpdated: 2026-01-31T12:45:35-05:00
WhatFor: "Clarify which Step 9 bridge requests should respond on response topics vs. state topics, and identify the documented commands to use instead."
WhenToUse: "Use when troubleshooting missing bridge responses or validating Zigbee2MQTT MQTT command behavior."
---

# Bridge request verification report (Step 9)

## Goal

Verify the Step 9 "failed" bridge requests against the Zigbee2MQTT documentation and document the correct expectations and fixes.

## Context

In Step 9 of the diary, the following requests were marked as non-responsive:
- `bridge/request/health_check`
- `bridge/request/logging`
- `bridge/request/devices` (no `bridge/response/devices` response observed)

Zigbee2MQTT's MQTT topics page documents both the bridge state topics (`bridge/info`, `bridge/devices`, `bridge/definitions`, `bridge/logging`, `bridge/health`) and the bridge request endpoints (including `health_check` and `options`). It also states that `bridge/request/+` is answered on `bridge/response/+`.

## Quick Reference

### Findings summary

| Step 9 command | Docs say | Likely reason for Step 9 behavior | Correction |
| --- | --- | --- | --- |
| `bridge/request/logging` | No `bridge/request/logging` endpoint is documented. `bridge/logging` is a published log stream topic. Log-level changes are done via `bridge/request/options` (e.g., `advanced.log_level`). | Used a non-documented request topic, so no response. | Use `bridge/request/options` with `{"options": {"advanced": {"log_level": "info"}}}`.
| `bridge/request/health_check` | `bridge/request/health_check` is documented with an empty payload and a response on `bridge/response/health_check`. `bridge/health` is a separate published state topic. | If no response was observed, it may be due to subscribing only to `bridge/health` or missing the response because of timing/timeout. | Subscribe to `bridge/response/health_check` and send an empty payload (use `-n`), and optionally watch `bridge/health` for periodic health info.
| `bridge/request/devices` | `bridge/devices` is a documented state topic that publishes the device inventory (retained). `bridge/request/+` should respond on `bridge/response/+`. | The device list arrived on `bridge/devices`; no response was seen on `bridge/response/devices`. | Subscribe to `bridge/devices` during validation; optionally also watch `bridge/response/devices` in case the request endpoint responds.

## Notes on documented behavior

- **State topics:** `bridge/info`, `bridge/devices`, and `bridge/definitions` are documented as state topics published by Zigbee2MQTT. It is normal to see results there.
- **Request/response rule:** The MQTT topics page says `bridge/request/+` responds on `bridge/response/+` and gives `permit_join` as an example.
- **Logging control:** Runtime log-level changes are documented via `bridge/request/options` with `advanced.log_level`. There is no documented `bridge/request/logging`; `bridge/logging` is an output topic.
- **Health:** Health data is published on `bridge/health` and configured by `health.interval`. This is distinct from the `health_check` request/response.

## Usage Examples

### Correct logging change (runtime)

```bash
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/response/options' -C 1 &
mosquitto_pub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/request/options' -m '{"options":{"advanced":{"log_level":"info"}}}'
wait
```

### Health check validation (response + state)

```bash
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/response/health_check' -C 1 &
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/health' -C 1 &
mosquitto_pub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/request/health_check' -n
wait
```

### Devices validation (response + state)

```bash
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/response/devices' -C 1 &
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/devices' -C 1 &
mosquitto_pub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/request/devices' -m '{}'
wait
```

## References

- https://www.zigbee2mqtt.io/guide/usage/mqtt_topics_and_messages.html
- https://www.zigbee2mqtt.io/guide/usage/health.html
- https://www.zigbee2mqtt.io/guide/configuration/logging.html

## Related

- `reference/04-postmortem-zigbee2mqtt-bridge-request-validation.md`
- `playbook/02-bridge-request-response-validation-tmux-docker.md`
