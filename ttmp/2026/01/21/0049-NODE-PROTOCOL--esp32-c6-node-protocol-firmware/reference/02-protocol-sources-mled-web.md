---
Title: Protocol Sources (mled-web)
Ticket: 0049-NODE-PROTOCOL
Status: active
Topics:
    - esp32c6
    - wifi
    - console
    - esp-idf
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 2026-01-21--mled-web/c/mledc/node.c
      Note: C node state machine (epoch gating
    - Path: 2026-01-21--mled-web/c/mledc/protocol.h
      Note: Wire structs and constants (ground truth sizes)
    - Path: 2026-01-21--mled-web/c/mledc/time.c
      Note: Wrap-around-safe u32 show-clock math
    - Path: 2026-01-21--mled-web/c/mledc/transport_udp.c
      Note: UDP multicast join/send/recv implementation
    - Path: 2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/design-doc/01-python-app-protocol-review.md
      Note: Detailed Python-side wire layout + behavioral semantics
    - Path: 2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/sources/local/protocol.md (imported).md
      Note: Imported protocol notes (two-phase cue + ping/pong + header layout)
ExternalSources: []
Summary: Primary sources for the MLED/1 UDP multicast protocol and its C/Python implementations (wire format + node behavior).
LastUpdated: 2026-01-21T14:46:04.575938313-05:00
WhatFor: Provide a curated, copy/paste-ready index of authoritative protocol sources used to design/port MLED/1 onto ESP32-C6.
WhenToUse: Use when implementing/porting the protocol, debugging wire captures, or reconciling spec-vs-implementation behavior.
---



# Protocol Sources (mled-web)

## Goal

Collect the authoritative MLED/1 protocol sources (markdown notes + Python review + C implementation) and summarize the parts that matter when implementing an ESP32-C6 firmware node.

## Context

“MLED/1” is a UDP multicast protocol for synchronized multi-node LED control. The core idea is a **two-phase cue**:

1) `CUE_PREPARE` (per-node, not time critical): store pattern config for cue X.
2) `CUE_FIRE` (time critical, multicast): execute cue X at show-time T.

Discovery/status uses `PING` (multicast) and `PONG` (unicast response containing node state). Time synchronization uses:
- **BEACON** (multicast): coarse sync; carries controller show-time in header field `execute_at_ms`.
- Optional **TIME_REQ/TIME_RESP** (unicast): refinement; Cristian-style half-RTT correction.

For this ticket, treat the implementations as ground truth:
- The Python implementation review documents the intended wire structs and flows and calls out gaps.
- The C implementation (`mledc/`) is a minimal node that joins multicast, gates on epoch, schedules fires, and replies with PONG/ACK.

## Quick Reference

### Transport constants (Python + C)

- Multicast group: `239.255.32.6`
- UDP port: `4626`
- Multicast TTL: `1`

### Datagram framing

```text
[ 32-byte header ][ payload_len bytes payload ]
```

### Header (32 bytes, little-endian)

```text
0  .. 3   magic           = "MLED"
4         version         = 1
5         type            = message type (u8)
6         flags           = target mode + ACK_REQ bit (u8)
7         hdr_len         = 32
8  .. 11  epoch_id        (u32)
12 .. 15  msg_id          (u32)
16 .. 19  sender_id       (u32)  // controller=0, node=node_id
20 .. 23  target          (u32)  // depends on target mode
24 .. 27  execute_at_ms   (u32)  // show-time timestamp; 0 => ASAP (convention)
28 .. 29  payload_len     (u16)
30 .. 31  reserved        (u16)
```

### Flags (u8)

- bits `0..1`: target mode
  - `0`: ALL
  - `1`: NODE (target is node_id)
  - `2`: GROUP (target is group_id)
- bit `2`: `ACK_REQ` (0x04)

### Message types + payload sizes (implemented)

| Type | Hex | Direction | Payload | Size |
|---|---:|---|---|---:|
| `BEACON` | 0x01 | controller → multicast | none | 0 |
| `TIME_REQ` | 0x03 | node → controller (unicast) | none | 0 |
| `TIME_RESP` | 0x04 | controller → node (unicast) | `req_msg_id,u32 master_rx_show_ms,u32 master_tx_show_ms` | 12 |
| `CUE_PREPARE` | 0x10 | controller → multicast | cue + pattern config | 28 |
| `CUE_FIRE` | 0x11 | controller → multicast | `cue_id` | 4 |
| `CUE_CANCEL` | 0x12 | controller → multicast | `cue_id` | 4 |
| `PING` | 0x20 | controller → multicast | none | 0 |
| `PONG` | 0x21 | node → controller (unicast) | node status | 43 |
| `ACK` | 0x22 | node → controller (unicast) | ack for controller msg_id | 8 |

### PatternConfig payload (20 bytes)

```text
0        pattern_type     (u8)
1        brightness_pct   (u8)
2        flags            (u8)  // bit0 seed_valid (convention)
3        reserved0        (u8)
4..7     seed             (u32)
8..19    data[12]         (12 bytes; type-specific)
```

### CUE_PREPARE payload (28 bytes)

```text
0..3     cue_id           (u32)
4..5     fade_in_ms       (u16)
6..7     fade_out_ms      (u16)
8..27    pattern_config   (20 bytes)
```

### Node scheduling invariant (wrap-around-safe show clock)

Both Python and C treat show time as `u32` milliseconds and compare timestamps using wrap-around-safe signed-diff logic. A correct ESP32 port must preserve this invariant (or expand to 64-bit time end-to-end).

## Usage Examples

### “Two-phase cue” from a controller’s perspective

```text
for each node:
  send multicast CUE_PREPARE (target_mode=NODE, target=node_id), request ACK

send multicast CUE_FIRE (target_mode=ALL, execute_at_ms = show_ms + lead_time_ms)
```

### A node’s minimal loop (conceptual)

```text
join multicast group
while running:
  process_due_fires()
  pkt = recv(timeout = next_due_or_500ms)
  if pkt.type == BEACON: update epoch + coarse offset + controller_addr; maybe TIME_REQ
  if pkt.type == PING:  send PONG unicast to src
  if epoch matches: handle CUE_PREPARE / CUE_FIRE / CUE_CANCEL / TIME_RESP
```

## Related

### Imported protocol notes

- `2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/sources/local/protocol.md (imported).md`

### Python implementation review (wire tables + semantics)

- `2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/design-doc/01-python-app-protocol-review.md`

### C reference node implementation (portable behavior reference)

- `2026-01-21--mled-web/c/mledc/protocol.h`
- `2026-01-21--mled-web/c/mledc/protocol.c`
- `2026-01-21--mled-web/c/mledc/node.h`
- `2026-01-21--mled-web/c/mledc/node.c`
- `2026-01-21--mled-web/c/mledc/transport_udp.h`
- `2026-01-21--mled-web/c/mledc/transport_udp.c`
- `2026-01-21--mled-web/c/mledc/time.h`
- `2026-01-21--mled-web/c/mledc/time.c`
