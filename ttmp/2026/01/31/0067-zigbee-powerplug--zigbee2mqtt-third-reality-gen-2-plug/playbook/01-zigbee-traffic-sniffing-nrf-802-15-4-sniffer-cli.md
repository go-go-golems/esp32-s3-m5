---
Title: Zigbee traffic sniffing (nRF 802.15.4 sniffer, CLI)
Ticket: 0067-zigbee-powerplug
Status: active
Topics:
    - zigbee
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Headless/CLI playbook for over-the-air Zigbee sniffing with an nRF 802.15.4 sniffer and Zigbee2MQTT, including key handling and decryption guidance."
LastUpdated: 2026-01-31T11:59:43-05:00
WhatFor: "Capture and analyze Zigbee join/traffic on your own network using CLI tools and a Nordic nRF sniffer."
WhenToUse: "Use when you need an OTA Zigbee capture (pcapng) without Wireshark GUI, or to analyze join traffic for your own devices."
---

# Zigbee traffic sniffing (nRF 802.15.4 sniffer, CLI)

## Purpose

Capture and analyze over-the-air Zigbee traffic from your own network using a Nordic nRF 802.15.4 sniffer and `tshark` in a headless/CLI workflow. This playbook emphasizes legitimate auditing of your own devices and uses keys you already possess from your Zigbee2MQTT/coordinator setup.

## Environment Assumptions

- Linux host with `tshark` installed.
- nRF 802.15.4 sniffer attached (USB serial) and recognized by Wireshark extcap.
- Zigbee2MQTT running (so you can check channel and access keys).
- You are auditing *your own* Zigbee network/devices.

## Safety and scope (read first)

Zigbee is based on symmetric encryption (AES-128). Join traffic and most regular traffic are encrypted; you will need your **network key** and, for join/transport frames, often the **Trust Center link key** to decrypt. Zigbee2MQTT’s sniffing guide makes this explicit and documents how to obtain and format your keys. You should only use your own keys and only for your own devices/network.

## Decision: choose your capture path

### Option A — Over-the-air (OTA) sniffing (recommended with nRF)

This captures real RF frames. Use this for “what actually happened on the air.”

### Option B — Coordinator-side (ZEP over UDP)

Some setups forward coordinator packets via ZEP (UDP/17754). This is useful for Wireshark-style capture but is not a true RF sniffer. Zigbee2MQTT documents ZEP capture and key setup, including filtering with `udp.port==17754`.

## Prerequisites

- Confirm extcap visibility in `tshark`. `tshark` can list extcap interfaces, while `dumpcap -D` omits extcap interfaces.
- Know your Zigbee channel (`advanced.channel` in Zigbee2MQTT config).
- Have your Zigbee keys available from Zigbee2MQTT (network key, and Trust Center link key if needed).

## Procedure

### Step 1 — Verify extcap sniffer interface

Prose: The nRF sniffer appears as an **extcap** interface in `tshark` output. If you do not see it, ensure the proper extcap script/firmware is installed for your sniffer. The nRF-802.15.4-sniffer project is a common extcap implementation and documents the serial-based extcap workflow.

Commands:

```bash
sudo apt install -y wireshark tshark python3-serial
sudo tshark -D | grep -i -E 'nrf|802\.15\.4|sniffer'
```

### Step 2 — Find Zigbee channel

Prose: Zigbee2MQTT runs on a fixed channel (unless you change it). Use `advanced.channel` in your Zigbee2MQTT configuration to match the sniffer channel. Zigbee2MQTT recommends ZLL channels (11/15/20/25) to avoid interference.

Commands:

```bash
# Example: look in your Zigbee2MQTT config
rg -n "advanced:" -n /path/to/zigbee2mqtt/data/configuration.yaml
```

Expected snippet:

```yaml
advanced:
  channel: 15
```

### Step 3 — Inspect extcap options (channel, device)

Prose: Most extcap tools expose channel/serial options via `--extcap-config`. Use the extcap tool to list the available interfaces and configuration options.

Commands (pattern):

