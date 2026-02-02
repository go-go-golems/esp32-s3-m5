---
Title: zigctl getting started + power plug join
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
  - zigbee
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Beginner-friendly zigctl tutorial: broker bring-up, zigctl basics, and verifying a smart plug joins Zigbee2MQTT successfully."
LastUpdated: 2026-02-02T20:55:00-05:00
WhatFor: "Teach a Zigbee newcomer how to use zigctl to verify Zigbee2MQTT and confirm a power plug has joined." 
WhenToUse: "Use when setting up zigctl for the first time or onboarding a new Zigbee device to Zigbee2MQTT."
---

# zigctl Getting Started + Power Plug Join (Beginner Tutorial)

## Purpose
Get a first-time user comfortable with Zigbee and zigctl, then walk through bringing up the broker, running basic zigctl commands, and confirming a power plug has joined the Zigbee2MQTT network.

## Environment Assumptions
- Docker is installed and working.
- `zigctl` lives at: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl`
- Zigbee2MQTT test stack exists at:
  `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/scripts/zigbee2mqtt-test`
- The host broker port is **1884** (1883 is already used on this host).
- Coordinator is connected and accessible inside the container (Sonoff Zigbee 3.0 USB Dongle Plus).

---

## Fundamentals (Short Callouts)

> **What is Zigbee2MQTT?**
> Zigbee2MQTT acts as the bridge between Zigbee radios and MQTT messages. Your Zigbee devices (like a smart plug) don’t speak HTTP—Zigbee2MQTT translates their Zigbee traffic into MQTT topics you can read and control.

> **What is a Zigbee coordinator?**
> The coordinator (USB dongle) is the hub of your Zigbee network. All devices join *through it*, and Zigbee2MQTT depends on it.

> **What is zigctl?**
> zigctl is a CLI that talks to Zigbee2MQTT over MQTT. It’s the “command-line remote control” for your Zigbee network.

---

## Step 1 — Start the broker + Zigbee2MQTT stack

We run Mosquitto and Zigbee2MQTT together. The tmux playbook keeps both services running in panes.

```bash
# Start the stack (tmux)
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/
  ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/
  scripts/validation/10-start-broker-tmux.sh
```

**Expected outcome:**
- `z2m` and `z2m-mosquitto` containers are `Up`.

Verify quickly:

```bash
docker ps --format 'table {{.Names}}\t{{.Image}}\t{{.Status}}'
```

---

## Step 2 — Confirm zigctl can talk to the broker

This checks basic connectivity and confirms Zigbee2MQTT is alive.

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ bridge info \
  --broker mqtt://localhost:1884 \
  --base-topic zigbee2mqtt \
  --output json
```

**Expected outcome:**
- JSON output that includes Zigbee2MQTT version, coordinator info, and configuration.

---

## Step 3 — See the current device list

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ bridge devices \
  --broker mqtt://localhost:1884 \
  --base-topic zigbee2mqtt \
  --output json
```

**Expected outcome:**
- A list of devices. On a fresh network, you’ll usually see only **Coordinator**.

---

## Step 4 — Enable permit-join (pairing window)

This opens a short window where new devices are allowed to join.

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ bridge permit-join \
  --broker mqtt://localhost:1884 \
  --base-topic zigbee2mqtt \
  --seconds 60 \
  --output json
```

**Expected outcome:**
- Response payload with `status: ok` and the permitted time.

> **Tip:** Zigbee joining is time-sensitive. Turn on pairing mode on the plug *right after* you enable permit-join.

If you want zigctl to wait and report join events automatically, add `--watch`:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ bridge permit-join \
  --broker mqtt://localhost:1884 \
  --base-topic zigbee2mqtt \
  --seconds 60 \
  --watch \
  --output json
```

---

## Step 5 — Put the power plug into pairing mode

For a Third Reality Gen 2 plug:
- Plug it in.
- Hold the physical button **for >10 seconds** until the LED flashes.

**What this means:**
- The device is now “advertising” that it wants to join.

---

## Step 6 — Watch the join event with zigctl

Use a listener to see state updates live while the device joins.

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

timeout 60s go run ./ listen raw \
  --broker mqtt://localhost:1884 \
  --topic 'zigbee2mqtt/#' \
  --output json
```

**Expected outcome:**
- Messages appear with topics like:
  - `zigbee2mqtt/bridge/event`
  - `zigbee2mqtt/<new_device_name>`

> **Callout:** The *exact* join messages vary, but you should see at least one new `zigbee2mqtt/<device>` topic shortly after the plug starts pairing.

---

## Step 7 — Confirm the device is now in the device list

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ bridge devices \
  --broker mqtt://localhost:1884 \
  --base-topic zigbee2mqtt \
  --output json
```

**Expected outcome:**
- The new plug appears with a `friendly_name` (Zigbee2MQTT assigns one). That is the name you’ll use in MQTT topics.

---

## Step 8 — Quick sanity control (optional)

If you see a friendly name like `office_plug`, you can test control using raw MQTT.

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/zigctl

go run ./ mqtt pub \
  --broker mqtt://localhost:1884 \
  --topic 'zigbee2mqtt/office_plug/set' \
  --message '{"state":"ON"}'
```

**Expected outcome:**
- Plug turns on. If nothing happens, re-check the device name from the device list.

---

## Exit Criteria (Success Indicators)
- `zigctl bridge info` returns a valid JSON payload.
- `zigctl bridge devices` lists a new plug (in addition to Coordinator).
- A join event appears in `zigctl listen raw` output.
- Optional: plug toggles on/off using `zigctl mqtt pub`.

---

## Troubleshooting (Common Pitfalls)

- **No join events appear:**
  - Make sure permit-join is enabled *right before* pairing.
  - Retry pairing mode (hold button >10 seconds until it flashes).

- **Device doesn’t show in list:**
  - Re-run `zigctl bridge devices` after 30–60 seconds.

- **Broker not reachable:**
  - Confirm the stack is up (`docker ps`).
  - Ensure you are using port **1884** on the host.

---

## Next Steps
- Rename the device in Zigbee2MQTT UI for a friendly topic.
- Use `zigctl listen state --device <friendly_name>` to watch state changes live.
- Create a small script that toggles the plug based on your automation needs.
