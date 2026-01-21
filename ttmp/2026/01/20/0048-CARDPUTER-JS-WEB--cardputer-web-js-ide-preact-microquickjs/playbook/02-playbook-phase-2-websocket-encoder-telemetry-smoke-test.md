---
Title: 'Playbook: Phase 2 WebSocket encoder telemetry smoke test'
Ticket: 0048-CARDPUTER-JS-WEB
Status: active
Topics:
    - cardputer
    - esp-idf
    - webserver
    - http
    - rest
    - websocket
    - preact
    - zustand
    - javascript
    - quickjs
    - microquickjs
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp
      Note: Reference WS endpoint and broadcast helpers
    - Path: esp32-s3-m5/ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/playbook/01-websocket-over-wi-fi-esp-idf-playbook.md
      Note: WS debugging playbook for failure mode triage
    - Path: esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/design-doc/02-phase-2-design-encoder-position-click-over-websocket.md
      Note: Phase 2 message schema and WS broadcast model validated by this playbook
ExternalSources: []
Summary: 'Repeatable smoke-test procedure for Phase 2: verify `/ws` WebSocket streams encoder telemetry (pos + click) to both a CLI WS client and the browser UI, including reconnect behavior.'
LastUpdated: 2026-01-20T23:09:00.157509777-05:00
WhatFor: A command-oriented checklist to validate Phase 2 WebSocket telemetry end-to-end and to debug the typical failure modes (no upgrades, stale clients, reconnect loops).
WhenToUse: Use after implementing Phase 2 WS encoder streaming, or when changing encoder driver / WS broadcast code.
---


# Playbook: Phase 2 WebSocket encoder telemetry smoke test

## Purpose

Validate that encoder telemetry is streamed over WebSocket:

- rotating the encoder changes `pos` and/or `delta` in messages
- clicking the encoder changes `pressed` (or produces an edge event, depending on implementation)
- browser UI displays these values and stays correct across reconnects

## Environment Assumptions

- Phase 1 firmware exists and serves the UI and `/ws` endpoint (Phase 2 builds on it).
- Encoder hardware is attached and initialized (exact hardware may vary):
  - built-in rotary encoder, or
  - external Chain Encoder (U207) over Grove UART (see MO-036 for protocol assumptions).
- You can determine the device IP (`<ip>`) from serial logs.
- Optional: you have a WS client installed on the host machine:
  - `websocat` or `wscat` (either works).

## Commands

```bash
# 0) Flash + monitor firmware (same as Phase 1)
cd esp32-s3-m5/0048-cardputer-js-web
idf.py flash monitor

# 1) From host: verify the WS upgrade works and messages arrive (choose one)

# Option A: websocat
websocat "ws://<ip>/ws"

# Option B: wscat (node-based)
wscat -c "ws://<ip>/ws"
```

## Exit Criteria

When the WS client connects:

- the connection upgrades (no HTTP 404/405)
- you see JSON frames of the form:

```json
{"type":"encoder","seq":1,"ts_ms":123,"pos":0,"delta":0,"pressed":false}
```

When you rotate the encoder:

- `pos` changes monotonically by small increments/decrements
- `seq` increases
- `delta` reflects the direction (+/−) if included

When you click the encoder:

- `pressed` toggles or an explicit click edge is observed (depending on schema)

In the browser UI (visit `http://<ip>/`):

- a “WS connected” indicator is shown (or equivalent)
- `pos` and `pressed` update in realtime as you rotate/click

Reconnect test:

- close the WS client / reload the browser tab
- WS reconnects and updates resume without requiring a device reboot

## Notes

### Failure modes and what they mean

- WS client gets `404` or `upgrade rejected`:
  - `/ws` route not registered, or WS support not enabled in `sdkconfig` (`CONFIG_HTTPD_WS_SUPPORT`)
- WS connects but no messages:
  - encoder driver not running, or broadcast loop not sending frames (check logs)
  - coalescing/broadcast interval too large (e.g. only sends on edges)
- Messages arrive but UI doesn’t update:
  - frontend store not parsing `type: "encoder"` correctly
  - browser served stale assets (clear cache; consider cache headers)
