---
Title: Zigbee2MQTT validation report (broker + zigctl)
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation/10-start-broker-tmux.sh
      Note: Script to start broker in tmux
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation/20-run-bridge-tests.sh
      Note: Bridge mosquitto_pub/sub validation script
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation/25-run-zigctl-tests.sh
      Note: zigctl validation script
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation/30-stop-broker-tmux.sh
      Note: Script to stop broker and tmux
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation/bridge-validation-20260202T013834Z.log
      Note: Initial timeout log
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation/bridge-validation-20260202T014228Z.log
      Note: Successful bridge validation log
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/validation/zigctl-validation-20260202T014315Z.log
      Note: zigctl validation log
ExternalSources: []
Summary: Detailed validation run for Mosquitto + Zigbee2MQTT (tmux/Docker) plus zigctl CLI checks, with comparisons to prior mosquitto_pub/sub tests and full command/log inventory.
LastUpdated: 2026-02-02T20:45:30-05:00
WhatFor: Provide an OSHA-level validation record for broker bring-up and MQTT/zigctl behavior on this host.
WhenToUse: Use when you need reproducible evidence of bridge request behavior and zigctl command correctness.
---


# Zigbee2MQTT validation report (broker + zigctl)

## Goal
Validate the Mosquitto + Zigbee2MQTT stack using the existing tmux/Docker playbook, then test zigctl commands against the live broker, and compare results to the prior mosquitto_pub/sub validation run.

## Context
- Host: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5`
- Playbook: `playbook/02-bridge-request-response-validation-tmux-docker.md`
- Compose: `scripts/zigbee2mqtt-test/docker-compose.yml`
- Coordinator: Sonoff Zigbee 3.0 USB Dongle Plus at `/dev/serial/by-id/.../dev/ttyUSB0`
- Host broker port: `1884` (port 1883 conflict documented in earlier run)

## Quick Reference

### Scripts (all stored in ticket)
- Start stack: `scripts/validation/10-start-broker-tmux.sh`
- Bridge tests (mosquitto_pub/sub): `scripts/validation/20-run-bridge-tests.sh`
- Zigctl tests: `scripts/validation/25-run-zigctl-tests.sh`
- Stop stack: `scripts/validation/30-stop-broker-tmux.sh`

### Logs from this run
- `scripts/validation/bridge-validation-20260202T013834Z.log` (initial attempt, timed out)
- `scripts/validation/bridge-validation-20260202T014228Z.log` (successful mosquitto_pub/sub run)
- `scripts/validation/zigctl-validation-20260202T014315Z.log` (zigctl validation)

### Commands executed (high-level)
- `tmux new-session -d -s z2m-test ...` (start Mosquitto + Zigbee2MQTT)
- `mosquitto_pub/sub` tests against `localhost:1884`
- `go run ./` zigctl commands with `--broker mqtt://localhost:1884`

## Usage Examples

```bash
# Start services
scripts/validation/10-start-broker-tmux.sh

# Run bridge validation via mosquitto_pub/sub
scripts/validation/20-run-bridge-tests.sh

# Run zigctl validation
scripts/validation/25-run-zigctl-tests.sh

# Stop services
scripts/validation/30-stop-broker-tmux.sh
```

---

# OSHA-Style Inspection Report

## Inspection Summary
- **Inspection date/time (UTC):** 2026-02-02 01:38Z–01:45Z
- **Inspection scope:** Broker bring-up, bridge request/response behavior, zigctl CLI behavior, and variance analysis vs prior run.
- **Result:** Stack brought up successfully; all primary bridge requests responded; zigctl commands worked; one behavioral delta observed (health_check now responds).

## Environment & Controls (Observed)
- **Containers up:** `z2m` (zigbee2mqtt) and `z2m-mosquitto` (mosquitto). Status: Up (3+ minutes).
- **Broker access:** Host port `1884` (mapped to container `1883`).
- **Coordinator:** `/dev/ttyUSB0` exposed to Zigbee2MQTT container.
- **Start method:** tmux session `z2m-test` with two panes per playbook.

## Detailed Steps & Observations

### 1) Bring-up (tmux + Docker)
- **Script:** `scripts/validation/10-start-broker-tmux.sh`
- **Expected:** Two containers running in tmux panes.
- **Observed:** `tmux z2m-test running`. `docker ps` showed both `z2m` and `z2m-mosquitto` containers up.
- **Deviation:** None.

### 2) Initial mosquitto_pub/sub run (timed out)
- **Script:** `scripts/validation/20-run-bridge-tests.sh` (first version)
- **Log:** `bridge-validation-20260202T013834Z.log`
- **Observed:** Script hung at `mqtt sanity` step due to unbounded `mosquitto_sub` wait. Command timed out at 120s.
- **Root cause:** No timeout flags used in `mosquitto_sub`, so it blocked indefinitely if the pub/sub handshake missed timing.
- **Corrective action:** Updated script to use `-W` timeouts and explicit wait handling; reran.

