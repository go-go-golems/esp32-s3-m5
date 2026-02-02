---
Title: zigctl nRF Sniffer Tooling for Coding Sessions
Ticket: 0068-ZIGBEE-SNIFFING
Status: active
Topics:
    - zigbee
    - sniffer
    - pcap
    - go
    - cli
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/ttmp/2026/02/01/0068-ZIGBEE-SNIFFING--zigbee-sniffer-go-cli/design-doc/01-go-zigbee-sniffer-pcap-decoder-cli.md
      Note: |-
        Baseline zigctl sniff design doc (overall capture + decode scope)
        base sniff design scope
    - Path: esp32-s3-m5/ttmp/2026/02/01/0068-ZIGBEE-SNIFFING--zigbee-sniffer-go-cli/reference/02-nrf-sniffer-802-15-4-firmware-and-protocol-deep-dive.md
      Note: |-
        Firmware and extcap protocol deep dive (serial line format, pcap/TAP details)
        serial protocol and pcap/TAP reference
    - Path: esp32-s3-m5/zigctl/cmd/mqtt/root.go
      Note: |-
        zigctl command group structure and Glazed/Cobra integration
        command group layout and Glazed wiring example
    - Path: esp32-s3-m5/zigctl/cmd/root.go
      Note: |-
        zigctl root wiring and registration patterns
        zigctl root command registration pattern
    - Path: esp32-s3-m5/zigctl/pkg/zigbee/config.go
      Note: |-
        zigctl config loading and defaults
        config loading pattern for zigctl
ExternalSources: []
Summary: Detailed design for a zigctl subgroup that directly controls and captures from Nordic nRF 802.15.4 sniffer hardware, optimized for interactive coding sessions.
LastUpdated: 2026-02-02T11:49:49-05:00
WhatFor: Guide implementation of nRF sniffer tooling that integrates with zigctl and enables quick, reliable capture workflows during development.
WhenToUse: Use when implementing `zigctl sniff nrf` commands, serial protocol handling, device discovery, and capture pipelines.
---


# zigctl nRF Sniffer Tooling for Coding Sessions

## Executive Summary

Add a dedicated `zigctl sniff nrf` command group that makes the Nordic nRF 802.15.4 sniffer hardware feel like a first-class developer tool. The goal is to minimize setup friction in day-to-day coding sessions: discover the device, set channel, start/stop capture, stream pcap to Wireshark, and export pcapng files, all from `zigctl` with consistent Glazed output. The implementation mirrors Nordic’s extcap protocol (text-based serial command/response and line-based packet frames) and emits IEEE 802.15.4 TAP metadata for Wireshark compatibility.

This design complements the existing `zigctl sniff` architecture by focusing on the **hardware interaction layer**: serial protocol, device discovery, capture control, and developer ergonomics. It keeps the pipeline small and deterministic so it can be scripted, automated in tests, or run interactively in a shell during live debugging.

## Problem Statement

We need reliable, low-friction tooling to interact with a Nordic nRF 802.15.4 sniffer during development. The current workflow relies on Wireshark extcap and a Python script; it is great for manual use but awkward to automate or embed in our own CLI. We want a `zigctl`-native workflow that:

- Finds the sniffer device automatically and reliably (USB VID/PID).
- Lets us set channel and capture state without opening a terminal emulator.
- Streams captures to pcap/pcapng with IEEE 802.15.4 TAP metadata.
- Works in short-lived, iterative coding sessions without extra setup.
- Produces structured output for scripting and quick diagnostics.

## Scope

### In scope (this doc)

- Device discovery and identification (VID/PID, serial path).
- Serial control protocol (channel, receive, sleep, bootloader).
- Capture pipeline for nRF serial output → `Frame` → pcapng/TAP.
- Zigctl CLI UX for daily use (list, info, capture, live, doctor).
- Configuration defaults and overrides in zigctl config.
- Tests and validation strategy for parsing and pcap output.

### Out of scope

- Deep Zigbee decode/analysis (handled by `zigctl sniff decode` in the base design doc).
- Firmware compilation or build system integration (optional future). 
- Alternative sniffers beyond Nordic nRF (ZEP, other chips) — separate capture backend.

## Constraints and Assumptions

- Nordic firmware prints a fixed line format: `received: <hex> power: <rssi> lqi: <lqi> time: <timestamp>`.
- Host side must strip the last two bytes (FCS) to match the IEEE 802.15.4 NOFCS/TAP linktypes.
- The sniffer exposes a USB CDC ACM serial port with VID 0x1915 and PID 0x154B.
- The Zigbee channel is in the 11–26 range.
- We are building inside the existing `zigctl` Go module and must follow its Glazed/Cobra conventions.

## Proposed Solution

