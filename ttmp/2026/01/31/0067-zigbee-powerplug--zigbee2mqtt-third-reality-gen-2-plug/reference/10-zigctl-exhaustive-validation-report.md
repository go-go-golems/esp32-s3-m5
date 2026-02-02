---
Title: zigctl exhaustive validation report
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation/40-run-zigctl-exhaustive.sh
      Note: Exhaustive zigctl validation script
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation/zigctl-exhaustive-20260202T014757Z.log
      Note: Exhaustive zigctl evidence log
ExternalSources: []
Summary: Exhaustive test run of all zigctl commands against the local Zigbee2MQTT broker, with evidence logs and comparison to prior mosquitto_pub/sub validation.
LastUpdated: 2026-02-02T20:50:15-05:00
WhatFor: Provide an audit-grade record that every zigctl verb works against the live broker.
WhenToUse: Use when verifying zigctl correctness or comparing new builds to prior broker validation results.
---


# zigctl exhaustive validation report

## Goal
Confirm that **every zigctl command currently implemented** executes successfully against the local Zigbee2MQTT stack, and document behavior with an audit-grade record.

## Context
- Stack is the same as the earlier mosquitto_pub/sub validation (tmux + Docker).
- Host MQTT broker exposed on **1884**.
- Coordinator: Sonoff Zigbee 3.0 USB Dongle Plus at `/dev/ttyUSB0`.
- Base topic: `zigbee2mqtt`.

## Quick Reference

### Scripts (all stored in ticket)
- Start stack (tmux): `scripts/validation/10-start-broker-tmux.sh`
- Exhaustive zigctl run: `scripts/validation/40-run-zigctl-exhaustive.sh`
- Stop stack: `scripts/validation/30-stop-broker-tmux.sh`

### Logs
- Exhaustive zigctl log: `scripts/validation/zigctl-exhaustive-20260202T014757Z.log`

### Run sequence
```bash
scripts/validation/10-start-broker-tmux.sh
scripts/validation/40-run-zigctl-exhaustive.sh
scripts/validation/30-stop-broker-tmux.sh
```

---

# OSHA-Style Inspection Report

## Inspection Summary
- **Inspection date/time (UTC):** 2026-02-02 01:47Z–01:49Z
- **Scope:** All zigctl commands (bridge, mqtt, listen) + sanity checks of broker state.
- **Result:** All zigctl commands executed successfully. Streaming commands terminated via timeout as expected.

## Environment & Controls (Observed)
- **Containers up:** `z2m`, `z2m-mosquitto` (Up 9+ minutes).
- **Broker access:** `mqtt://localhost:1884` (host mapping to container 1883).
- **Coordinator:** `/dev/ttyUSB0` inside zigbee2mqtt container.
- **tmux session:** `z2m-test` already running (no restart required).

## Command Inventory (All Executed)
| Command | Purpose | Expected outcome | Observed outcome | Pass/Fail |
| --- | --- | --- | --- | --- |
| `zigctl bridge info` | Fetch bridge metadata | JSON row with bridge info | JSON row with config, coordinator, versions | PASS |
| `zigctl bridge devices` | List devices | JSON row(s) with device list | Coordinator listed | PASS |
| `zigctl bridge permit-join` | Enable joining | JSON ok response | `status: ok` | PASS |
| `zigctl mqtt pub` | Publish to topic | Row confirming publish | Row with topic + message | PASS |
| `zigctl mqtt sub` | Subscribe raw topic | Row received then timeout | Row received; timeout exit | PASS |
| `zigctl listen raw` | Stream all topics | Row(s) received then timeout | Rows received; timeout exit | PASS |
| `zigctl listen state --device test_device` | Stream device state | Row(s) received then timeout | Rows received; timeout exit | PASS |

## Detailed Execution & Evidence

### 1) Bridge commands
- **Info:** returned full bridge info payload, including config + versions.
- **Devices:** returned list with `Coordinator` (expected for empty network).
- **Permit join:** returned `{"status":"ok"}`.

### 2) MQTT pub/sub
- **Publish:** success row with topic `zigbee2mqtt/test/zigctl`.
- **Subscribe:** received one message, then terminated via `timeout` (exit code 124 treated as expected).

### 3) Listen raw
- **Subscription:** `zigbee2mqtt/#`.
- **Stimulus:** published synthetic payload to `zigbee2mqtt/test_device`.
- **Observed:** rows included the injected payload plus bridge topics (definitions/extensions). Timeout termination expected and observed.

### 4) Listen state
- **Subscription:** `zigbee2mqtt/test_device` via `--device test_device`.
- **Stimulus:** published synthetic payload to `zigbee2mqtt/test_device`.
- **Observed:** row with device `test_device`, payload `{state: OFF}`. Timeout termination expected and observed.

## Comparison to prior mosquitto_pub/sub validation
- Prior raw MQTT tests verified `permit_join`, `info`, `devices`, and `definitions` responses.
- This run confirms **zigctl** correctly invokes the same request/response flows and produces consistent outputs for `bridge info/devices/permit-join`.
- The exhaustive run did **not** re-test `bridge/response/health_check` or `bridge/request/logging`; those remain documented in the prior report.

## Deviations / Anomalies
- **None** for zigctl commands themselves.
- Streaming commands were intentionally terminated by `timeout` to make the test deterministic.

## Evidence Artifacts
- `scripts/validation/zigctl-exhaustive-20260202T014757Z.log`
- `scripts/validation/40-run-zigctl-exhaustive.sh`

## Risk Review
- **Low risk:** All commands executed successfully. No crashes, hangs, or incorrect topic routing observed.
- **Residual risk:** Streaming commands depend on external message production; payloads were synthetic rather than device-generated.

## Follow-up Recommendations
- If you want device-specific validation (e.g., Third Reality plug state toggles), confirm the device is powered and paired, then re-run `zigctl listen state` and `zigctl mqtt pub` against the device topic. (Use the `plz-confirm` flow only if you want a physical device verification run.)

---

## Appendix: Raw Command Inventory

```bash
# Exhaustive zigctl run
scripts/validation/40-run-zigctl-exhaustive.sh
```