### 3) Mosquitto_pub/sub validation (successful)
- **Script:** `scripts/validation/20-run-bridge-tests.sh` (updated with timeouts)
- **Log:** `bridge-validation-20260202T014228Z.log`
- **Observed responses:**
  - **mqtt sanity** (test topic): Received `hello` → OK.
  - **permit_join**: Response on `bridge/response/permit_join` → OK.
  - **info**: Response on `bridge/info` → OK (large JSON payload with config + versions).
  - **devices**: Response on `bridge/devices` → OK (list contained Coordinator only).
  - **definitions**: Response on `bridge/definitions` → OK (large JSON payload).
  - **health_check**: **Response received** on `bridge/response/health_check` → **Deviation vs prior run.**
  - **logging**: No response within 6s → Expected in this run.

### 4) Zigctl command validation
- **Script:** `scripts/validation/25-run-zigctl-tests.sh`
- **Log:** `zigctl-validation-20260202T014315Z.log`
- **Commands + results:**
  - `zigctl bridge info` → JSON output with full bridge config, network, coordinator info.
  - `zigctl bridge devices` → JSON output listing Coordinator.
  - `zigctl bridge permit-join --seconds 1` → JSON response with `status: ok`.
  - `zigctl mqtt pub/sub` → Published and received message on `zigbee2mqtt/test/zigctl`; sub terminated by timeout (expected in script), with captured row output.
- **Deviation:** None.

## Comparison to Prior Mosquitto Tests (Reference)
- **Prior run (from diary + playbook):**
  - `permit_join`, `info`, `devices`, `definitions` responded correctly.
  - **`health_check` and `logging` produced no response.**
- **This run:**
  - `permit_join`, `info`, `devices`, `definitions` responded correctly (consistent).
  - **`health_check` now responded with `{"data":{"healthy":true},"status":"ok"}` (inconsistent with prior run).**
  - `logging` still produced no response (consistent with prior run).

**Interpretation:** Health check behavior is not stable across runs; likely dependent on Zigbee2MQTT version, configuration, or timing. This should be treated as a conditional response, not guaranteed absent.

## Findings & Risks (OSHA-style)

### Finding 1: Unbounded subscriber waits can stall validation runs
- **Severity:** Medium (availability/automation risk)
- **Evidence:** `bridge-validation-20260202T013834Z.log` hung at mqtt sanity step.
- **Cause:** `mosquitto_sub -C 1` without timeout.
- **Corrective action:** Updated script to use `-W` and detect timeout. Retest successful.

### Finding 2: Health check response behavior is inconsistent across runs
- **Severity:** Low (behavioral variance; no data loss)
- **Evidence:** Prior run: no response. Current run: response on `bridge/response/health_check`.
- **Impact:** Test harness must treat health_check as potentially responding (do not assume no response).
- **Mitigation:** Update expectations to log both possibilities and track response if present.

### Finding 3: Logging response still absent in bridge/request/logging tests
- **Severity:** Low (expected behavior per prior report)
- **Evidence:** No response within 6s in current run; consistent with prior behavior.
- **Impact:** Use `bridge/request/options` for log level changes (already documented in prior report).

## Artifacts & Evidence
- **Scripts:**
  - `scripts/validation/10-start-broker-tmux.sh`
  - `scripts/validation/20-run-bridge-tests.sh`
  - `scripts/validation/25-run-zigctl-tests.sh`
  - `scripts/validation/30-stop-broker-tmux.sh`
- **Logs:**
  - `scripts/validation/bridge-validation-20260202T013834Z.log` (timeout)
  - `scripts/validation/bridge-validation-20260202T014228Z.log` (successful)
  - `scripts/validation/zigctl-validation-20260202T014315Z.log`

## Validation Checklist (Observed)
- [x] Broker running on host port 1884
- [x] Zigbee2MQTT container running and attached to coordinator
- [x] `permit_join` request/response succeeds
- [x] `info` request produces payload
- [x] `devices` request produces payload
- [x] `definitions` request produces payload
- [x] `zigctl` bridge commands return JSON output
- [x] `zigctl` mqtt pub/sub round-trip message
- [x] Variance vs prior run documented

## Deviations & Follow-ups
- **Deviation:** Health check responded in this run (previously absent).
  - **Follow-up suggestion:** If stability matters, add a note to the playbook that health_check may respond and update expected outcomes accordingly.
- **Deviation:** Initial script lacked timeouts; corrected and re-run.

---

## Appendix: Raw Command Inventory

### Broker start
- `scripts/validation/10-start-broker-tmux.sh`

### Mosquitto tests
- `scripts/validation/20-run-bridge-tests.sh`

### Zigctl tests
- `scripts/validation/25-run-zigctl-tests.sh`

### Broker stop
- `scripts/validation/30-stop-broker-tmux.sh`
