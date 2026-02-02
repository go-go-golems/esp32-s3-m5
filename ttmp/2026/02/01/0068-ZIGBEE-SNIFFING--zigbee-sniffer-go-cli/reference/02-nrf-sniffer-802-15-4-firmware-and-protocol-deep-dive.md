---
Title: nRF Sniffer 802.15.4 Firmware and Protocol Deep Dive
Ticket: 0068-ZIGBEE-SNIFFING
Status: active
Topics:
    - zigbee
    - sniffer
    - pcap
    - go
    - cli
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../../../tmp/sdk-nrf/samples/peripheral/802154_sniffer/prj.conf
      Note: Sniffer firmware configuration (raw mode
    - Path: ../../../../../../../../../../../../tmp/sdk-nrf/samples/peripheral/802154_sniffer/src/main.c
      Note: Firmware receive path
    - Path: nRF-Sniffer-for-802.15.4/README.md
      Note: Repository overview and firmware source pointer
    - Path: nRF-Sniffer-for-802.15.4/nrf802154_sniffer/nrf802154_sniffer.py
      Note: Extcap host protocol
ExternalSources:
    - https://www.nordicsemi.com/Products/Development-tools/nRF-Sniffer-for-802154
    - https://docs.nordicsemi.com/bundle/ug_sniffer_802154/page/UG/sniffer_802154/programming_firmware_802154.html
    - https://www.wireshark.org/docs/man-pages/extcap.html
    - https://datatracker.ietf.org/doc/draft-ietf-opsawg-pcaplinktype/
    - https://docs.rs/pcap-sys/latest/pcap_sys/constant.DLT_IEEE802_15_4_NOFCS.html
    - https://wiki.makerdiary.com/nrf52840-mdk-usb-dongle/capturing_data/
    - https://wiki.makerdiary.com/nrf52840-mdk-usb-dongle/inspecting_data/
    - https://www.ti.com/solution/zigbee
    - https://www.nxp.com/design/software/embedded-software/zigbee-3-0-software:ZIGBEE-3-0-SW
    - https://en.wikipedia.org/wiki/IEEE_802.15.4
    - https://en.wikipedia.org/wiki/Zigbee
Summary: Deep, source-linked analysis of the Nordic nRF 802.15.4 sniffer firmware, host extcap protocol, data formats, and Zigbee fundamentals.
LastUpdated: 2026-02-02T11:35:49-05:00
WhatFor: Build, validate, and extend zigctl sniff and other tooling that needs to speak the Nordic sniffer protocol correctly.
WhenToUse: Use when implementing capture backends, decoding metadata, or debugging sniffer firmware/host integration.
---


# nRF Sniffer 802.15.4 Firmware and Protocol Deep Dive

## Goal

Provide a detailed, source-anchored understanding of the Nordic 802.15.4 sniffer firmware and the host-side protocol used to capture frames into Wireshark/pcap. This document is written as an engineering textbook: it explains the system end-to-end (radio to packets), exposes the serial and pcap data formats, and grounds the behavior in Zigbee and IEEE 802.15.4 fundamentals that matter for sniffing.

## Context

Nordic's nRF Sniffer for 802.15.4 is a Wireshark-integrated capture tool built on a small firmware sample and a host-side extcap plugin. The firmware is an nRF Connect SDK sample called `802154_sniffer` that runs on nRF52/nRF53 boards; it enables promiscuous IEEE 802.15.4 reception, prints packet data over a UART shell, and exposes a simple serial command set. The host plugin (a Python extcap script) discovers the sniffer via USB VID/PID, sends commands, parses the serial output, and emits pcap frames to Wireshark. The overall system is simple by design: a text-based serial protocol on one side, a pcap linktype on the other.

## Quick Reference

### Serial command protocol (firmware shell)

Commands accepted over the serial shell:

- `channel <11-26>`: set the IEEE 802.15.4 channel.
- `receive`: start capture (radio RX on).
- `sleep`: stop capture (radio off).
- `bootloader`: reboot into bootloader (dongle only).

Source pointers (local clone):

- Firmware command handlers: `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/src/main.c#L89` through `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/src/main.c#L162`
- Shell command registration: same file, `SHELL_CMD_ARG_REGISTER(...)` at lines 108, 121, 134, 161

### Packet line format (serial output)

The firmware prints one line per received frame:

```
received: <hex-psdu> power: <rssi_dbm> lqi: <lqi> time: <timestamp_us>
```

- `<hex-psdu>` is the PSDU bytes in hex (includes FCS in firmware output; the host strips the last 2 bytes).
- `<rssi_dbm>` is signed RSSI in dBm.
- `<lqi>` is IEEE 802.15.4 Link Quality Indicator.
- `<timestamp_us>` is time since boot in microseconds (derived from packet timestamp).

Source pointers (local clone):

- Firmware format string: `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/src/main.c#L77`
- Host regex and parsing logic (strip last 4 hex chars = 2 bytes): `nRF-Sniffer-for-802.15.4/nrf802154_sniffer/nrf802154_sniffer.py#L97` and `#L150`

### Host pipeline summary

1. Extcap enumerates USB CDC devices with VID 0x1915 and PID 0x154B.
2. The extcap script issues `sleep`, turns shell echo off, sets `channel`, and sends `receive`.
3. The script reads serial lines and parses the `received:` format.
4. It converts relative timestamps to UNIX time (first packet anchors the epoch).
5. It writes pcap records to a FIFO using a chosen DLT:
   - DLT 283: IEEE 802.15.4 TAP (adds metadata via TLVs)
   - DLT 230: IEEE 802.15.4 without FCS

Source pointers (local clone):

- VID/PID + enumeration: `nRF-Sniffer-for-802.15.4/nrf802154_sniffer/nrf802154_sniffer.py#L89`
- Serial command sequence: `nRF-Sniffer-for-802.15.4/nrf802154_sniffer/nrf802154_sniffer.py#L387`
- Pcap record writing + TAP header: `nRF-Sniffer-for-802.15.4/nrf802154_sniffer/nrf802154_sniffer.py#L315`

### Key config flags (firmware)

- `CONFIG_IEEE802154_RAW_MODE=y` and `CONFIG_NET_PKT_TIMESTAMP=y` to access raw frames with timestamps.
- USB CDC ACM configuration and VID/PID match the host script.
- Increased RX buffers to reduce frame drops.

Source pointer: `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/prj.conf#L1`

## Usage Examples

### Manual serial interaction (firmware shell)

```
# Connect to the CDC ACM serial port.
channel 23
receive
# Observe lines:
received: 4e45... power: -39 lqi: 220 time: 15822687
sleep
```

### Pseudocode: minimal host reader (single channel)

```pseudo
open_serial(port)
write("sleep\r\n")
write("shell echo off\r\n")
write("channel 15\r\n")
write("receive\r\n")

pcap = open_fifo()
write_pcap_global_header(dlt = IEEE802_15_4_TAP)

while line = read_line():
    if match(line, "received: <hex> power: <rssi> lqi: <lqi> time: <t>"):
        psdu_hex = group("hex")
        psdu = hex_to_bytes(psdu_hex[0:-4])   # strip FCS
        ts = convert_relative_time(t)
        record = build_pcap_record(psdu, ts, rssi, lqi, channel)
        pcap.write(record)
```

### Pseudocode: extcap handshake (Wireshark)

```pseudo
if --extcap-interfaces:
    print_extcap_interface_entries()
if --extcap-dlts:
    print_dlt_entries()
if --extcap-config:
    print_arg_entries()
if --capture:
    open_fifo_and_stream_pcap()
```

## Part I: System Overview

```
+-----------------------+        +------------------------+
| IEEE 802.15.4 Air     |        | Wireshark / tshark      |
| (Zigbee, Thread, etc) |        | dissectors, UI          |
+-----------+-----------+        +------------+-----------+
            |                                 ^
            v                                 |
+-----------------------+        +------------+-----------+
| nRF Firmware          |  UART  | Host extcap plugin     |
| - Promiscuous RX      +------->| - Parse serial lines   |
| - Timestamp/LQI/RSSI  |        | - Build pcap records   |
| - Shell CLI           |        | - FIFO to Wireshark    |
+-----------+-----------+        +------------+-----------+
            |                                 ^
            v                                 |
+-----------------------+        +------------+-----------+
| USB CDC ACM (VID/PID) |        | Pcap linktype          |
+-----------------------+        | - IEEE 802.15.4 TAP    |
                                 | - IEEE 802.15.4 NOFCS  |
                                 +------------------------+
```

The core idea: keep the firmware minimal, expose a text-based serial API, and let Wireshark do the heavy lifting. The host is a thin translation layer between two wire formats: the firmware line protocol and pcap records.

## Part II: Firmware Architecture

### 1. Radio configuration and promiscuous mode

The firmware configures the 802.15.4 radio in promiscuous mode and disables automatic ACK generation. This is critical for sniffing: it allows the radio to capture frames not addressed to the device without affecting the network by sending ACKs.

Source pointer (local clone): `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/src/main.c#L172`

### 2. Packet receive path

The receive path is centered on Zephyr's `net_recv_data` callback. When a packet arrives, the firmware:

1. Extracts the raw PSDU from the packet buffer.
2. Reads LQI and RSSI fields from the packet metadata.
3. Pulls a timestamp (PTP time), converts to microseconds.
4. Toggles LED 4 to indicate activity.
5. Prints a line in the `received:` format to the UART shell.

Source pointer (local clone): `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/src/main.c#L55`

### 3. Timestamps and time base

The packet timestamp is derived from Zephyr's packet timestamp (seconds + nanoseconds). The host interprets this as time since boot and converts it to a UNIX epoch by anchoring the first observed packet to the local clock. This gives Wireshark a monotonic timeline that is "close enough" for capture analysis, without requiring NTP or time synchronization on the device.

Source pointer (local clone): `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/src/main.c#L69` and `nRF-Sniffer-for-802.15.4/nrf802154_sniffer/nrf802154_sniffer.py#L117`

### 4. Shell commands

The firmware uses Zephyr's shell backend over UART. Each command is minimal and has a single responsibility:

- `channel`: set the radio channel.
- `receive`: put the radio into RX state and speed up the heartbeat LED.
- `sleep`: stop RX and slow the heartbeat LED.
- `bootloader`: (dongle only) pull reset low to enter bootloader mode.

Source pointer (local clone): `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/src/main.c#L89`

### 5. USB CDC ACM configuration

The firmware advertises as a CDC ACM device with a specific VID/PID that the host plugin looks for. This stable identity is how the extcap script finds the sniffer.

Source pointer (local clone): `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/prj.conf#L28`

## Part III: Serial Protocol and Data Format

The serial protocol is intentionally human-readable. It is a line-based shell output with a single packet format. That makes it debuggable with any serial terminal and easy to parse in a host tool.

### 1. Syntax

Simplified ABNF:

```
LINE = "received:" SP HEXDATA SP "power:" SP RSSI SP "lqi:" SP LQI SP "time:" SP TIMESTAMP
HEXDATA = 1*HEXDIG
RSSI = ["-"] 1*DIGIT
LQI = 1*DIGIT
TIMESTAMP = ["-"] 1*DIGIT
```

### 2. Data semantics

- `HEXDATA` is the PSDU, byte-for-byte, in hexadecimal.
- The host removes the last two bytes (4 hex characters) before writing to pcap. This likely corresponds to the FCS (CRC) that Wireshark expects to be omitted in the NOFCS and TAP linktypes.
- `RSSI` is a signed integer in dBm.
- `LQI` is an 8-bit indicator of link quality.
- `TIMESTAMP` is microseconds since boot (derived from the PTP time in `net_pkt`).

### 3. Why text, not binary?

Text has three advantages:

- Works with Zephyr shell without custom framing code.
- Debuggable in a terminal in case extcap fails.
- Tolerant to missing packets (parsing is line-based and stateless).

The downside is throughput: long hex strings are larger than binary frames. The firmware compensates partially by increasing RX buffer count and by running over USB CDC ACM rather than a slower UART.

## Part IV: Host Extcap Plugin and PCAP/TAP

### 1. Extcap role in Wireshark

Wireshark extcap programs are external capture modules invoked by Wireshark. They advertise interfaces, capture options, and output raw pcap data to a FIFO. The nRF sniffer plugin is one of these extcap programs. Wireshark asks it for:

- interfaces (`--extcap-interfaces`),
- supported DLTs (`--extcap-dlts`),
- config arguments (`--extcap-config`), and
- actual capture (`--capture --fifo <path>`).

This matches the extcap protocol defined in Wireshark's manual. The extcap module is simply a bidirectional adapter between the sniffer's serial protocol and a pcap stream.

### 2. DLT choices

The host script supports two pcap linktypes:

- `DLT_IEEE802_15_4_TAP (283)`: includes a TAP header with metadata (RSSI, channel, LQI).
- `DLT_IEEE802_15_4_NOFCS (230)`: raw frames without the FCS bytes.

The TAP linktype is defined in the IETF pcap linktype registry, while NOFCS is defined in libpcap constants. These are the identifiers Wireshark uses to invoke the correct dissector.

### 3. Pcap record layout (with TAP)

```
+----------------------+-------------------------------------------+
| Pcap Record Header   | 16 bytes                                  |
| - ts_sec             |                                           |
| - ts_usec            |                                           |
| - incl_len           |                                           |
| - orig_len           |                                           |
+----------------------+-------------------------------------------+
| 802.15.4 TAP header  | 28 bytes (TLVs)                            |
| - TLV: RSSI (float)  |                                           |
| - TLV: Channel       |                                           |
| - TLV: LQI           |                                           |
+----------------------+-------------------------------------------+
| PSDU bytes           | (no FCS)                                   |
+----------------------+-------------------------------------------+
```

The extcap script computes `incl_len` as `len(PSDU) + 28` when TAP is enabled and writes the TLVs as defined in the TAP specification. The PSDU bytes follow the TLVs.

### 4. Timestamp conversion

The host maintains two timestamps: the first local UNIX time and the first sniffer timestamp. Each subsequent sniffer timestamp is translated to UNIX time by offsetting from the initial pair. This preserves ordering and approximate real time for Wireshark without requiring the device to know the UNIX epoch.

## Part V: Zigbee Fundamentals That Matter for Sniffing

> CALL OUT: Fundamentals
> Zigbee uses IEEE 802.15.4 at the PHY and MAC layers (2.4 GHz O-QPSK DSSS with channels 11-26). Zigbee then defines network (NWK) and application (APS/ZCL) layers on top of those frames. As a sniffer, you see the 802.15.4 MAC frames; Zigbee parsing is a higher-layer interpretation of the MAC payload.

### 1. PHY and channelization

IEEE 802.15.4 defines channelization and modulation for low-rate WPANs. In the 2.4 GHz band, channels 11-26 use O-QPSK with DSSS. For a sniffer, this means:

- You must pick a channel to capture; you cannot capture all channels simultaneously with a single radio.
- Channel 11 is the default in many Zigbee deployments, but coordinators can pick any channel 11-26.

### 2. MAC frame types and addressing

Zigbee traffic rides inside the 802.15.4 MAC payload. The MAC header includes addressing fields (PAN ID, short address, extended address) and control flags (frame type, security, ACK request). Sniffers capture the raw MAC frames; dissectors decode higher layers.

### 3. Roles and topology

Zigbee networks are typically mesh networks with three roles:

- **Coordinator**: starts the network, manages keys, may bridge to other networks.
- **Router**: forwards traffic, stays powered.
- **End device**: often sleeps and only communicates through a parent.

Sniffed traffic patterns differ by role. For example, end devices may emit bursts of data and then go quiet; routers show forwarding traffic.

### 4. Security implications

Zigbee security uses AES-128 keys at the network and application layers. Even with a sniffer, packet contents may be encrypted unless you have the network key or link keys for decryption. The sniffer's role is to capture the frames faithfully; decryption is a downstream step in Wireshark or other tools.

## Part VI: Practical Interaction Guidance

### 1. Channel strategy

Single-channel sniffing is the default. If you want broader coverage:

- Use multiple sniffers, one per channel.
- Or implement channel hopping (with the cost of missing frames during hops).

### 2. LQI and RSSI usage

RSSI gives an absolute signal power estimate; LQI provides a quality metric tied to the PHY. In practice:

- RSSI is useful for distance or interference heuristics.
- LQI is often more stable for link health.

### 3. FCS handling

The extcap script strips the last two bytes from the firmware hex string. Wireshark expects no FCS for DLT_IEEE802_15_4_NOFCS and the TAP variant (unless a specific FCS-present linktype is used). This is why the host removes the FCS. If you implement your own host, match this behavior or explicitly choose a linktype that includes FCS.

### 4. Time alignment

The device timestamp is relative; the host anchors it to the first packet's local time. If you need high-precision timestamps across multiple sniffers, you must implement clock synchronization (e.g., PTP, GPS, or host side calibration).

## Part VII: Implications for zigctl sniff

The firmware and extcap protocol imply a minimal, robust capture backend for `zigctl sniff`:

- The capture loop can be line-based and stateless (no framing beyond newline).
- The sniffer only exposes channel and capture state; no filtering happens on-device.
- The host is responsible for choosing linktype and for emitting metadata (RSSI, LQI, channel) in TAP TLVs.
- The PSDU is the canonical frame payload; any higher-level parsing should be separate.

This aligns with the Go CLI design document: the sniffer backend should normalize into a simple `Frame{timestamp, channel, rssi, lqi, bytes}` model, then write pcapng/TAP for Wireshark compatibility.

## Appendix A: ASCII Protocol Diagram

```
FIRMWARE (UART line):

received: <hex_psdu_with_fcs> power: <rssi_dbm> lqi: <lqi> time: <t_us>

HOST (pcap record):

pcap_record_header(ts_sec, ts_usec, incl_len, orig_len)
+ tap_tlvs(rssi, channel, lqi)
+ psdu_bytes_without_fcs
```

## Appendix B: Source Map (local clones)

Firmware (nRF Connect SDK sample):

- `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/src/main.c`
- `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/prj.conf`
- `/tmp/sdk-nrf/samples/peripheral/802154_sniffer/README.rst`

Host extcap plugin (Nordic repo clone):

- `nRF-Sniffer-for-802.15.4/nrf802154_sniffer/nrf802154_sniffer.py`
- `nRF-Sniffer-for-802.15.4/README.md`

## Related

- `ttmp/2026/02/01/0068-ZIGBEE-SNIFFING--zigbee-sniffer-go-cli/design-doc/01-go-zigbee-sniffer-pcap-decoder-cli.md`

## Sources (external)

- Nordic nRF Sniffer product page: https://www.nordicsemi.com/Products/Development-tools/nRF-Sniffer-for-802154
- Nordic sniffer programming guide (login required): https://docs.nordicsemi.com/bundle/ug_sniffer_802154/page/UG/sniffer_802154/programming_firmware_802154.html
- Wireshark extcap manual: https://www.wireshark.org/docs/man-pages/extcap.html
- IETF pcap linktype registry: https://datatracker.ietf.org/doc/draft-ietf-opsawg-pcaplinktype/
- libpcap linktype constant (NOFCS): https://docs.rs/pcap-sys/latest/pcap_sys/constant.DLT_IEEE802_15_4_NOFCS.html
- Makerdiary sniffer capture guide: https://wiki.makerdiary.com/nrf52840-mdk-usb-dongle/capturing_data/
- Makerdiary sniffer inspection guide: https://wiki.makerdiary.com/nrf52840-mdk-usb-dongle/inspecting_data/
- Zigbee overview (TI): https://www.ti.com/solution/zigbee
- Zigbee 3.0 overview (NXP): https://www.nxp.com/design/software/embedded-software/zigbee-3-0-software:ZIGBEE-3-0-SW
- IEEE 802.15.4 overview (Wikipedia): https://en.wikipedia.org/wiki/IEEE_802.15.4
- Zigbee overview (Wikipedia): https://en.wikipedia.org/wiki/Zigbee
