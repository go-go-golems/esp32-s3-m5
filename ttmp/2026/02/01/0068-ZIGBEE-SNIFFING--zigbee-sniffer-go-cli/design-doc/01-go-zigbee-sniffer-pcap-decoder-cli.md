---
Title: Zigbee Sniffer + PCAP Decoder (zigctl sniff)
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
    - Path: ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/design-doc/01-zigbee-cli-tool-design-zigctl.md
      Note: Reference for zigctl Glazed CLI conventions
    - Path: ttmp/2026/02/01/0068-ZIGBEE-SNIFFING--zigbee-sniffer-go-cli/sources/local/zigbee-sniffing.md
      Note: Primary imported notes that informed the design
ExternalSources: []
Summary: Design for a zigctl subgroup that captures 802.15.4 frames, writes pcapng/TAP, and decodes with tshark using Glazed commands.
LastUpdated: 2026-02-01T20:20:44-05:00
WhatFor: Guide implementation of the zigctl sniff subgroup and PCAP decoder.
WhenToUse: Use when building, reviewing, or extending the CLI architecture.
---



# Zigbee Sniffer + PCAP Decoder (zigctl sniff)

## Executive Summary

Design a Go CLI subgroup, `zigctl sniff`, that captures IEEE 802.15.4 frames from an nRF 802.15.4 sniffer (serial), writes pcapng with 802.15.4 TAP metadata (channel/RSSI/LQI), and optionally decodes captures via `tshark` JSON for higher-level Zigbee timelines and summaries. The sniffing functionality lives alongside the existing `zigctl` code under `zigbee/` (shared `go.mod`) to reuse shared config, Glazed layers, and utilities. Capture backends (nRF serial, ZEP UDP, pcap/pcapng file) feed a unified internal frame model and multiple sinks (pcapng writer, stdout stream, summary/report). Commands are implemented as Glazed commands (Bare/Writer/Glaze depending on output needs) to provide structured output formats with minimal formatter code. A native Zigbee parser is explicitly **out of scope** for the MVP; decoding uses Wireshark dissectors via `tshark` first, with a clear extension point for later native parsing and decryption.

## Problem Statement

We need a repeatable, CLI-first workflow to capture over-the-air Zigbee traffic and analyze it programmatically without constantly driving Wireshark by hand. The current nRF sniffer workflow is Wireshark-centric (extcap) and not easily automated. We also want a robust, Go-native capture tool that can:

- Capture true OTA frames (not just what the coordinator sees).
- Preserve channel/RSSI/LQI metadata in pcapng for use with Wireshark/tshark.
- Provide machine-readable decoding for CLI analysis (join timelines, device summaries).
- Support streaming and offline workflows (live pipe to Wireshark, or decode after the fact).

## Scope and Non-Goals

### In scope (MVP)

- zigctl subgroup: capture from nRF serial sniffer → pcapng (linktype 802.15.4 TAP).
- Optional live streaming to stdout (pcapng) for Wireshark.
- Optional decode subcommand using `tshark -T json`.
- ZEP UDP capture backend (if a sniffer bridge emits ZEP) to normalize into pcapng.
- pcap/pcapng file input backend for offline decode/summary.
- CLI UX for channel selection, serial port selection, and output paths.

### Explicit non-goals (MVP)

- Implementing full Zigbee NWK/APS/ZCL parsing in Go.
- Decrypting packets natively in Go (tshark/Wireshark handles it).
- Attacks, key recovery, or any offensive tooling.

## Proposed Solution

### High-Level Architecture

The CLI is built around a small internal frame model that is populated by capture backends and consumed by output sinks.

```
+-----------------------+
| Capture Backends      |
| - nRF Serial          |
| - ZEP UDP             |
| - PCAP/PCAPNG file    |
+----------+------------+
           |
           v
+-----------------------+
| Frame Model           |
| Frame{                |
|  ts, channel, rssi,   |
|  lqi, bytes []byte    |
| }                     |
+----------+------------+
           |
           v
+-----------------------+
| Sinks                 |
| - PCAPNG writer       |
| - stdout stream       |
| - tshark decode       |
| - summaries/reports   |
+-----------------------+
```

