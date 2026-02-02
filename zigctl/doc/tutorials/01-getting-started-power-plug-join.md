---
Title: Getting started with zigctl + joining a power plug
Slug: zigctl-getting-started-power-plug-join
Short: "Step-by-step tutorial for first-time users: bring up the stack, run zigctl, and confirm a smart plug joins your network."
Topics:
  - zigbee
  - zigctl
  - onboarding
Commands:
  - bridge
  - listen
  - mqtt
Flags:
  - --broker
  - --base-topic
SectionType: Tutorial
IsTopLevel: true
ShowPerDefault: true
Order: 10
---

This tutorial is written for first-time Zigbee users. It shows how to bring up Zigbee2MQTT, use zigctl, and confirm a power plug has joined the network successfully. You do **not** need to understand Zigbee internals to follow it.

## What you are about to do

You will:
- Start the broker and Zigbee2MQTT stack.
- Verify zigctl can talk to the broker.
- Enable joining for a short window.
- Put the plug into pairing mode.
- Confirm the new device appears in the device list.

> **Fundamental:** Zigbee2MQTT translates Zigbee radio traffic into MQTT topics. zigctl talks to those MQTT topics.

## 1) Start the broker + Zigbee2MQTT stack

```bash
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/
  ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/
  scripts/validation/10-start-broker-tmux.sh
```

Expected outcome:
- Containers `z2m` and `z2m-mosquitto` are **Up**.

Verify:

```bash
docker ps --format 'table {{.Names}}\t{{.Image}}\t{{.Status}}'
```

## 2) Verify zigctl can reach the broker

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ bridge info \
  --broker mqtt://localhost:1884 \
  --base-topic zigbee2mqtt \
  --output json
```

Expected outcome:
- JSON output with Zigbee2MQTT version and coordinator info.

## 3) Check the current device list

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ bridge devices \
  --broker mqtt://localhost:1884 \
  --base-topic zigbee2mqtt \
  --output json
```

Expected outcome:
- A list containing at least **Coordinator**.

## 4) Enable permit-join (pairing window)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ bridge permit-join \
  --broker mqtt://localhost:1884 \
  --base-topic zigbee2mqtt \
  --seconds 60 \
  --output json
```

Expected outcome:
- JSON response with `status: ok`.

> **Tip:** Pairing is time-sensitive. Put the plug into pairing mode immediately after this step.

If you want zigctl to wait and report join events automatically, add `--watch`:

```bash
go run ./ bridge permit-join \
  --broker mqtt://localhost:1884 \
  --base-topic zigbee2mqtt \
  --seconds 60 \
  --watch \
  --output json
```

## 5) Put the plug into pairing mode

For a Third Reality Gen 2 plug:
- Plug it in.
- Hold the button for **>10 seconds** until the LED flashes.

## 6) Watch the join event (optional but recommended)

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

timeout 60s go run ./ listen raw \
  --broker mqtt://localhost:1884 \
  --topic 'zigbee2mqtt/#' \
  --output json
```

Expected outcome:
- Messages appear, including a topic like `zigbee2mqtt/<new_device_name>`.

> **Fundamental:** Zigbee devices are represented by their MQTT topic name. That name becomes your control target.

## 7) Confirm the device list updated

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ bridge devices \
  --broker mqtt://localhost:1884 \
  --base-topic zigbee2mqtt \
  --output json
```

Expected outcome:
- The new plug appears with a `friendly_name`. That name becomes the topic you control.

## 8) Quick control test (optional)

If the device is named `office_plug`:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ mqtt pub \
  --broker mqtt://localhost:1884 \
  --topic 'zigbee2mqtt/office_plug/set' \
  --message '{"state":"ON"}'
```

Expected outcome:
- The plug turns on.

## Exit criteria
- `zigctl bridge info` returns a valid payload.
- `zigctl bridge devices` lists the plug.
- Optional: the plug responds to `mqtt pub` control.

## Troubleshooting

- **No join event:** retry permit-join, then re-enter pairing mode.
- **No device list update:** wait 30–60 seconds and re-run `bridge devices`.
- **Broker errors:** confirm you are using port **1884** on the host.
