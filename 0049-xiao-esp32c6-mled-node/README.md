# ESP32-C6 (XIAO) 0049 — MLED/1 Node (UDP multicast)

This firmware turns an **ESP32-C6** into an **MLED/1 node** on your LAN:

- Wi‑Fi provisioning via `esp_console` (USB Serial/JTAG by default)
- UDP multicast listener on `239.255.32.6:4626`
- Discovery/status via `PING` → `PONG` (unicast reply)
- Two-phase cues: `CUE_PREPARE` stores pattern config, `CUE_FIRE` schedules execution at show-time

## Build

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0049-xiao-esp32c6-mled-node

idf.py set-target esp32c6
idf.py build
```

## Flash + Monitor

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

## Provision Wi‑Fi (console)

At the `c6>` prompt:

```text
wifi scan
wifi join 0 mypassword --save
wifi status
```

## Host smoke tests (Python)

See the ticket playbook:

- `ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/playbook/01-playbook-flash-esp32-c6-node-python-smoke-test.md`

