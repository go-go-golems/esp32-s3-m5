---
Title: 'Playbook: validate ws281x component in 0049'
Ticket: 0059-WS281X-COMPONENT
Status: active
Topics:
    - esp32c6
    - esp-idf
    - leds
    - rmt
    - component
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T16:46:39.361550329-05:00
WhatFor: ""
WhenToUse: ""
---

# Playbook: validate ws281x component in 0049

## Purpose

Verify that `components/ws281x` works as a drop-in replacement for the WS281x engine sources previously embedded in `0049-xiao-esp32c6-mled-node`, by building, flashing, and driving all patterns from the host.

## Environment Assumptions

- ESP‑IDF: `~/esp/esp-idf-5.4.1`
- Hardware: XIAO ESP32C6 with WS281x LEDs wired to the configured GPIO (default: D1/GPIO1), connected over USB (`/dev/ttyACM0`)
- Network: host and device on the same LAN segment (multicast TTL=1)
- Python: `python3` available

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

### Host pattern smoke tests

In another terminal:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0049-xiao-esp32c6-mled-node

python3 tools/mled_ping.py --timeout 2.0 --repeat 2
# If you get no PONGs but can ping the device IP, try forcing the multicast egress interface:
# python3 tools/mled_ping.py --bind-ip <your-host-lan-ip> --timeout 2.0 --repeat 2

python3 tools/mled_smoke.py --timeout 2.0 --cue 42 --delay-ms 800 --pattern rainbow
python3 tools/mled_smoke.py --timeout 2.0 --cue 43 --delay-ms 800 --pattern chase --chase-fg '#ff0000' --chase-bg '#000010'
python3 tools/mled_smoke.py --timeout 2.0 --cue 44 --delay-ms 800 --pattern breathing --breathing-color '#00a0ff'
python3 tools/mled_smoke.py --timeout 2.0 --cue 45 --delay-ms 800 --pattern sparkle --sparkle-mode 2 --sparkle-density 15
# Same bind-ip hint applies:
# python3 tools/mled_smoke.py --bind-ip <your-host-lan-ip> ...
```

## Exit Criteria

- `idf.py build` succeeds.
- Device connects to Wi‑Fi and logs `mled_node: started: ...`.
- For each cue, device logs:
  - `mled_node: APPLY cue=...`
  - `mled_effect_led: apply: type=... bri=...%`
- LEDs visibly change between patterns (rainbow/chase/breathing/sparkle).

## Notes

- If you see no LED change but `mled_effect_led: apply` logs, double-check the WS281x wiring (GPIO, power, ground).
