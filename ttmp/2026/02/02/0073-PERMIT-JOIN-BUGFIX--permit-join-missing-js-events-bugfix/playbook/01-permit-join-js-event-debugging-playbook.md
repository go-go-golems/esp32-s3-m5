---
Title: Permit-Join JS Event Debugging Playbook
Ticket: 0073-PERMIT-JOIN-BUGFIX
Status: active
Topics:
  - zigbee
  - javascript
  - mqtt
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
  - Path: ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/04-permit-join-watch-yaml.js
    Note: JS permit-join watcher
  - Path: ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/05-confirm-run-permit-join.sh
    Note: plz-confirm gated wrapper
  - Path: zigctl/pkg/jsruntime/zigctlmod/watch.go
    Note: Watch stream buffering implementation
Summary: Reproduction and diagnostics playbook for missing JS join events during permit-join.
LastUpdated: 2026-02-02T00:00:00-05:00
WhatFor: Repeatable steps to compare JS watch output with raw MQTT streams.
WhenToUse: Use when join events are not visible in JS scripts.
---

# Permit-Join JS Event Debugging Playbook

## Purpose
Reproduce and diagnose cases where the JS watch stream does not show join events even though Zigbee2MQTT logs indicate activity. The core method is to compare the JS watch output with a raw MQTT subscription during the same permit-join window.

## Environment Assumptions
- Repo: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5`
- Zigbee2MQTT + Mosquitto are running
- `zigctl` builds and runs locally
- You can put the device into pairing mode during the permit-join window

## Commands

### 1) Start raw MQTT capture (terminal A)
```bash
mosquitto_sub -h localhost -t 'zigbee2mqtt/bridge/#' -v
```
Optional: narrow to event channel only
```bash
mosquitto_sub -h localhost -t 'zigbee2mqtt/bridge/event' -v
```

### 2) Start JS permit-join watch (terminal B)
```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ js run \
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/04-permit-join-watch-yaml.js \
  --arg broker=mqtt://localhost:1884 \
  --arg baseTopic=zigbee2mqtt \
  --arg seconds=120 \
  --arg timeout=60s \
  --arg watchTopic=bridge/event
```

For higher visibility (noisy), switch watchTopic:
```bash
--arg watchTopic=bridge/#
```

### 3) Pair the device
- Put the device into pairing mode immediately after permit-join starts.
- Keep it active for at least 10–20 seconds.

## Exit Criteria
- **Success**: Both JS output and mosquitto_sub show join events on `bridge/event`.
- **Mismatch**: mosquitto_sub shows join events but JS does not → likely buffer/drop/timing issue.
- **No events**: neither shows join events → likely device not paired or join not occurring.

## Failure Modes & Fixes
- **JS missing events only**: increase JS buffer size (future fix), narrow to `bridge/event`, or reduce output volume.
- **Both miss events**: verify pairing mode timing and permit-join response, confirm Zigbee2MQTT logs show join.
- **Too noisy**: use `bridge/event` instead of `bridge/#` for smaller payloads.

## Timing Notes
- The JS watch buffer can drop messages under heavy output. Prefer `bridge/event` for diagnostics.
- If using `bridge/#`, expect a burst of `bridge/info` payloads at start-up; consider starting JS watch first and waiting for the burst before pairing.
