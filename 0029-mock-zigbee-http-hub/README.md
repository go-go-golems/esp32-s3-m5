# Tutorial 0029: Mock Zigbee HTTP hub (`esp_http_server` + `esp_event`)

Firmware demo that simulates a Zigbee coordinator-style architecture without Zigbee:

- HTTP API posts **commands** into an application `esp_event` bus.
- Virtual devices emit **state/telemetry notifications** on the same bus.
- A WebSocket endpoint streams bus traffic to clients for observability.

## Configure Wi-Fi

This project uses STA mode. Set credentials via:

```bash
cd 0029-mock-zigbee-http-hub
idf.py set-target esp32s3
idf.py menuconfig  # Tutorial 0029: Mock Zigbee HTTP hub
```

Or configure at runtime over the USB Serial/JTAG `esp_console` REPL (credentials persist in NVS):

```text
hub> wifi scan
hub> wifi set <ssid> <password> save
hub> wifi connect
hub> wifi status
```

If you don't see a `hub>` prompt, ensure the console channel is set to USB Serial/JTAG (this project ships `sdkconfig.defaults` for that; existing local `sdkconfig` values can override it).

If you see `esp_console started over UART`, the firmware was built with a UART console backend and the REPL likely won't be interactive over the Cardputer’s USB port (`/dev/ttyACM0`). Fix by:

- `idf.py menuconfig` → enable USB Serial/JTAG console, disable UART console, or
- delete the local `sdkconfig` so `sdkconfig.defaults` can be applied on next configure/build.

To forget saved credentials:

```text
hub> wifi clear
```

Or write credentials into the local `sdkconfig` (gitignored):

```bash
cd 0029-mock-zigbee-http-hub
idf.py set-target esp32s3
WIFI_SSID="..." WIFI_PASSWORD="..." ./scripts/configure_wifi_sdkconfig.sh
```

## Build / flash

```bash
cd 0029-mock-zigbee-http-hub
idf.py build flash monitor
```

## API quickstart

- `GET /` — embedded UI (connects to WS and decodes protobuf events in-browser)
- `GET /v1/health` — plain text
- `WS /v1/events/ws` — protobuf `hub.v1.HubEvent` as binary frames
- **HTTP API is protobuf-only** (`Content-Type: application/x-protobuf`):
  - `GET /v1/devices` → `hub.v1.DeviceList`
  - `GET /v1/devices/{id}` → `hub.v1.Device`
  - `POST /v1/devices` (body: `hub.v1.CmdDeviceAdd`) → `hub.v1.Device`
  - `POST /v1/devices/{id}/set` (body: `hub.v1.CmdDeviceSet`) → `hub.v1.ReplyStatus`
  - `POST /v1/devices/{id}/interview` → `hub.v1.ReplyStatus`
  - `POST /v1/scenes/{id}/trigger` → `hub.v1.ReplyStatus`
  - `POST /v1/debug/seed` — convenience endpoint to generate demo traffic (plain text)

Host-side helper scripts live in the ticket workspace:

- WS stream smoke test: `ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/ws_hub_events.js`
- Protobuf HTTP client: `ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/http_pb_hub.js`
