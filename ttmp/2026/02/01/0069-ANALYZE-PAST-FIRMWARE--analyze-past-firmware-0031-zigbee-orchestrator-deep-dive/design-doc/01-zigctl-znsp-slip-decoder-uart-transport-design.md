---
Title: 'Zigctl: ZNSP SLIP Decoder + UART Transport (Design)'
Ticket: 0069-ANALYZE-PAST-FIRMWARE
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp32s3
    - esp32h2
    - esp32
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/reference/02-esp32-h2-ncp-firmware-and-znsp-protocol-host-integration-reference.md
      Note: Authoritative protocol details used by this design.
    - Path: zigctl/cmd/listen/raw.go
      Note: Existing raw output pattern for streaming data.
    - Path: zigctl/cmd/root.go
      Note: Entry point for wiring new command groups.
    - Path: zigctl/pkg/zigbee/layer.go
      Note: Pattern for adding zigctl configuration layers.
ExternalSources: []
Summary: Design for adding ZNSP SLIP framing/decoding and UART transport to zigctl with a Go API and request/response flow.
LastUpdated: 2026-02-02T11:40:04-05:00
WhatFor: Guide the zigctl implementation of UART ZNSP framing/decoding and request/response handling.
WhenToUse: Before implementing a zigctl-native host path to the ESP32-H2 Zigbee NCP via SLIP over UART.
---


# Zigctl: ZNSP SLIP Decoder + UART Transport (Design)

## Executive Summary

Add a new zigctl transport and protocol layer for ESP32-H2 Zigbee NCP devices that speak ZNSP over SLIP/UART. The design defines a Go-native framing/decoding library, a UART transport with buffering, and a request/response session API (including timeouts and notify handling). The goal is to let zigctl talk directly to a Zigbee NCP from a host computer without MQTT, while matching the actual firmware behavior documented in the H2 NCP reference.

## Problem Statement

Zigctl today targets Zigbee2MQTT (MQTT broker) workflows. For ESP32-H2 NCP development and debugging, we also need a direct host path over UART that can:

- Encode and decode ZNSP frames with SLIP framing and CRC16.
- Handle stream fragmentation and boundary detection.
- Provide a request/response API with timeouts and optional concurrency.
- Surface asynchronous notify frames (ZNSP type=2) to callers.

Without this, developers rely on embedded host firmware or bespoke scripts, which makes protocol debugging slower and harder to integrate into zigctl tooling.

## Proposed Solution

### Scope and Architecture

Introduce a new `zigctl/pkg/znsp` package family that implements:

1) SLIP framing/deframing for byte streams.
2) ZNSP frame packing/unpacking (header, payload, CRC16).
3) A session layer for request/response and notifications.
4) A UART transport adapter to feed the decoder and write encoded frames.

High-level data flow:

```
UART bytes -> SlipDecoder -> ZNSPFrameParser -> Session (notify + response) -> zigctl commands
zigctl commands -> Session -> ZNSPFrameEncoder -> SlipEncoder -> UART write
```

### Package Layout (Proposed)

```
zigctl/pkg/znsp/
  frame/        # Header, Frame, CRC, encode/decode
  slip/         # Streaming SLIP encoder/decoder
  session/      # Request/response tracking, notify fan-out
  transport/    # UART transport (serial port config, reader loop)
```

### On-Wire Behavior (from firmware)

- SLIP special bytes: 0xC0 END, 0xDB ESC, 0xDC ESC_END, 0xDD ESC_ESC.
- Encoder prepends an END and terminates with END.
- Frame layout (packed): flags(2) + id(2) + sn(1) + len(2) + payload + crc16(2).
- CRC16 = `crc16_le(0xFFFF, header+payload)`; 16-bit appended in little-endian.
- Frame type: 0=request, 1=response, 2=notify.
- Host example matches responses by ID only (not sequence). Zigctl should improve by matching `(id, sn)` and optionally allow ID-only mode to mirror firmware behavior.

### Zigctl Integration Surface

Add a new command namespace (example):

- `zigctl ncp info` (query version, get current channel, etc.)
- `zigctl ncp znsp req <id> [bytes...]` (raw requests)
- `zigctl ncp monitor` (stream notify frames)

Configuration options (new zigctl layer or CLI flags):

- `--ncp-port` (serial device path)
- `--ncp-baud` (default 115200)
- `--ncp-timeout` (request timeout)
- `--ncp-slip-max-frame` (default 1024; matches firmware buffer)

Note: If we need a Glazed layer for these, follow existing zigctl patterns for `zigbee` settings.

## Design Decisions

1) **Separate SLIP and ZNSP framing packages.**
   - Rationale: isolates stream decoding from protocol parsing, supports reuse in other tools.

2) **Streaming decoder API rather than single-shot.**
   - Rationale: UART data arrives in arbitrary chunks; the decoder must accept partial input.

3) **Session layer owns request correlation.**
   - Rationale: keep caller APIs simple; centralize timeouts, concurrency, and response matching.

4) **Default to strict `(id, sn)` matching, with a compatibility flag for `id`-only.**
   - Rationale: correctness; optional compatibility with the embedded host example.

5) **Expose raw notify frames and parsed notify events.**
   - Rationale: notify frames are critical for debugging; callers may want raw payloads.

