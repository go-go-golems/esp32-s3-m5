---
Title: JS Debugging Playbook
Ticket: 0070-ZIGBEE-JS-RUNTIME
Status: active
Topics:
  - zigbee
  - javascript
  - goja
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
  - Path: zigctl/cmd/js/run.go
    Note: zigctl JS runner
  - Path: zigctl/pkg/jsruntime/zigctlmod/zigctlmod.go
    Note: JS API surface
  - Path: ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/03-jsruntime-logging-test.js
    Note: Logging test script
  - Path: ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/04-permit-join-watch-yaml.js
    Note: Permit-join watcher
  - Path: ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/05-confirm-run-permit-join.sh
    Note: plz-confirm gated wrapper
Summary: Playbook for running zigctl JS scripts, passing args, and debugging Zigbee2MQTT with the JS runtime.
LastUpdated: 2026-02-02T00:00:00-05:00
WhatFor: Repeatable, operator-friendly JS debugging workflows.
WhenToUse: Use before running permit-join or JS-based diagnostics.
---

# JS Debugging Playbook

## Purpose
Run zigctl JS scripts to debug Zigbee2MQTT behavior, including permit-join flows and live event streams. This playbook shows how to invoke the JS runtime, pass arguments, and select the appropriate watch topics.

## Environment Assumptions
- You are in: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5`
- Zigbee2MQTT + Mosquitto are running (see tmux runbook if needed)
- `zigctl` builds locally (`go run ./ js run ...` works)

## Scripts Available
- `scripts/03-jsruntime-logging-test.js` — sanity check: prints bridge info/devices and a short event stream.
- `scripts/04-permit-join-watch-yaml.js` — opens permit-join + streams MQTT events.
- `scripts/05-confirm-run-permit-join.sh` — plz-confirm gated wrapper for permit-join.

## Command Patterns

### 1) Run a JS script (basic)
```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ js run \
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/03-jsruntime-logging-test.js
```

### 2) Run permit-join watcher (recommended flags)
Use key=value args to avoid positional confusion:
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

### 3) High-noise debugging (bridge/#)
If you need bridge logs, info, and state, use `bridge/#`:
```bash
go run ./ js run \
  /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/04-permit-join-watch-yaml.js \
  --arg broker=mqtt://localhost:1884 \
  --arg baseTopic=zigbee2mqtt \
  --arg seconds=120 \
  --arg timeout=60s \
  --arg watchTopic=bridge/#
```

### 4) Operator-gated permit-join (plz-confirm)
```bash
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/02/01/0070-ZIGBEE-JS-RUNTIME--zigbee-js-runtime/scripts/05-confirm-run-permit-join.sh
```

## Passing Args (JS runtime)
- `zigctl js run` passes args as `--arg key=value` to `zigctlArgs` inside JS.
- Recommended parameters:
  - `broker` (e.g., `mqtt://localhost:1884`)
  - `baseTopic` (default `zigbee2mqtt`)
  - `seconds` (permit-join window)
  - `timeout` (request timeout, e.g., `60s`)
  - `watchTopic` (`bridge/event` or `bridge/#`)

## Exit Criteria
- Permit-join response returns `status: ok`.
- Event stream shows messages on the selected watch topic.
- If a device is paired, you see join/announce events on `bridge/event`.

## Failure Modes & Fixes
- **No join events**: confirm device pairing mode, use operator-gated flow, or compare with `mosquitto_sub`.
- **Timeouts**: raise `timeout` to match your `seconds` window.
- **Too noisy**: switch from `bridge/#` to `bridge/event`.
- **No output**: verify Zigbee2MQTT is running and MQTT broker address matches.
