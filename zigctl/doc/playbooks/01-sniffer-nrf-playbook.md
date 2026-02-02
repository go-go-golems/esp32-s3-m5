---
Title: nRF sniffer playbook (zigctl)
Slug: zigctl-sniffer-nrf-playbook
Short: Step-by-step playbook for capturing 802.15.4 traffic with an nRF sniffer using zigctl.
Topics:
  - zigbee
  - sniffer
  - zigctl
Commands:
  - sniff nrf list
  - sniff nrf info
  - sniff nrf channel
  - sniff nrf capture
  - sniff nrf live
  - sniff nrf bootloader
Flags:
  - --port
  - --channel
  - --format
  - --out
SectionType: Application
IsTopLevel: true
ShowPerDefault: true
Order: 10
---

This playbook is a practical, end-to-end checklist for capturing Zigbee (IEEE 802.15.4) traffic using the Nordic nRF sniffer and `zigctl`. It is optimized for quick, repeatable runs during development.

## Prerequisites

You need a Nordic nRF sniffer running the 802.15.4 sniffer firmware and connected via USB. You also need a local `zigctl` build with the `sniff nrf` commands available.

- Firmware flashed and device plugged in.
- `zigctl` compiled or runnable via `go run .`.
- Optional: Wireshark installed for live analysis.

## Steps

### Step 1: Confirm the sniffer is visible

List connected nRF sniffers by USB VID/PID. If multiple devices are listed, you will need `--port` in later steps.

```bash
zigctl sniff nrf list
```

### Step 2: Query device status

Check the device metadata and current channel. This also validates serial connectivity.

```bash
zigctl sniff nrf info --port /dev/ttyACM0
```

### Step 3: Set the channel (optional but recommended)

Set the Zigbee channel you want to capture. Valid values are 11–26.

```bash
zigctl sniff nrf channel --port /dev/ttyACM0 --channel 20
```

### Step 4: Capture to a pcapng file

Capture frames to a file for later analysis. Use `--format pcapng-tap` to keep RSSI/LQI metadata.

```bash
zigctl sniff nrf capture --port /dev/ttyACM0 --channel 20 --format pcapng-tap --out ./captures/office.pcapng
```

### Step 5: Live stream into Wireshark

Stream a live capture to Wireshark. This prints pcapng bytes to stdout, so avoid extra stdout logs.

```bash
zigctl sniff nrf live --port /dev/ttyACM0 --channel 20 --format pcapng-tap | wireshark -k -i -
```

### Step 6: Reboot into bootloader (dongle only)

Use this if you need to reflash the dongle.

```bash
zigctl sniff nrf bootloader --port /dev/ttyACM0
```

## Complete Example

```bash
# Discover device
zigctl sniff nrf list

# Inspect status
zigctl sniff nrf info --port /dev/ttyACM0

# Capture to file
zigctl sniff nrf capture --port /dev/ttyACM0 --channel 20 --format pcapng-tap --out ./captures/test.pcapng

# Live capture
zigctl sniff nrf live --port /dev/ttyACM0 --channel 20 --format pcapng-tap | wireshark -k -i -
```

## Troubleshooting

| Problem | Cause | Solution |
|---|---|---|
| `no nRF 802.15.4 sniffer devices found` | Device not connected or wrong firmware | Replug the USB device, confirm VID/PID, reflash sniffer firmware |
| `permission denied` on `/dev/ttyACM0` | User lacks serial permissions | Add user to `dialout`/`uucp` group and re-login |
| No packets in capture | Wrong channel or quiet network | Confirm coordinator channel, retry with known traffic |
| Wireshark shows no RSSI/LQI | Output format not TAP | Use `--format pcapng-tap` |

## See Also

- `zigctl help zigctl-architecture-overview` — zigctl CLI conventions and output patterns.
- `zigctl help zigctl-sniffer-nrf-playbook` — this playbook entry (for quick access).
- `zigctl sniff nrf doctor` — hardware diagnostics.