### Command group: `zigctl sniff nrf`

Provide a focused set of commands that cover 90% of interactive sniffer workflows:

```
zigctl sniff nrf list
zigctl sniff nrf info --port /dev/ttyACM0
zigctl sniff nrf channel --port /dev/ttyACM0 --set 20
zigctl sniff nrf capture --port /dev/ttyACM0 --channel 20 --out capture.pcapng
zigctl sniff nrf live --port /dev/ttyACM0 --channel 20 | wireshark -k -i -
zigctl sniff nrf doctor
zigctl sniff nrf bootloader --port /dev/ttyACM0
```

> CALL OUT: Developer ergonomics
> The list/info/doctor commands should be fast and zero-config so they can be run repeatedly during a coding session. Capture commands should default to a sensible channel and output format, and log enough context to help debug when hardware is misconfigured.

### Architecture Overview

```
+------------------------+     +---------------------+     +------------------------+
| zigctl sniff nrf CLI   | --> | nrf serial session  | --> | Frame stream (channel) |
+------------------------+     +---------------------+     +------------------------+
          |                                                          |
          |                                                          v
          |                                             +-----------------------+
          |                                             | pcapng writer (TAP)   |
          |                                             +-----------------------+
          |                                                          |
          v                                                          v
+------------------------+                                 +---------------------+
| Status/diagnostics     |                                 | stdout/pcapng file  |
+------------------------+                                 +---------------------+
```

### Key ideas

- The **nRF serial session** is a small state machine that sends `sleep`, `shell echo off`, `channel`, and `receive`, then parses incoming lines into packets.
- A **frame model** captures `timestamp`, `rssi`, `lqi`, `channel`, and `payload` (no FCS). This matches the baseline sniff design and allows reuse of pcapng/TAP writers.
- The **pcapng writer** is responsible for translating frames into TAP metadata. This is how Wireshark sees channel/RSSI/LQI.

## Detailed CLI Design

### `zigctl sniff nrf list`

- Output: detected devices (port path, VID/PID, USB serial, product string).
- Glazed output: table/JSON/CSV with fields: `port`, `vid`, `pid`, `serial`, `manufacturer`, `product`.
- Uses the serial enumerator (`go.bug.st/serial/enumerator` or equivalent).

### `zigctl sniff nrf info`

- Connects to a specific port and prints:
  - device identification (VID/PID, product string)
  - current channel (by sending `channel` with no arg)
  - simple status (readiness, DTR line availability)
- Optional: attempt a `sleep` command to ensure control.

### `zigctl sniff nrf channel`

- `--set <11-26>`: set channel and confirm it by reading the channel value back.
- If no `--set`, print the current channel.

### `zigctl sniff nrf capture`

- Long-running capture that writes to a pcapng file.
- Defaults:
  - `--channel` default: 11
  - `--out` default: `sniff-<timestamp>.pcapng`
  - `--format` default: `pcapng-tap`
- Emits a summary of frames, dropped lines, and output path on exit.

### `zigctl sniff nrf live`

- Same as capture, but writes pcapng to stdout (or a named FIFO).
- Designed for `wireshark -k -i -` or `tshark -i -` workflows.
- Support `--quiet` to suppress non-pcap output to stdout.

### `zigctl sniff nrf doctor`

- A quick diagnostic command to validate:
  - device discovery
  - port accessibility (permissions)
  - ability to send `sleep`/`channel`/`receive`
  - ability to parse a mocked packet line

### `zigctl sniff nrf bootloader`

- Send `bootloader` command (dongle only).
- If unsupported, return a user-friendly error.

## Configuration

### New config section

Extend `~/.config/zigctl/config.yaml` with a `sniffer:` block:

```yaml
sniffer:
  nrf:
    default_port: "/dev/ttyACM0"
    default_channel: 11
    default_format: "pcapng-tap"
    auto_sleep_on_exit: true
    serial_baud: 115200
```

> CALL OUT: Backward compatibility
> The config loader should be extended in a backward-compatible way; if the `sniffer` block is missing, defaults are applied. This mirrors the existing zigctl config style.

### Glazed layer

Create a `sniffer` Glazed layer with fields:

- `sniffer.nrf.default_port` (string)
- `sniffer.nrf.default_channel` (int)
- `sniffer.nrf.default_format` (enum: `pcapng-tap`, `pcap-nofcs`)
- `sniffer.nrf.auto_sleep_on_exit` (bool)
- `sniffer.nrf.serial_baud` (int)

This layer is applied to the new commands to keep flags consistent and allow config-driven defaults.

## Serial Protocol Handling

### Command sequencing

The host should follow the extcap sequence that Nordic’s Python script uses:

