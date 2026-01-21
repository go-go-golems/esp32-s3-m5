---
Title: 'Playbook: Flash 0049 node + drive LED cues from host'
Ticket: 0057-MLED-NODE-LED
Status: active
Topics:
    - esp32c6
    - esp-idf
    - animation
    - wifi
    - console
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:37:51.991262062-05:00
WhatFor: ""
WhenToUse: ""
---

# Playbook: Flash 0049 node + drive LED cues from host

## Purpose

Flash the `0049-xiao-esp32c6-mled-node` firmware to a XIAO ESP32-C6, provision Wi‑Fi, and drive LED patterns from the host using the MLED/1 UDP multicast protocol (PING/PONG discovery, BEACON time sync, CUE_PREPARE + ACK, CUE_FIRE execution).

This playbook is intended to validate the **protocol → pattern mapping** once `0057-MLED-NODE-LED` is implemented. Until then, cue application is *log-only* (you will see `APPLY cue=...` in logs but no LED change).

## Environment Assumptions

- ESP‑IDF: `~/esp/esp-idf-5.4.1`
- Hardware: XIAO ESP32C6 with WS281x LEDs connected (same wiring as `0044`/`0043`), connected over USB (`/dev/ttyACM0`)
- Network: host and device on the same LAN segment (multicast TTL=1; guest SSIDs often block this)
- Python: `python3` available on the host

## Commands

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5

# Build
idf.py -C 0049-xiao-esp32c6-mled-node set-target esp32c6
idf.py -C 0049-xiao-esp32c6-mled-node build

# Flash + monitor
idf.py -C 0049-xiao-esp32c6-mled-node -p /dev/ttyACM0 flash monitor
```

### Provision Wi‑Fi (device console)

At the `c6>` prompt:

```text
wifi scan
wifi join 0 <password> --save
wifi status
```

### Host protocol tests

In another terminal:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0049-xiao-esp32c6-mled-node

# 1) Basic discovery
python3 tools/mled_ping.py --timeout 2.0 --repeat 3

# 2) Time sync + cue scheduling (currently uses a RAINBOW PatternConfig)
python3 tools/mled_smoke.py --timeout 2.0 --cue 42 --delay-ms 800
```

## Exit Criteria

- Device connects to Wi‑Fi and starts the node loop (monitor shows join + multicast startup).
- Host `mled_ping.py` prints at least one `pong ...`.
- Host `mled_smoke.py` prints:
  - `found node_id=...`
  - `sent CUE_PREPARE ... (awaiting ACK)` then `got ACK ...`
  - and later a `sent CUE_FIRE ...`
- Device monitor logs an `APPLY cue=...` for the fired cue.
- After `0057` is implemented: the LEDs visibly switch to the expected pattern for the sent `PatternConfig` (currently `mled_smoke.py` sends a RAINBOW pattern).

## Notes

- If you get no PONGs, verify multicast is allowed on your network and that the host firewall allows inbound UDP replies.
- If the device never ACKs CUE_PREPARE, confirm it is receiving BEACONs (time sync) and that `--node-id` (if used) matches the discovered ID.
