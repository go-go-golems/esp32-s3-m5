---
Title: 'Playbook: Flash ESP32-C6 node + Python smoke test'
Ticket: 0049-NODE-PROTOCOL
Status: active
Topics:
    - esp32c6
    - wifi
    - console
    - esp-idf
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "How to flash the 0049 ESP32-C6 MLED node firmware, provision Wi‑Fi via esp_console, and validate discovery via a Python PING→PONG smoke test."
LastUpdated: 2026-01-21T15:07:22.948999524-05:00
WhatFor: "Provide a deterministic test loop to validate the MLED/1 node firmware on real hardware (flash + join Wi‑Fi + ping discovery)."
WhenToUse: "Use after each protocol milestone to verify the node still joins multicast and responds with PONG status."
---

# Playbook: Flash ESP32-C6 node + Python smoke test

## Purpose

Flash the ESP32‑C6 firmware (`0049-xiao-esp32c6-mled-node`), provision Wi‑Fi credentials at runtime via `esp_console`, and validate that the device presents itself as a network node by responding to multicast `PING` with unicast `PONG`.

## When you can start testing

You can start testing as soon as `idf.py -C 0049-xiao-esp32c6-mled-node build` succeeds on your machine.

As of `esp32-s3-m5` commit `28eada6` (2026-01-21), this project builds under ESP‑IDF 5.4.1 and supports the PING→PONG smoke test.
As of commit `44e8e42`, the host tooling also includes a BEACON+PREPARE+FIRE two-phase cue smoke script.

The “minimum viable test” is:
- flash + monitor,
- connect the device to Wi‑Fi (`wifi join ...`),
- run `tools/mled_ping.py` on a host on the same LAN and see at least one `pong` line printed.

## Environment Assumptions

- Hardware: Seeed Studio **XIAO ESP32C6** connected over USB.
- ESP‑IDF: `~/esp/esp-idf-5.4.1` installed and usable.
- Console backend: USB Serial/JTAG (default in `sdkconfig.defaults`).
- Host and device are on the same L2 segment (multicast TTL=1; some guest networks block multicast).
- Python: `python3` available (std-lib only; no pip deps).

## Commands

### 0) Build firmware

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5

# One-time per project (safe to re-run)
idf.py -C 0049-xiao-esp32c6-mled-node set-target esp32c6

# Build
idf.py -C 0049-xiao-esp32c6-mled-node build
```

### 1) Flash + monitor

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5

idf.py -C 0049-xiao-esp32c6-mled-node -p /dev/ttyACM0 flash monitor
```

Expected logs include:
- `esp_console started over USB Serial/JTAG ...`
- after Wi‑Fi connect: `STA IP: ...`
- after got-IP: `node started: id=... name=... group=239.255.32.6:4626`

### 2) Provision Wi‑Fi in the REPL

At the `c6>` prompt:

```text
wifi scan
wifi join 0 <password> --save
wifi status
```

### 3) Run Python `PING` smoke test (host-side)

In a second terminal (on the same LAN):

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0049-xiao-esp32c6-mled-node
python3 tools/mled_ping.py --timeout 2.0 --repeat 2
```

Expected output includes at least one:

```text
pong from <ip>:4626 node_id=<id> name='c6-node' ...
```

If you see no PONGs:
- confirm `wifi status` shows `CONNECTED` and an IP
- confirm the host is on the same SSID/VLAN (guest networks commonly block multicast)
- try increasing `--timeout` to `5.0`
- try running the python script from a different machine on the same Wi‑Fi

### 4) Run “two-phase cue” smoke test (BEACON + PREPARE + scheduled FIRE)

This validates:
- `BEACON` epoch + coarse time sync
- optional `TIME_REQ/TIME_RESP` refinement
- `CUE_PREPARE` store + `ACK`
- `CUE_FIRE` scheduling + `APPLY cue=...` log on the device

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0049-xiao-esp32c6-mled-node
python3 tools/mled_smoke.py --timeout 2.0 --cue 42 --delay-ms 800
```

Expected:
- On the host: `found node_id=...`, `sent CUE_PREPARE ...`, `got ACK ...`, `sent CUE_FIRE ...`, `post-fire status: ... cue=42`
- On the device monitor: log lines containing:
  - `CUE_PREPARE cue=42 ...`
  - `CUE_FIRE cue=42 execute_at=...`
  - `APPLY cue=42 ...`

## Exit Criteria

- Firmware builds locally (`idf.py build` succeeds).
- Device joins Wi‑Fi and prints a valid STA IP.
- Host-side `tools/mled_ping.py` prints at least one `pong ...` line.

## Notes

- The cue smoke test uses a host-generated show clock based on `time.monotonic()`; it’s sufficient for correctness testing, not for “performance grade” sync characterization.
- The node currently applies cues in a “log-only” mode (it updates PONG state and prints `APPLY cue=...`). Hardware LED integration is a follow-up task.