## Proposed API (Sketch)

### Core Types

```go
// zigctl/pkg/znsp/frame

type FrameType uint8
const (
    FrameRequest FrameType = 0
    FrameResponse FrameType = 1
    FrameNotify FrameType = 2
)

type Header struct {
    Version uint8  // 4-bit on wire
    Type    FrameType
    ID      uint16
    Seq     uint8
    Len     uint16
}

type Frame struct {
    Header  Header
    Payload []byte
    CRC     uint16
    Raw     []byte // optional: decoded bytes (header+payload+crc)
}
```

### SLIP Decoder/Encoder

```go
// zigctl/pkg/znsp/slip

type Decoder struct {
    MaxFrame int
    // internal state: buffer, escaped, etc.
}

// Feed returns zero or more complete SLIP payloads (without END bytes).
func (d *Decoder) Feed(input []byte) ([][]byte, error)

func Encode(payload []byte) []byte
```

### ZNSP Frame Decode/Encode

```go
// zigctl/pkg/znsp/frame

func Decode(payload []byte) (*Frame, error) // expects header+payload+crc
func Encode(frame *Frame) ([]byte, error)   // returns header+payload+crc
```

### Session + Transport

```go
// zigctl/pkg/znsp/session

type Response struct {
    Frame *frame.Frame
    Err   error
}

type Session struct {
    // owns transport and decoder; manages pending requests
}

type Config struct {
    Timeout         time.Duration
    MatchByIDOnly   bool
    MaxFrame        int
}

func NewSession(t transport.Transport, cfg Config) *Session

func (s *Session) Request(ctx context.Context, id uint16, payload []byte) (*frame.Frame, error)
func (s *Session) NotifyChan() <-chan *frame.Frame
func (s *Session) Close() error
```

```go
// zigctl/pkg/znsp/transport

type Transport interface {
    Read(p []byte) (int, error)
    Write(p []byte) (int, error)
    Close() error
}

type UARTConfig struct {
    Port string
    Baud int
    ReadTimeout time.Duration
}

func OpenUART(cfg UARTConfig) (Transport, error)
```

## Pseudocode

### Streaming SLIP Decoder

```text
state: buf = [], escaped = false
for each byte b in input:
  if b == END:
    if buf not empty:
      emit buf; buf = []
    else:
      continue  // ignore empty frame
  else if b == ESC:
    escaped = true
  else if escaped:
    if b == ESC_END: buf += END
    else if b == ESC_ESC: buf += ESC
    else buf += b // protocol violation, keep raw
    escaped = false
  else:
    buf += b
  if len(buf) > MaxFrame: error
```

### ZNSP Frame Decode

```text
if len(payload) < headerSize+crcSize: error
parse header (packed, little endian)
if header.len + headerSize + crcSize > len(payload): error
calc crc16_le(0xFFFF, header+payload)
if crc != expected: error
return Frame{Header, Payload, CRC}
```

### Session Request Flow

```text
Request(id, payload):
  seq = nextSeq()
  frame = build request frame
  bytes = slip.Encode(frame.Encode())
  send over transport
  wait for response matching (id, seq) or timeout
  return response

readerLoop:
  read bytes from transport
  slipDecoder.Feed()
  for each slipPayload:
    frame = znsp.Decode(slipPayload)
    if frame.Type == Notify: send to notify chan
    else: match to pending request
```

## Alternatives Considered

1) **Reuse an existing SLIP library.**
   - Rejected: We need strict control of framing behavior (initial END, max frame size, protocol violations) and CRC handling.

2) **Match responses by ID only (like embedded host).**
   - Rejected as default: Safer to correlate by sequence. ID-only remains as compatibility mode.

3) **Use a single goroutine without a session layer.**
   - Rejected: Makes higher-level zigctl commands brittle and inconsistent, especially for notify handling.

## Implementation Plan

1) Add `zigctl/pkg/znsp/slip` with streaming decoder + encoder.
2) Add `zigctl/pkg/znsp/frame` with header parsing, CRC16, and encode/decode.
3) Add `zigctl/pkg/znsp/transport` with UART implementation.
4) Add `zigctl/pkg/znsp/session` with request/response and notify channel.
5) Add zigctl command(s): `zigctl ncp znsp req`, `zigctl ncp monitor`.
6) Add tests:
   - SLIP decode/encode round-trip.
   - Frame decode/encode + CRC errors.
   - Session request timeout + notify fan-out.
7) Validate with real ESP32-H2 NCP device.

## Open Questions

- Do we need SPI transport support in zigctl (future NCP devices)?
- Should we support multi-frame SLIP payloads (host example can parse multiple frames per decode)?
- Do we need to persist sequence numbers across restarts or keep them ephemeral?
- Should zigctl expose a raw hex dump mode for protocol debugging?

## References

- `ttmp/2026/02/01/0069-ANALYZE-PAST-FIRMWARE--analyze-past-firmware-0031-zigbee-orchestrator-deep-dive/reference/02-esp32-h2-ncp-firmware-and-znsp-protocol-host-integration-reference.md`
- `0031-zigbee-orchestrator/components/zb_host/src/esp_host_frame.c`
- `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_frame.c`
- `zigctl/pkg/zigbee/client.go`
