---
Title: 'Playbook: Validate component in 0049'
Ticket: 0058-MLED-NODE-COMPONENT
Status: active
Topics:
    - esp32c6
    - esp-idf
    - networking
    - protocol
    - tooling
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:40:49.92628535-05:00
WhatFor: "Verify component extraction preserves behavior in the 0049 node tutorial."
WhenToUse: "Use after refactors to `components/mled_node` or when onboarding a new consumer firmware."
---

# Playbook: Validate component in 0049

## Purpose

Verify that extracting the MLED node stack into `components/mled_node` does not change runtime behavior by rebuilding and smoke-testing the `0049-xiao-esp32c6-mled-node` tutorial.

## Environment Assumptions

- ESP‑IDF: `~/esp/esp-idf-5.4.1`
- Hardware: XIAO ESP32C6 connected over USB (`/dev/ttyACM0`)
- Network: host and device on the same LAN segment (multicast TTL=1)
- Python: `python3` available

## Commands

### Build firmware

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
idf.py -C 0049-xiao-esp32c6-mled-node set-target esp32c6
idf.py -C 0049-xiao-esp32c6-mled-node build
```

### Flash + monitor

```bash
idf.py -C 0049-xiao-esp32c6-mled-node -p /dev/ttyACM0 flash monitor
```

### Provision Wi‑Fi

At the `c6>` prompt:

```text
wifi scan
wifi join 0 <password> --save
wifi status
```

### Host smoke tests

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0049-xiao-esp32c6-mled-node
python3 tools/mled_ping.py --timeout 2.0 --repeat 2
python3 tools/mled_smoke.py --timeout 2.0 --cue 42 --delay-ms 800
```

## Exit Criteria

- `idf.py build` succeeds with the componentized code.
- Device connects to Wi‑Fi and starts the node.
- `mled_ping.py` prints at least one `pong ...`.
- `mled_smoke.py` prints `got ACK ...` and the device monitor logs an `APPLY cue=...`.

## Notes

- If PONGs do not arrive, validate that multicast is permitted on your network (guest SSIDs often block it).
