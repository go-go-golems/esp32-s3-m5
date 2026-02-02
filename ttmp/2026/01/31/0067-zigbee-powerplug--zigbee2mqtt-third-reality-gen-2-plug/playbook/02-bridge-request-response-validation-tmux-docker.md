---
Title: Bridge request/response validation (tmux + Docker)
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Step-by-step validation run for Zigbee2MQTT bridge requests using tmux, Docker Compose, and mosquitto_pub/sub."
LastUpdated: 2026-01-31T12:16:48-05:00
WhatFor: "Provide a repeatable procedure to start Mosquitto + Zigbee2MQTT and validate MQTT bridge requests on this host."
WhenToUse: "Use when you need to validate Zigbee2MQTT bridge command behavior or reproduce the postmortem run."
---

# Bridge request/response validation (tmux + Docker)

## Purpose

Run Mosquitto and Zigbee2MQTT in a tmux session, then execute bridge request commands and verify their responses. This playbook mirrors the actual validation run and documents observed response topics.

## Environment Assumptions

- Docker Engine is installed and working.
- tmux is installed.
- Sonoff Zigbee 3.0 USB Dongle Plus is attached (expected at `/dev/ttyUSB0`).
- Zigbee2MQTT test workspace exists at:
  `ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/zigbee2mqtt-test`
- Host port **1883** may be in use; this run uses **1884** for the host broker port.

## Commands

### 1) Start the tmux session

```bash
tmux new-session -d -s z2m-test -c /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/zigbee2mqtt-test

tmux split-window -h -t z2m-test:0 -c /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/zigbee2mqtt-test
```

### 2) Start Mosquitto and Zigbee2MQTT (two panes)

```bash
# Left pane
 tmux send-keys -t z2m-test:0.0 'docker compose up mosquitto' C-m

# Right pane
 tmux send-keys -t z2m-test:0.1 'docker compose up zigbee2mqtt' C-m
```

Expected outcome:
- `z2m-mosquitto` and `z2m` containers reach **Up** state.

### 3) Verify containers are running

```bash
docker ps --format 'table {{.Names}}\t{{.Image}}\t{{.Status}}'
```

Expected rows:
- `z2m-mosquitto` (Up)
- `z2m` (Up)

### 4) Sanity-check MQTT broker on host port 1884

```bash
# Should echo "hello"
mosquitto_sub -h localhost -p 1884 -t 'test' -C 1 &
mosquitto_pub -h localhost -p 1884 -t 'test' -m 'hello'
wait
```

Expected output:
- `hello`

### 5) Bridge request validation (known-good)

**Permit join** (response topic):

```bash
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/response/permit_join' -C 1 &
mosquitto_pub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/request/permit_join' -m '{"time": 1}'
wait
```

Expected output:

```json
{"data":{"time":1},"status":"ok"}
```

**Info** (publishes to state topic):

```bash
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/info' -C 1 &
mosquitto_pub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/request/info' -m '{}'
wait
```

Expected output:
- JSON containing `version`, `network.channel`, and `coordinator` metadata.

**Devices** (publishes to state topic):

```bash
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/devices' -C 1 &
mosquitto_pub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/request/devices' -m '{}'
wait
```

Expected output:
- JSON list; on a fresh setup, only the Coordinator appears.

**Definitions** (publishes to state topic):

```bash
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/definitions' -C 1 &
mosquitto_pub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/request/definitions' -m '{}'
wait
```

Expected output:
- Large JSON payload with cluster/action definitions.

### 6) Known non-responders (documented behavior)

**Health check** (no response observed in this run):

```bash
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/response/health_check' -C 1 &
mosquitto_pub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/request/health_check' -m '{}'
wait
```

**Logging** (no response observed in this run):

```bash
mosquitto_sub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/response/logging' -C 1 &
mosquitto_pub -h localhost -p 1884 -t 'zigbee2mqtt/bridge/request/logging' -m '{"level":"info"}'
wait
```

## Exit Criteria

- Both containers are running in tmux.
- MQTT broker accepts a test message on host port **1884**.
- `permit_join`, `info`, `devices`, and `definitions` produce responses/published payloads.
- Observed non-responses for `health_check` and `logging` are noted.

## Failure Modes and Fixes

- **Mosquitto fails to bind on 1883**: a host service already uses 1883. Change the Compose mapping to `1884:1883` and retry.
- **No coordinator detected**: verify `/dev/ttyUSB0` or update the device path in `data/configuration.yaml`.
- **No MQTT responses**: confirm you are publishing to host port 1884, not 1883.

## Notes

- The Zigbee2MQTT service connects to the internal broker at `mqtt://mosquitto:1883`; the host uses `1884` only for local testing.
- Keep the tmux session open for live log inspection.

## Related

- `reference/04-postmortem-zigbee2mqtt-bridge-request-validation.md`
- `scripts/zigbee2mqtt-test/docker-compose.yml`
