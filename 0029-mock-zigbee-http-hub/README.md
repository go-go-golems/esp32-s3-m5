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

- `GET /v1/health`
- `GET /v1/devices`
- `POST /v1/devices`
- `POST /v1/devices/{id}/set`
- `POST /v1/devices/{id}/interview`
- `POST /v1/scenes/{id}/trigger`
- WebSocket event stream: `GET /v1/events/ws`
