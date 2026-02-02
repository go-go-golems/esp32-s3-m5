---
Title: Zigbee2MQTT Third Reality Gen 2 plug quickstart
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Quickstart for running Zigbee2MQTT with a Third Reality Gen 2 smart plug (3RSP02028BZ) using Docker Compose, plus pairing and MQTT control examples."
LastUpdated: 2026-01-31T11:31:18-05:00
WhatFor: "Provide copy/paste-ready setup and control steps for Zigbee2MQTT + Third Reality Gen 2 smart plug."
WhenToUse: "Use when setting up Zigbee2MQTT on plain Linux/Docker or when you need MQTT commands for the 3RSP02028BZ plug."
---

# Zigbee2MQTT Third Reality Gen 2 plug quickstart

## Goal

Provide a practical, copy/paste-ready setup for Zigbee2MQTT on plain Linux (Docker Compose), then pair and control the Third Reality Gen 2 smart plug (3RSP02028BZ) over MQTT.

## Context

You need three things: a Zigbee coordinator (USB dongle or network adapter), an MQTT broker (Mosquitto is common), and Zigbee2MQTT (service + web UI). This reference focuses on a plain Linux/Docker setup and the specific device commands for the Third Reality Gen 2 plug.

## Quick Reference

### Requirements

1. Zigbee coordinator (USB dongle or network adapter)
2. MQTT broker (Mosquitto)
3. Zigbee2MQTT

### Docker Compose setup (plain Linux)

#### 1) Find your coordinator serial device

Zigbee2MQTT recommends using the stable `/dev/serial/by-id/...` path.

```bash
ls -l /dev/serial/by-id
```

#### 2) Create `docker-compose.yml`

```yaml
services:
  mosquitto:
    image: eclipse-mosquitto:2
    container_name: mosquitto
    restart: unless-stopped
    ports:
      - "1883:1883"
    volumes:
      - ./mosquitto:/mosquitto

  zigbee2mqtt:
    image: ghcr.io/koenkk/zigbee2mqtt
    container_name: zigbee2mqtt
    restart: unless-stopped
    ports:
      - "8080:8080"
    volumes:
      - ./data:/app/data
      - /run/udev:/run/udev:ro
    environment:
      - TZ=America/New_York
    devices:
      # Replace with YOUR /dev/serial/by-id path:
      - /dev/serial/by-id/usb-XXXXXX:/dev/ttyACM0
```

#### 3) Create `data/configuration.yaml`

Minimal working config:

```yaml
frontend: true

mqtt:
  server: mqtt://mosquitto:1883

serial:
  port: /dev/ttyACM0
  # If autodetect fails, set adapter explicitly (zstack/ember/ezsp/etc).
```

If Zigbee2MQTT cannot auto-detect your adapter, set `serial.adapter` and the correct port explicitly.

#### 4) Start it

```bash
docker compose up -d
```

Open the Zigbee2MQTT UI: `http://localhost:8080`

### Pair the Third Reality Gen 2 plug

1. In Zigbee2MQTT UI: **Permit join**.
2. Plug in the smart plug.
3. Hold the button for more than 10 seconds until the LED flashes (pairing mode).
4. It should appear in Zigbee2MQTT; rename it to something like `office_plug`.

### Zigbee2MQTT MQTT topics

- `zigbee2mqtt/<FRIENDLY_NAME>/set` (send commands)
- `zigbee2mqtt/<FRIENDLY_NAME>` (state updates)
- `zigbee2mqtt/<FRIENDLY_NAME>/get` (request a value)

### Watch messages in a terminal

```bash
mosquitto_sub -h localhost -t 'zigbee2mqtt/#' -v
```

### Turn ON / OFF

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/office_plug/set' -m '{"state":"ON"}'
mosquitto_pub -h localhost -t 'zigbee2mqtt/office_plug/set' -m '{"state":"OFF"}'
```

### Set power-on behavior

Values: `on`, `off`, `previous`.

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/office_plug/set' -m '{"power_on_behavior":"previous"}'
```

### Countdown timers

```bash
# OFF -> ON in 60s
mosquitto_pub -h localhost -t 'zigbee2mqtt/office_plug/set' -m '{"countdown_to_turn_on":60}'

# ON -> OFF in 300s
mosquitto_pub -h localhost -t 'zigbee2mqtt/office_plug/set' -m '{"countdown_to_turn_off":300}'
```

### Reset total energy counter

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/office_plug/set' -m '{"reset_total_energy":"Reset"}'
```

### Common fixes (won't start / won't pair)

- Wrong serial port path: use `/dev/serial/by-id/...` (stable) instead of `/dev/ttyACM0` if it changes.
- Adapter not detected: set `serial.adapter` explicitly and confirm the port.
- USB permission issues (Docker): add group permissions or use rootless config per Zigbee2MQTT Docker guidance.

## Usage Examples

### Example: end-to-end quick test

1. Start Zigbee2MQTT and Mosquitto via Docker Compose.
2. Pair the plug and rename it `office_plug`.
3. Monitor state updates:

```bash
mosquitto_sub -h localhost -t 'zigbee2mqtt/office_plug' -v
```

4. Toggle the relay:

```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/office_plug/set' -m '{"state":"ON"}'
```

## Related

- Zigbee2MQTT adapter settings: https://www.zigbee2mqtt.io/guide/configuration/adapter-settings.html
- Zigbee2MQTT Docker install: https://www.zigbee2mqtt.io/guide/installation/02_docker.html
- Third Reality 3RSP02028BZ device page: https://www.zigbee2mqtt.io/devices/3RSP02028BZ.html