### Capture Backend: nRF Serial

- Treat the Nordic `nrf802154_sniffer.py` extcap script as the protocol spec. This file comes from the nRF 802.15.4 sniffer repository (the Wireshark extcap integration) and defines the serial framing/commands we must match.
- Implement the serial framing and command/response semantics 1:1 in Go.
- Provide channel selection and a device/port discovery helper.
- Output frames with timestamp + metadata + raw bytes.

### Capture Backend: ZEP UDP

- ZEP (Zigbee Encapsulation Protocol) wraps 802.15.4 frames in UDP so Wireshark/tshark can decode them live.
- Listen on UDP `:17754` by default (Wireshark's conventional ZEP port).
- Parse ZEP headers + payload to reconstruct 802.15.4 frames and metadata (channel/RSSI/LQI when present).
- Normalize into the same `Frame` model and write as pcapng or decode.

### Capture Backend: PCAP/PCAPNG

- Read input files to drive the decode/summarize commands.
- Preserve metadata when present; tolerate plain 802.15.4 frames when absent.

### Output Sinks

- **pcapng writer** using `pcapgo.NgWriter` (no libpcap dependency).
- **stdout stream** (pcapng) for `wireshark -k -i -`.
- **tshark decoder**: shell out to `tshark -T json` and parse JSON events into higher-level summaries.
- **summary/report**: CLI summaries of joins, devices, cluster usage, timeline of events.

## Design Decisions

### Use pcapng + 802.15.4 TAP (linktype 283)

- pcapng is required for the 802.15.4 TAP pseudoheader metadata.
- Linktype 283 is the correct code for IEEE 802.15.4 TAP.
- This preserves channel/RSSI/LQI and makes captures first-class in Wireshark/tshark.

### Defer Zigbee parsing to tshark (initially)

- Wireshark dissectors are mature and handle Zigbee variants correctly.
- `tshark -T json` provides structured output usable by Go.
- A native parser can be added later in a limited, targeted way.

### Pluggable capture backends

- Captures can come from nRF serial, ZEP UDP, or existing pcap files.
- Normalizing to a single frame model simplifies decode/summarize logic.

### Glazed commands for output flexibility

- Use `cmds.BareCommand` for human-readable text output.
- Use `cmds.WriterCommand` for binary output (pcapng to file or stdout).
- Use `cmds.GlazeCommand` for structured output (tables/JSON/CSV/YAML).
- For commands that need both human and structured output, use dual mode (`cli.WithDualMode(true)` + `cli.WithGlazeToggleFlag(...)`) so a single command can expose both.
- Glazed handles parameter parsing and output formatting; Cobra is only used for command registration and the root entrypoint (zigctl convention).

### Align with zigctl architecture

- The sniffing functionality is a `zigctl` subgroup, sharing the same module and CLI wiring.
- Reuse the shared Zigbee Glazed layers, config loader, and logging facilities from zigctl.

## Detailed Design

### Repository Layout (Proposed)

```
zigbee/
  go.mod
  go.sum
  cmd/
    zigctl/
      root.go
      sniff/
        root.go
        capture_ota.go
        capture_zep.go
        live_ota.go
        decode_pcap.go
        summarize_pcap.go
        doctor_system.go
  internal/
    app/
      config.go
      logging.go
      run.go
    capture/
      nrf/
        protocol.go
        serial.go
        framing.go
      zep/
        zep.go
      file/
        pcap.go
    model/
      frame.go
    pcapng/
      writer.go
      tap.go
    decode/
      tshark.go
      json.go
    report/
      join_timeline.go
      device_summary.go
```

Notes:
- Command structure follows zigctl conventions: one file per verb, and one directory with a `root.go` per CLI group.
- `internal/pcapng/tap.go` contains TAP header struct + serialization.
- `internal/capture/nrf/protocol.go` mirrors `nrf802154_sniffer.py` framing.
- `internal/decode/tshark.go` shells out and streams JSON parsing.
- `report/` is strictly derived data, never authoritative parsing.

### CLI Commands (Draft)

```
zigctl sniff capture ota --port /dev/serial/by-id/usb-Nordic_... --channel 15 --out capture.pcapng
zigctl sniff capture zep --listen :17754 --out zep.pcapng
zigctl sniff live ota --port ... --channel 15 --stdout-pcapng | wireshark -k -i -
zigctl sniff decode pcap --in capture.pcapng --tshark-json --out frames.json
zigctl sniff summarize pcap --in capture.pcapng --format table --output json
zigctl sniff doctor system --output table
```

#### Capture flags

- `--port`: serial device path (auto-detect via `/dev/serial/by-id` optional)
- `--channel`: Zigbee channel (11–26)
- `--out`: pcapng output path
- `--stdout-pcapng`: stream to stdout instead of file
- `--tap`: include 802.15.4 TAP metadata (default true)

#### Decode flags

- `--tshark`: path or auto-detect `tshark` on PATH
- `--tshark-json`: output JSON to file or stdout
- `--filters`: tshark display filters

#### Summarize flags

- `--format`: `table|json|csv`
- `--keys`: optional path to known keys for tshark (network key, link key)

#### Glazed output flags (for GlazeCommand / dual commands)

- `--output`: `table|json|yaml|csv`
- `--fields`: select output columns
- `--sort-columns`: sort output columns

### Command Interface Mapping (Glazed)

| Command group | Verb | Interface | Output behavior |
| --- | --- | --- | --- |
| sniff capture | ota / zep | `cmds.WriterCommand` | Binary pcapng to file or stdout |
| sniff live | ota | `cmds.WriterCommand` | Binary pcapng stream to stdout |
| sniff decode | pcap | `cmds.GlazeCommand` | Structured rows of decoded frames + optional JSON sidecar |
| sniff summarize | pcap | `cmds.GlazeCommand` | Structured summaries (tables/JSON/CSV/YAML) |
| sniff doctor | system | `cmds.BareCommand` (or dual) | Human-readable checks; optionally dual-mode glazed |

### Internal Frame Model

```go
// internal/model/frame.go

type Frame struct {
    Timestamp time.Time
    Channel   uint8
    RSSI      int8
    LQI       uint8
    Payload   []byte // raw 802.15.4 MAC frame
}
```

- Metadata fields are optional for sources that don't provide them.
- `Payload` is always raw bytes to allow pcapng or tshark consumption.

### nRF Serial Protocol Integration

- Mirror the nRF extcap script framing exactly to avoid drift.
- Commands: set channel, start/stop capture, read frames.
- Frames: decode header → metadata → raw MAC frame.
- The protocol implementation should be testable with recorded serial fixtures.

### pcapng Writing (802.15.4 TAP)

- Write an Interface Description Block with linktype `LINKTYPE_IEEE802_15_4_TAP` (283).
- For each frame, emit Enhanced Packet Blocks:
  - TAP pseudoheader TLVs (channel, RSSI, LQI) when available.
  - Raw MAC frame bytes following the TAP header.
- If metadata missing, write the MAC frame without TAP TLVs (still 283 linktype).

### tshark Decoder Integration

- Spawn `tshark` with `-r <pcapng> -T json` (streaming stdout).
- Use a JSON decoder (streaming) to avoid loading the full capture into memory.
- Post-process into:
  - Join timeline (device join/rejoin events)
  - Per-device traffic summaries
  - Cluster usage (ZCL cluster IDs)
  - Event window around joins

### Zigbee Layer Primer (NWK / APS / ZCL)

- **NWK (Network Layer):** Zigbee routing, addressing, and network-level security headers. This is where device address/pan/route metadata lives.
- **APS (Application Support Sub-layer):** Application endpoints, groups, and binding. This is the bridge between network routing and application payloads.
- **ZCL (Zigbee Cluster Library):** Application-level clusters and attributes (e.g., On/Off, Metering, Temperature).

**Who parses these layers?** In the MVP, Wireshark/tshark dissectors do the NWK/APS/ZCL parsing. The CLI shells out to `tshark -T json` and extracts structured fields for summaries. Native parsing is a later, optional extension point for targeted clusters or offline use.

### Error Handling and Resilience

- Serial backend retries on transient read errors and logs frame drops.
- Clear error messages if `tshark` is missing or fails.
- Use context cancellation to stop capture gracefully on SIGINT.

### Config and UX

- Reuse zigctl config file `~/.config/zigctl/config.yaml` for shared settings.
- Add a `sniff` section for defaults (channel, serial port, tshark path).
- Environment overrides follow zigctl conventions (e.g., `ZIGCTL_TSHARK`, `ZIGCTL_SNIFF_DEFAULT_CHANNEL`).
- `zigctl sniff doctor system` validates serial port, tshark availability, and permissions.

### Glazed Command Construction (Pattern)

- Use `cmds.NewCommandDescription(...)` with explicit flags (via `fields.New`) and layers.
- Decode parsed parameters into a settings struct using `values.DecodeSectionInto(vals, schema.DefaultSlug, &Settings{})` or the equivalent parsed-layer decoding.
- For Glaze commands, return rows with `types.NewRow(...)` and let Glazed handle formatting.
- For Writer commands, stream binary output to the provided `io.Writer`.
- Build cobra commands with `cli.BuildCobraCommand`, optionally enabling dual mode with `cli.WithDualMode(true)` and a custom glaze toggle flag.

### Security & Compliance

- Only designed for **your own networks/devices**.
- Keys are never printed in logs by default.
- Optional secure key file path for tshark (`--keys`), not stored by the tool.

## Alternatives Considered

1. **Full native Zigbee stack in Go**
   - Pros: no external dependencies.
   - Cons: high complexity, hard to maintain, poor time-to-value.

2. **Use Wireshark extcap directly (no Go CLI)**
   - Pros: minimal engineering.
   - Cons: poor automation, limited CLI UX, no custom summaries.

3. **Use CC2531/whsniff**
   - Pros: existing CLI pipeline.
   - Cons: not current hardware, different firmware, no nRF benefits.

## Implementation Plan

1. **Add sniff group under zigctl** with `cmd/zigctl/sniff/root.go` and per-verb files.
2. **Implement nRF serial backend**
   - Port extcap serial protocol, add tests with recorded fixtures.
3. **pcapng writer + TAP metadata**
   - Emit pcapng with linktype 283 and metadata.
4. **Glazed command wiring (zigctl)**
   - Implement `WriterCommand` for capture/live and `GlazeCommand` for decode/summarize within the sniff subgroup.
5. **CLI sniff capture/live commands**
   - Wire flags, `io.Writer` plumbing, and stdout streaming.
6. **tshark decode integration**
   - Add `decode` and `summarize` commands.
7. **ZEP backend** (optional)
   - Add UDP listener and conversion to frames.
8. **Docs + samples**
   - Provide example commands and expected outputs.

## Open Questions

- Confirm the exact nRF sniffer serial framing (validate against current extcap script version).
- Should we support multiple simultaneous channels (multi-sniffer) or stick to single-channel MVP?
- Do we want to output ZEP as an option, in addition to pcapng?

## Testing Strategy

- Unit tests for serial framing parse/build functions.
- Golden pcapng fixtures (compare frame counts + metadata).
- Integration test: `zigctl sniff decode pcap` on a known capture and compare summary outputs.

## Dependencies

- `github.com/hatching/gopacket/pcapgo` for pcapng writing.
- `go.bug.st/serial` or equivalent for serial port access.
- `github.com/go-go-golems/glazed` and `github.com/spf13/cobra` for CLI structure and structured output.
- System dependency: `tshark` (optional but recommended).

## Related Sources

- Imported notes: `sources/local/zigbee-sniffing.md` (ticket source import).
- Reference design: `ttmp/2026/01/31/0067-zigbee-powerplug--zigbee2mqtt-third-reality-gen-2-plug/design-doc/01-zigbee-cli-tool-design-zigctl.md` (zigctl Glazed CLI patterns).