1. `sleep` (ensure clean state)
2. `shell echo off` (avoid echo noise in parsing)
3. `channel <N>`
4. `receive`

### Parsing lines

- Use a compiled regex to parse `received: <hex> power: <rssi> lqi: <lqi> time: <ts>`.
- Strip the last 2 bytes from the hex payload to remove FCS.
- Convert timestamp to UNIX time with the same algorithm as extcap: anchor first packet to local time.

### Pseudocode: session lifecycle

```pseudo
open_serial(port, baud)
write("sleep\r\n")
write("shell echo off\r\n")
write("channel <N>\r\n")
write("receive\r\n")

for each line:
  if matches(packet_line):
     frame = parse(line)
     frame.payload = strip_fcs(frame.payload)
     frame.ts = correct_time(frame.ts)
     send(frame)

on exit:
  write("sleep\r\n")  # optional, configurable
  close_serial()
```

### Error handling

- If serial disconnects, surface a clear error and exit non-zero.
- If a line cannot be parsed, increment a counter and continue (log at debug).
- If device does not respond to `channel` or `receive`, return a user-friendly error with hints.

## Pcapng/TAP Emission

- Use the IEEE 802.15.4 TAP DLT (283) to include channel/RSSI/LQI.
- If TAP is not supported in the output (or requested), fall back to NOFCS (DLT 230).
- The payload bytes are the PSDU without FCS.

> CALL OUT: TAP metadata
> This is the critical path for Wireshark usability. Without TAP TLVs, users lose channel and RSSI/LQI data, and multi-sniffer coordination becomes harder.

## Implementation Plan

### Phase 1: Wiring and config

- Add `sniff` command group to zigctl root.
- Add `sniffer` config struct and config loader helpers.
- Add `sniffer` Glazed layer.

### Phase 2: Core serial session

- Implement `pkg/sniffer/nrf` with:
  - device discovery (VID/PID)
  - serial session (command send + line read)
  - packet parser + FCS strip
  - timestamp correction

### Phase 3: CLI commands

- Implement `list`, `info`, `channel`, `capture`, `live`, `doctor`, `bootloader`.
- Ensure `capture`/`live` use the same pipeline and differ only by output sink.

### Phase 4: Tests and validation

- Unit tests for line parsing (including malformed input).
- Golden tests for pcapng/TAP record output.
- A mocked serial reader test that feeds known lines and checks frame output.

## Alternatives Considered

### 1. Use the Python extcap script directly

- Pros: proven, Wireshark-compatible.
- Cons: not integrated with zigctl; difficult to script and to reuse in Go code; adds Python dependency.

### 2. Embed libpcap and write pcap directly

- Pros: standard capture pipeline.
- Cons: heavier dependency; less portable; doesn’t help with the serial protocol anyway.

### 3. Build a full Zigbee decoder in Go

- Pros: avoid tshark dependency.
- Cons: large effort; not needed for capture or serial control; outside MVP.

## Risks and Mitigations

- **Serial protocol drift**: firmware could change line format.
  - Mitigation: keep parser tolerant, and document firmware version expectations.
- **Permission issues on serial ports**: common on Linux.
  - Mitigation: `doctor` prints actionable guidance (dialout group).
- **Performance of line parsing**: large frames and high throughput.
  - Mitigation: use buffered reads and avoid per-line allocations; incrementally parse.

## Open Questions

- Do we want to expose firmware flashing as a zigctl command (requires nrfjprog/west)?
- Should we support channel hopping mode as a separate command?
- Should live capture optionally fork Wireshark with the correct invocation?

## Success Criteria

- A developer can run `zigctl sniff nrf list` and see their device.
- `zigctl sniff nrf capture` writes a pcapng file that Wireshark decodes with RSSI/LQI.
- A `zigctl sniff nrf live | wireshark -k -i -` workflow works without extra tooling.
- The commands are scriptable and produce stable structured output.

## Appendix: Expected Module Layout (Proposed)

```
zigctl/
  cmd/
    sniff/
      root.go
      nrf_list.go
      nrf_info.go
      nrf_channel.go
      nrf_capture.go
      nrf_live.go
      nrf_doctor.go
      nrf_bootloader.go
  pkg/
    sniffer/
      nrf/
        protocol.go
        parser.go
        session.go
        discover.go
      model/
        frame.go
      pcap/
        tap.go
        writer.go
```

## Related

- `ttmp/2026/02/01/0068-ZIGBEE-SNIFFING--zigbee-sniffer-go-cli/reference/02-nrf-sniffer-802-15-4-firmware-and-protocol-deep-dive.md`
- `ttmp/2026/02/01/0068-ZIGBEE-SNIFFING--zigbee-sniffer-go-cli/design-doc/01-go-zigbee-sniffer-pcap-decoder-cli.md`
