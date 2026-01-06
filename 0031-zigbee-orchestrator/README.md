# 0031: Zigbee orchestrator (Cardputer host)

Firmware project for ticket `0031-ZIGBEE-ORCHESTRATOR`.

Goal: a real Zigbee coordinator-style “orchestrator” architecture:

- Cardputer (ESP32‑S3) host: Wi‑Fi + HTTP + WebSocket + internal `esp_event` bus.
- Unit Gateway H2 (ESP32‑H2) NCP: Zigbee stack/radio.
- Async HTTP contract: action endpoints return `202 Accepted` with `req_id`; results stream over WS.

Ticket docs (design/analysis/tasks/diary):
`ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/`

## Status

Implemented so far (Phase 1–2):

- USB‑Serial/JTAG `esp_console` REPL (`gw>`)
- Application `esp_event` bus with a stub `GW_CMD_PERMIT_JOIN` command and `GW_EVT_CMD_RESULT`
- Console commands:
  - `version`
  - `monitor on|off|status`
  - `gw status`
  - `gw post permit_join <seconds> [req_id]`
  - `gw demo start|stop`

Wi‑Fi / HTTP / WS / Zigbee integration are next phases (see ticket `tasks.md`).

## Build / flash / monitor (Phase 1–2)

```bash
cd 0031-zigbee-orchestrator
idf.py build
idf.py -p /dev/ttyACM* flash monitor
```

Example REPL session:

```text
gw> help
gw> version
gw> monitor on
gw> gw post permit_join 5
gw> gw demo start
gw> gw demo stop
```