```bash
# locate extcap tools
ls /usr/lib/*/wireshark/extcap 2>/dev/null || true
ls /usr/lib/wireshark/extcap 2>/dev/null || true

# substitute <extcap-tool> with the actual tool name
tools/your-extcap-tool --extcap-interfaces
<extcap-tool> --extcap-config --extcap-interface=<interface>
```

### Step 4 — Capture OTA traffic to pcapng

Prose: Start capture *before* joining. OTA join frames are time-sensitive and only appear during the join window.

Commands:

```bash
sudo tshark -i <NrfSnifferIface> -w zigbee_join.pcapng
# Stop capture with Ctrl+C
```

### Step 5 — Trigger join while capturing

Prose: Keep capture running, then permit join in Zigbee2MQTT and place the plug into pairing mode.

Commands:

```bash
# Permit join for 60 seconds
mosquitto_pub -h localhost -t 'zigbee2mqtt/bridge/request/permit_join' -m '{"time": 60}'

# Put the plug into pairing mode (hold its button until LED flashes)
```

### Step 6 — Decrypt (legit: use your own keys)

Prose: Zigbee2MQTT’s sniffing guide explains that captured packets are encrypted and you typically need the network key and Trust Center link key to decrypt. It also shows how to format and load keys into Wireshark.

Key sources:
- **Network key**: from Zigbee2MQTT config or coordinator backup.
- **Trust Center link key**: often the default Zigbee Alliance key unless you changed it (note: some vendor ecosystems use different link keys).

Default key values (from Zigbee2MQTT sniffing guide):
- **Trust Center link key (ZigBeeAlliance09)**: `5A:69:67:42:65:65:41:6C:6C:69:61:6E:63:65:30:39`
- **Default network key** (if you never changed it): `01:03:05:07:09:0B:0D:0F:00:02:04:06:08:0A:0C:0D`

Prose: Zigbee2MQTT recommends using a random network key (rather than the default) for security; if you did so, use the one stored in your configuration/backup.

### Step 7 — Headless analysis (tshark)

Prose: After you’ve configured keys in Wireshark (prefs are shared with tshark), you can run `tshark` over the capture file to inspect decoded fields headlessly.

Commands:

```bash
# list Zigbee-layer frames
tshark -r zigbee_join.pcapng -Y 'zbee_nwk || zbee_aps' | head

# filter for command frames (join/announce often show up here)
tshark -r zigbee_join.pcapng -Y 'zbee_nwk.cmd.id || zbee_aps' | head
```

## Pseudocode (workflow logic)

```text
function capture_join(sniffer_iface, channel):
    assert extcap_available(sniffer_iface)
    start_capture(sniffer_iface, channel, outfile="zigbee_join.pcapng")
    permit_join(time=60)
    put_device_in_pairing_mode()
    stop_capture()
    if have_keys():
        load_keys_into_wireshark()
        analyze_with_tshark("zigbee_join.pcapng")
```

## Failure modes and fixes

- **No extcap interface shown**: verify nRF sniffer firmware + extcap script installation; check the extcap tool path and pySerial dependency.
- **No useful payloads (encrypted)**: ensure network key and Trust Center link key are loaded in Wireshark/tshark.
- **Wrong channel**: confirm `advanced.channel` in Zigbee2MQTT config and set the sniffer to that channel.

## Exit Criteria

- A `zigbee_join.pcapng` file exists and contains frames.
- Join sequence traffic is visible (association, device announce, or APS/NWK frames).
- Optional: decrypted payloads are visible after keys are configured.

## References (authoritative)

- Zigbee2MQTT sniffing guide: https://www.zigbee2mqtt.io/advanced/zigbee/04_sniff_zigbee_traffic.html
- Zigbee2MQTT all settings (channel guidance): https://www.zigbee2mqtt.io/guide/configuration/all-settings.html
- Zigbee2MQTT secure network: https://www.zigbee2mqtt.io/advanced/zigbee/03_secure_network.html
- Tshark extcap interfaces: https://tshark.dev/capture/sources/extcap_interfaces/
- nRF 802.15.4 sniffer extcap: https://github.com/stig-bjorlykke/nRF-802.15.4-sniffer
