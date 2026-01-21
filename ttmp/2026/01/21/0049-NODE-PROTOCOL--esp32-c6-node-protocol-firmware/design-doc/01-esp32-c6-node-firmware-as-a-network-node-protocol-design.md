---
Title: ESP32-C6 Node Firmware as a Network Node (Protocol + Design)
Ticket: 0049-NODE-PROTOCOL
Status: active
Topics:
    - esp32c6
    - wifi
    - console
    - esp-idf
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/2026-01-21--mled-web/c/mledc/node.c
      Note: Minimal C reference node state machine (epoch gating, time sync, cue scheduling, PING/PONG)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/2026-01-21--mled-web/c/mledc/protocol.h
      Note: Wire structs/constants and payload sizes (ground truth)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0045-xiao-esp32c6-preact-web/main/wifi_mgr.c
      Note: Proven ESP32-C6 Wi-Fi STA manager with NVS creds + got-IP callback
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0045-xiao-esp32c6-preact-web/main/wifi_console.c
      Note: Proven esp_console Wi-Fi command UX (scan/join/save) for ESP32-C6
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.c
      Note: Single-owner task + queue control surface (good pattern for applying cues safely)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/main/led_task.h
      Note: Control-plane message types + status snapshot shape (maps to node status / PONG)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-033-ESP32C6-PREACT-WEB--esp32-c6-device-hosted-preact-zustand-web-ui-with-esp-console-wi-fi-selection/design-doc/01-design-esp32-c6-console-configured-wi-fi-device-hosted-preact-zustand-counter-ui-embedded-assets.md
      Note: Detailed design for console-configured Wi-Fi selection (reusable patterns for this ticket)
ExternalSources: []
Summary: "Textbook-style design for an ESP32-C6 firmware that provisions Wi‑Fi via esp_console and implements the MLED/1 UDP multicast node protocol (epoch + time sync + prepare/fire cues + ping/pong discovery), grounded in existing C/Python protocol sources and prior repo firmwares."
LastUpdated: 2026-01-21T14:46:04.170354387-05:00
WhatFor: "Provide a detailed, implementable blueprint (architecture + data structures + task model + pseudocode + diagrams) for porting MLED/1 onto ESP32‑C6 as a Wi‑Fi-connected network node."
WhenToUse: "Use when implementing an ESP32‑C6 node firmware, reviewing protocol correctness, or aligning controller and node behavior across languages (Python/C/ESP-IDF)."
---

# ESP32-C6 Node Firmware as a Network Node (Protocol + Design)

## Executive Summary

We want an ESP32‑C6 firmware that:

1) **Provisions Wi‑Fi STA credentials at runtime** via `esp_console` (scan → join → optional NVS save).
2) **Joins a UDP multicast group** and implements a lightweight node protocol (“MLED/1”) so a controller can discover nodes, synchronize their clocks, and trigger synchronized cues.
3) **Presents itself as a node** by responding to `PING` with `PONG` (unicast) and by accepting a two-phase cue workflow:
   - `CUE_PREPARE` (per node, not time critical): store a pattern config for cue X.
   - `CUE_FIRE` (multicast, time critical): execute cue X at show-time T.

This document is deliberately “textbook-style”: it explains the *why* behind the design (distributed systems, clocks, idempotency, and the hazards of UDP), then provides a concrete ESP‑IDF architecture with **task diagrams**, **state machines**, and **pseudocode** that mirrors the existing C reference implementation and prior repo firmware patterns.

The implementation strategy is “compose proven parts”:
- Wi‑Fi STA + console provisioning: reuse patterns from `0045-xiao-esp32c6-preact-web/` (derived from `0029-mock-zigbee-http-hub/`).
- Cue application and concurrent state safety: reuse the **single-owner task + queue** pattern from `0044-xiao-esp32c6-ws281x-patterns-console/`.
- Protocol semantics and wire sizes: follow `2026-01-21--mled-web/c/mledc/` and the Python protocol review docs.

## Problem Statement

We need a firmware “node” that can be dropped into a room, provisioned onto Wi‑Fi without reflashing, and then controlled as part of a multi-node system. The system must solve three problems that are easy to underestimate:

1) **Provisioning:** how does an operator give the node a network identity (SSID/password) without recompiling firmware or hardcoding secrets?
2) **Discovery:** how does a controller find all nodes on the LAN (and display their status) without maintaining a static list of IPs?
3) **Synchronization:** how can a controller make multiple nodes perform an action “at the same time” within the tolerances of commodity Wi‑Fi and UDP?

Constraints:
- The node must run on **ESP32‑C6**, using **ESP‑IDF** (console, Wi‑Fi STA, and sockets via lwIP).
- Control messages are over **UDP multicast** (cheap broadcast) and **unicast replies** (avoid reply storms).
- The protocol should remain compact and fixed-layout, suitable for embedded devices: avoid JSON and large dynamic parsing.
- The protocol should tolerate:
  - packet loss,
  - duplicates,
  - message reordering,
  - clock drift,
  - node reboots and controller restarts.

Finally, we want the firmware architecture to be easy to reason about: avoid “many tasks mutating shared state”; prefer clear ownership, bounded queues, and observable status.

## Proposed Solution

### 0. System model: one controller, many nodes

We model the system as a “local show network”:
- One **controller** (PC or Raspberry Pi) sends multicast control messages.
- Many **nodes** (ESP32‑C6) listen on the multicast group and perform actions.

We assume a typical home/venue network:
- Wi‑Fi is shared medium with variable latency and occasional packet loss.
- UDP multicast is available within one hop (TTL=1).

### 1. Protocol as a set of invariants

The protocol is built around three invariants:

1) **Epoch establishes authority.**
   - The controller has an `epoch_id` (changes on controller restart).
   - Nodes accept time/cue messages only when `hdr.epoch_id` matches their current controller epoch.

2) **Show time is a logical time base used for scheduling.**
   - Nodes maintain `show_ms = local_ms + offset_ms`.
   - The offset is learned from `BEACON` and refined via `TIME_REQ/TIME_RESP`.

3) **Two-phase cues separate “configuration” from “synchronization”.**
   - `CUE_PREPARE` distributes per-node pattern configs early.
   - `CUE_FIRE` is one small multicast packet that schedules execution at a shared show-time.

These invariants are what make the system *work despite UDP*.

### 2. Firmware architecture: explicit ownership and small interfaces

We split the firmware into four conceptual subsystems:

1) **Console control plane** (`esp_console`):
   - exposes `wifi ...` commands (scan/join/save/status),
   - exposes `node ...` commands (status/debug/start/stop).

2) **Wi‑Fi manager** (STA state machine):
   - configures Wi‑Fi and persists credentials to NVS,
   - signals “got IP” and “lost IP”.

3) **Node protocol task** (UDP networking + scheduler):
   - owns sockets, join multicast, parse packets,
   - owns protocol state: epoch, controller addr, time offset, cue store, pending fires,
   - runs a loop combining `select()` receive with “next due fire” timeouts.

4) **Effect task** (e.g., LED patterns):
   - owns hardware (WS281x, display, etc.),
   - receives “apply pattern config” messages from the node task through a bounded queue.

The key architectural decision is that the **node task never directly mutates the effect task’s internal state**. It sends a message, and the effect task applies it. This is the same single-owner pattern already used for LED patterns in `0044`.

### 3. Diagrams: how the pieces fit

#### 3.1 Task diagram (ownership and message passing)

```text
                    +---------------------------+
USB Serial/JTAG     |      esp_console REPL     |
 (interactive) ---> |  commands: wifi, node     |
                    +-------------+-------------+
                                  |
                                  | calls APIs (safe, nonblocking)
                                  v
                    +---------------------------+
                    |        Wi-Fi manager      |
                    |  (STA state + NVS creds)  |
                    +-------------+-------------+
                                  |
            got-IP / lost-IP      | callbacks/events
                                  v
                    +---------------------------+
                    |     Node protocol task    |
                    | UDP sockets + scheduler   |
                    | epoch/time/cues/fires     |
                    +-------------+-------------+
                                  |
                                  | queue messages (apply pattern)
                                  v
                    +---------------------------+
                    |     Effect owner task     |
                    | (e.g. WS281x patterns)    |
                    +---------------------------+
```

#### 3.2 Sequence diagram: discovery

```text
Controller                     Node
----------                     ----
multicast PING  -------------> (recv PING)
                               build PONG status
unicast  PONG  <-------------  sendto(src_ip:src_port)
```

#### 3.3 Sequence diagram: two-phase cue

```text
Controller                            Node
----------                            ----
multicast CUE_PREPARE (to node_id) --> store cue[cue_id]=pattern
                                      (optional ACK unicast)
multicast CUE_FIRE execute_at=T  ----> schedule (cue_id,T)
...
at show_ms ~= T                         apply cue[cue_id] to effect task
```

### 4. Concrete design: the ESP32-C6 node as a state machine

At any moment the node is in a composite state:

- Wi‑Fi: `DISCONNECTED | CONNECTING | CONNECTED(ip)`
- Controller epoch: `UNKNOWN | KNOWN(epoch_id, controller_addr)`
- Time sync: `UNSYNCED | COARSE(BEACON) | REFINED(TIME_REQ)`
- Cue store: set of `(cue_id -> pattern_cfg)`
- Pending fires: set of `(cue_id, execute_at_show_ms)`

We will implement this state machine explicitly inside the node task (in one struct), and make the control plane (`node status`) print a snapshot.

### 5. The “textbook” part: why these choices work

#### 5.1 Why multicast + unicast replies?

Multicast is the “one packet reaches all listeners” mechanism; it is efficient for `CUE_FIRE`.
But if all nodes replied via multicast, the network would see a reply storm. Unicast replies are a simple rule:

- **Controller shouts** on multicast.
- **Nodes whisper** back unicast.

This design also makes controllers easier: they can listen on one UDP port and see individual node addresses naturally.

#### 5.2 Why not “send the full pattern config at fire time”?

If `CUE_FIRE` carried large payloads, the synchronized “go moment” would become fragile:
- larger packets are more likely to be dropped,
- processing time at the receiver is higher and more variable,
- encoding/decoding becomes more expensive exactly when timing matters.

Splitting into prepare/fire is a classic real-time systems trick: move work earlier when time is not critical.

#### 5.3 Why not “just use wall-clock time (NTP)”?

Wall-clock time is a global service, but it adds dependencies:
- the network must provide NTP or you must run your own,
- nodes must be configured with time zones/RTC semantics,
- and you still have to handle drift and latency.

In contrast, the show clock is a local logical clock designed solely for scheduling cues. It is simpler: one controller defines it, nodes follow it, and it resets with the epoch.

#### 5.4 Timing error budget: what “sync” means on Wi‑Fi

To reason about synchronized firing, we need a simple error model. Let:

- `T*` be the controller’s intended show-time for a cue.
- `Tnode` be the node’s local estimate of show-time at the moment it fires.

The node fires when `show_ms(node) >= T*`. The real-world timing error is approximately:

```text
error ≈ clock_offset_error + local_scheduler_jitter + effect_apply_latency
```

Where:
- `clock_offset_error` comes from imperfect time sync (BEACON/TIME_REQ), dominated by Wi‑Fi one-way delay variability.
- `local_scheduler_jitter` comes from task scheduling and how often the node checks “is it due?” (frame cadence / select timeout).
- `effect_apply_latency` is how long it takes to enqueue/apply the new pattern (queue delay + render loop boundary).

Two-phase cues are a strategy for minimizing the *variance* of the “go” moment:
- All heavy payload parsing happens in `CUE_PREPARE`, outside the critical window.
- `CUE_FIRE` is tiny; it is processed quickly.
- The node can schedule fire using its local clock rather than “when packet arrives”.

Practical guidance (controller-side):
- Send `CUE_FIRE` with a small lead time (e.g., `T* = now_show + 200ms`) rather than “ASAP”.
  - This converts network delay uncertainty into a fixed-latency schedule on the node.
- Optionally retransmit `CUE_FIRE` 2–3 times (same cue_id / execute_at), spaced by ~20–50ms, to mitigate packet loss.

Practical guidance (node-side):
- Clamp select/receive timeout to a small bound (the C reference uses 500ms; for tighter sync you might use 10–25ms near the due window).
- Ensure the effect task can apply the cue without blocking (queue + single owner task is ideal).

### 6. ESP32-C6 implementation details (pseudocode + diagrams)

#### 6.1 Node identity

We need two identity fields:
- `node_id` (u32): stable-ish identifier used in headers and targeting.
- `name[16]`: human-friendly label sent in PONG.

Recommended on ESP32:
- derive `node_id` from the STA MAC address (last 4 bytes, little-endian) or a hash:

```text
node_id = fnv1a32(mac_bytes[0..5])
```

Expose console commands to override/persist these in NVS if needed:
- `node id set <u32> [--save]`
- `node name set <string> [--save]`

#### 6.2 Wi‑Fi provisioning via esp_console

Adopt the proven command UX from `0045-xiao-esp32c6-preact-web/main/wifi_console.c`:

```text
wifi status
wifi scan [max]
wifi join <index> <password> [--save]
wifi set --ssid <ssid> --pass <password> [--save]
wifi connect
wifi disconnect
wifi clear
```

Important implementation constraints (from prior tickets):
- Never print the password.
- Keep “runtime creds” (RAM) separate from “saved creds” (NVS).
- Avoid symbol collisions with ESP‑IDF by prefixing APIs (e.g., `node_wifi_*` rather than `wifi_sta_*`).

Pseudocode: Wi‑Fi manager start

```text
wifi_mgr_start():
  ensure_nvs()
  ensure_netif_and_default_event_loop()
  create default STA netif
  esp_wifi_init()
  register WIFI_EVENT and IP_EVENT handlers
  esp_wifi_set_mode(STA)

  if NVS creds exist:
    load into runtime slot
  if autoconnect enabled and runtime creds exist:
    apply runtime config
  esp_wifi_start()
```

#### 6.3 UDP sockets on ESP‑IDF (lwIP)

We need the node to:
- bind UDP `:4626`,
- join multicast group `239.255.32.6`,
- receive multicast control packets,
- send unicast replies (PONG/ACK/TIME_REQ).

Conceptual pseudocode:

```text
sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)
setsockopt(sock, SO_REUSEADDR, 1)
bind(sock, INADDR_ANY:4626)

// join group
mreq.imr_multiaddr = inet_addr("239.255.32.6")
mreq.imr_interface = INADDR_ANY  // or sta_ip for stricter selection
setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq)

// limit outbound multicast scope
setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, 1)
```

On ESP32, a practical detail matters: if the station disconnects and reconnects, multicast membership may be lost. The robust approach is:
- on got-IP: (re)create socket and join group,
- on lost-IP/disconnect: close socket.

#### 6.4 Packet parsing (wire safety)

UDP datagrams are untrusted input. The parser must be strict:
- reject packets shorter than 32 bytes,
- validate magic/version/hdr_len,
- ensure `payload_len` does not exceed received length,
- ignore unknown types.

Pseudocode:

```text
parse_packet(buf,n):
  if n < 32: return DROP
  hdr = unpack_header_le(buf[0..31])
  if !validate(hdr): return DROP
  if 32 + hdr.payload_len > n: return DROP
  payload = buf[32 .. 32+payload_len)
  return (hdr,payload)
```

We strongly recommend **byte-level read/write helpers** (as in `mledc/protocol.c`) rather than casting packed structs. This avoids alignment pitfalls and makes endian conversions explicit.

#### 6.5 Node protocol task: run loop and epoch gating

The node task runs a loop that interleaves:
- “process due fires” (time-based actions),
- “receive next packet with timeout until next due fire (clamped)”.

This is exactly the structure in the C node implementation (`mledc/node.c`).

Pseudocode (high fidelity):

```text
node_task_main():
  while running:
    process_due_fires()
    timeout_ms = next_due_timeout_ms(clamp 0..500)

    pkt = recvfrom(sock, timeout_ms)
    if timeout: continue
    (hdr,payload) = parse_packet(pkt)

    // accept regardless of epoch
    if hdr.type == BEACON:
      handle_beacon(hdr, src)
      continue
    if hdr.type == PING:
      handle_ping(hdr, src)   // reply PONG unicast
      continue

    // epoch gating
    if controller_epoch == 0 or hdr.epoch_id != controller_epoch:
      continue

    switch hdr.type:
      TIME_RESP:   handle_time_resp(payload)
      CUE_PREPARE: handle_cue_prepare(hdr,payload,src)  // optional ACK
      CUE_FIRE:    handle_cue_fire(hdr,payload)
      CUE_CANCEL:  handle_cue_cancel(payload)
      default:     ignore

    process_due_fires()
```

#### 6.6 Time sync: BEACON + TIME_REQ refinement

**Coarse sync** uses `BEACON.execute_at_ms`:

```text
offset_ms = signed_diff(master_show_ms, local_ms)
show_ms = local_ms + offset_ms
```

**Refined sync** uses `TIME_REQ/TIME_RESP`:
- node sends `TIME_REQ` with `msg_id = req_id`,
- controller replies with `TIME_RESP` containing:
  - `req_msg_id`,
  - `master_rx_show_ms` (controller show time when request arrived),
  - `master_tx_show_ms` (controller show time when response sent).

Node computes:

```text
rtt = t3_local_ms - t0_local_ms
srv_proc = master_tx_show_ms - master_rx_show_ms
effective = max(0, rtt - srv_proc)
one_way = effective / 2
estimated_master_at_t3 = master_tx_show_ms + one_way
offset_ms = signed_diff(estimated_master_at_t3, t3_local_ms)
```

This is a classic Cristian-style correction with a rough server-processing subtraction.

Engineering advice (ESP-specific):
- Update offsets carefully: applying a big offset jump can cause scheduled fires to “jump across” the due threshold.
- A safe rule is: use `BEACON` to set epoch and an initial offset, then only *refine* with TIME_REQ when the RTT sample is small (min-of-N heuristic).

#### 6.7 Cue store and mapping to an effect engine

The protocol stores a “wire-format” pattern config (`PatternConfig`) which has fixed 12 bytes of type-specific parameters. The firmware should translate this into an internal config for its effect engine.

For WS281x patterns, `0044-xiao-esp32c6-ws281x-patterns-console/` already defines a nice internal schema. The mapping is straightforward:

```text
wire.pattern_type   -> led_pattern_type_t
wire.brightness_pct -> led_pattern_cfg_t.global_brightness_pct
wire.data[]         -> pattern-specific struct fields
```

Example: mapping `RAINBOW` (wire) to `led_rainbow_cfg_t` (internal)

```text
wire.data[0] = speed (0..20)       -> cfg.speed
wire.data[1] = saturation (0..100) -> cfg.saturation
wire.data[2] = spread_x10 (1..50)  -> cfg.spread_x10
```

The design goal is to keep the mapping *boring and explicit*, and to keep it in one place (`protocol_to_led_cfg()`), so that all future patterns are added consistently.

#### 6.8 Applying cues safely (queue boundary)

When a fire becomes due, the node task should send exactly one message to the effect task:

```text
led_msg = { type = LED_MSG_SET_PATTERN_CFG, pattern = cfg }
led_task_send(&led_msg)
```

This enforces a key concurrency invariant:

> The effect task owns hardware + animation state; the node task never touches those directly.

#### 6.9 Message-by-message behavior: a precise node “spec” (derived from code)

This section tightens the protocol into a node-side behavioral contract. Think of it as the “reference node spec” you would hand to someone implementing the same firmware in another language.

##### 6.9.1 Common receive rules

For every received datagram:

1) Parse + validate header.
2) If `hdr.payload_len` exceeds received length, drop.
3) Dispatch by `hdr.type`.

Additionally:
- Maintain a **seen-set** for deduplication keyed by `(epoch_id,msg_id,sender_id,type)` if desired. (The C reference does not dedup, but it is a common enhancement.)
- Never block in the receive handler; any heavy work should be “scheduled” or “queued”.

##### 6.9.2 `BEACON` (controller → multicast)

Semantics:
- Establish controller authority (epoch) and a baseline show-time mapping.

Rules:
- Accept `BEACON` regardless of current epoch.
- If `epoch_id` changes (or current epoch unknown):
  - set `controller_epoch = hdr.epoch_id`,
  - record `controller_addr = src`,
  - clear cue store and pending fires,
  - clear “active cue” metadata,
  - clear any dedup caches.
- Always update controller address to the latest beacon source.
- Set coarse time offset:
  - `offset_ms = signed_diff(hdr.execute_at_ms, local_ms())`
  - mark `time_synced = true`, `method = "BEACON"`.
- Optionally trigger a `TIME_REQ` to refine the offset.

Pseudocode:

```text
handle_beacon(hdr, src):
  if controller_epoch == 0 or hdr.epoch_id != controller_epoch:
    controller_epoch = hdr.epoch_id
    controller_addr = src
    clear(cues)
    clear(pending_fires)
    active_cue_id = 0

  controller_addr = src
  offset_ms = signed_diff(hdr.execute_at_ms, local_ms())
  time_synced = true
  last_sync_method = "BEACON"
  maybe_send_time_req()
```

##### 6.9.3 `PING` (controller → multicast)

Semantics:
- Discovery + status request.

Rules:
- Accept regardless of epoch (this supports discovery even before BEACON).
- Reply via unicast `PONG` to the ping sender’s source IP/port.
- Use `hdr.msg_id` as correlation id; set `PONG.hdr.msg_id = hdr.msg_id`.

Pseudocode:

```text
handle_ping(hdr, src):
  pong = build_pong_status()
  send_unicast(src, PONG(hdr.msg_id), pong_payload)
```

##### 6.9.4 `PONG` (node → controller unicast)

Semantics:
- The node’s status snapshot for UI/discovery.

Recommended ESP32 fields (source of truth: `led_task_get_status()` + Wi‑Fi + local time):
- `uptime_ms`: `esp_timer_get_time()/1000` (u32)
- `rssi_dbm`: `esp_wifi_sta_get_ap_info(&ap).rssi` (i8)
- `state_flags` bits:
  - bit0: running (always 1 for “node task alive”)
  - bit1: paused (optional; if effect task has pause)
  - bit2: time_synced
- `brightness_pct`: from effect status (global)
- `pattern_type`: current effect pattern
- `frame_ms`: from effect status
- `active_cue_id`: last applied cue id (0 if none)
- `controller_epoch`: current epoch (0 if unknown)
- `show_ms_now`: `show_ms()` (u32)
- `name[16]`: node name (NUL padded)

##### 6.9.5 `CUE_PREPARE` (controller → multicast, typically target_mode=NODE)

Semantics:
- “Store pattern config for cue_id”. Not time critical.

Rules:
- Must be epoch-gated:
  - ignore if `hdr.epoch_id != controller_epoch` or epoch unknown.
- Must respect targeting:
  - if `target_mode == NODE` and `hdr.target != node_id`, ignore.
  - if `target_mode == GROUP`, optionally filter by `group_id` (requires a stored group id).
  - if `target_mode == ALL`, accept.
- On valid payload:
  - store/overwrite cue entry for `cue_id`.
  - if `ACK_REQ` flag set, send unicast `ACK` to `src`.
- On invalid payload or storage full:
  - if `ACK_REQ`, send `ACK` with a nonzero error code.

Pseudocode:

```text
handle_cue_prepare(hdr, payload, src):
  if !epoch_ok(hdr): return
  if !target_matches(hdr): return

  cp = decode_cue_prepare(payload)
  if !cp.ok:
    if hdr.ack_req: send_ack(src, hdr.msg_id, code=BAD_PAYLOAD)
    return

  if !cue_store_put(cp):
    if hdr.ack_req: send_ack(src, hdr.msg_id, code=NO_SPACE)
    return

  if hdr.ack_req: send_ack(src, hdr.msg_id, code=OK)
```

##### 6.9.6 `CUE_FIRE` (controller → multicast, time critical)

Semantics:
- “Execute cue_id at show-time execute_at_ms”.

Rules:
- Must be epoch-gated.
- If payload invalid, ignore.
- Store `(cue_id, execute_at_ms)` into pending fires (bounded).
- If pending fires list is full:
  - drop (or drop oldest; choose one policy and document it).
- Duplicates:
  - Either allow duplicates and make `apply` idempotent, or dedup by `(cue_id, execute_at_ms)` (recommended if controller retransmits).

Pseudocode:

```text
handle_cue_fire(hdr, payload):
  if !epoch_ok(hdr): return
  cf = decode_cue_fire(payload)
  if !cf.ok: return
  schedule_fire(cf.cue_id, hdr.execute_at_ms)
```

##### 6.9.7 `CUE_CANCEL` (controller → multicast)

Semantics:
- “Cancel stored cue and any pending fires for cue_id”.

Rules:
- Must be epoch-gated.
- On valid payload:
  - delete cue store entry for cue_id,
  - delete all pending fires for cue_id.

##### 6.9.8 `TIME_REQ` / `TIME_RESP` (optional refinement)

Semantics:
- Refinement loop: node asks controller for timing info; controller replies with timestamps that allow the node to estimate offset.

Rules:
- `TIME_REQ` should be sent only when controller address is known (typically from BEACON).
- Track outstanding `TIME_REQ` by `req_id = msg_id` in a bounded table:
  - store `t0_local_ms` at send time.
- `TIME_RESP` must be epoch-gated and must match a stored `req_id`.
- On success, compute RTT and set refined offset; store last RTT sample.

#### 6.10 Wrap-around-safe time arithmetic (why `u32` clocks are tricky)

The protocol uses `u32` milliseconds on the wire for show time. That means wrap-around every ~49.7 days.

The naive comparison:

```text
if now_ms >= execute_at_ms: due
```

fails across wrap-around. The standard fix is to compare using a signed difference on the ring:

```text
diff = (now_ms - execute_at_ms) as unsigned u32
due if diff <= 0x7fffffff
```

Intuition:
- If `now` is “slightly after” `execute_at`, then `now-execute_at` is small → due.
- If `now` is “slightly before” `execute_at`, then `now-execute_at` underflows to a huge number (> 0x7fffffff) → not due.

This only works correctly if you never schedule events more than 2^31-1 ms into the future (~24.8 days). For this application (seconds/minutes), that assumption is safe.

#### 6.11 The `node` console command surface (operator ergonomics)

To make the node feel like a “thing on the network”, provide a small set of `node` commands:

```text
node status
node id
node id set <u32> [--save]
node name
node name set <name> [--save]
node group
node group set <u32> [--save]
node start
node stop
node time status
node cue list
node cue clear [cue_id]
```

Design principles:
- Commands print one-line machine-parsable summaries where possible (easy to copy into logs).
- Anything that changes persistent configuration is explicit (`--save`).
- Nothing ever prints Wi‑Fi passwords or other secrets.

#### 6.12 PatternConfig `data[12]` mapping tables (wire → internal)

The `PatternConfig` payload is fixed-size, but pattern parameters are packed into a 12-byte “blob”. The mapping must be stable and shared between controller and node.

This mapping is already described in the imported protocol notes, and the internal LED engine in `0044` aligns well with it. The tables below define the decoding contract the ESP32 node should implement.

General decoding rules:
- Treat bytes as unsigned (`0..255`) unless explicitly marked signed.
- Validate ranges and clamp or reject (choose one policy; this design recommends “clamp with warning” for operator friendliness).

##### 6.12.1 `RAINBOW` (`pattern_type=1`)

| Byte(s) | Meaning | Range | `0044` field |
|---:|---|---:|---|
| `data[0]` | speed (rotations/min) | `0..20` | `led_rainbow_cfg_t.speed` |
| `data[1]` | saturation (%) | `0..100` | `led_rainbow_cfg_t.saturation` |
| `data[2]` | spread_x10 | `1..50` | `led_rainbow_cfg_t.spread_x10` |
| others | reserved | — | ignored |

##### 6.12.2 `CHASE` (`pattern_type=2`)

| Byte(s) | Meaning | Range | `0044` field |
|---:|---|---:|---|
| `data[0]` | speed (LED/s) | `0..255` | `led_chase_cfg_t.speed` |
| `data[1]` | tail_len | `1..255` | `led_chase_cfg_t.tail_len` |
| `data[2]` | gap_len | `0..255` | `led_chase_cfg_t.gap_len` |
| `data[3]` | trains | `1..255` | `led_chase_cfg_t.trains` |
| `data[4..6]` | fg RGB | bytes | `led_chase_cfg_t.fg` |
| `data[7..9]` | bg RGB | bytes | `led_chase_cfg_t.bg` |
| `data[10]` | dir | `0 fwd, 1 rev, 2 bounce` | `led_chase_cfg_t.dir` |
| `data[11]` | fade_tail | `0/1` | `led_chase_cfg_t.fade_tail` |

##### 6.12.3 `BREATHING` (`pattern_type=3`)

| Byte(s) | Meaning | Range | `0044` field |
|---:|---|---:|---|
| `data[0]` | speed (breaths/min) | `0..20` | `led_breathing_cfg_t.speed` |
| `data[1..3]` | color RGB | bytes | `led_breathing_cfg_t.color` |
| `data[4]` | min_bri | `0..255` | `led_breathing_cfg_t.min_bri` |
| `data[5]` | max_bri | `0..255` | `led_breathing_cfg_t.max_bri` |
| `data[6]` | curve | `0 sine, 1 linear, 2 ease` | `led_breathing_cfg_t.curve` |
| `data[7..11]` | reserved | — | ignored |

##### 6.12.4 `SPARKLE` (`pattern_type=4`)

| Byte(s) | Meaning | Range | `0044` field |
|---:|---|---:|---|
| `data[0]` | speed (spawn control) | `0..20` | `led_sparkle_cfg_t.speed` |
| `data[1..3]` | color RGB | bytes | `led_sparkle_cfg_t.color` |
| `data[4]` | density_pct | `0..100` | `led_sparkle_cfg_t.density_pct` |
| `data[5]` | fade_speed | `1..255` | `led_sparkle_cfg_t.fade_speed` |
| `data[6]` | color_mode | `0 fixed, 1 random, 2 rainbow` | `led_sparkle_cfg_t.color_mode` |
| `data[7..9]` | background RGB | bytes | `led_sparkle_cfg_t.background` |
| `data[10..11]` | reserved | — | ignored |

##### 6.12.5 Seeds and determinism (optional)

The `PatternConfig.seed` field (u32) allows deterministic sparkle/random behavior across nodes. If `flags & 0x01` (“seed_valid”) is set, the node should initialize its pattern RNG with that seed when applying the cue.

This is optional but valuable for reproducible shows.

#### 6.13 Data structures: bounded stores (no malloc in hot paths)

The node’s correctness depends on predictable behavior under stress. The simplest embedded-friendly strategy is to use fixed-size arrays (as in the C reference):

- Cue store: `MAX_CUES` entries, each storing:
  - `cue_id` (u32)
  - `PatternConfig` (or already-decoded internal config)
  - `fade_in_ms`, `fade_out_ms`
  - valid bit
- Pending fires: `MAX_FIRES` entries of `(cue_id, execute_at_ms)`
- Outstanding time requests: `MAX_TIME_REQS` entries of `(req_id, t0_local_ms)`

This avoids heap fragmentation and makes backpressure policies explicit.

Suggested policies (documented and testable):
- `cue_put`: overwrite existing; else use first free; else return “no space”.
- `fire_add`: append if space; else drop newest (or drop oldest; pick one).
- `fire_remove_cue`: stable filter compaction (O(n), but n is small).

ASCII layout for intuition:

```text
cues[0..63]:
  [valid, cue_id, pattern_cfg, fade_in, fade_out]

pending_fires[0..63]:
  [cue_id, execute_at_ms]

time_reqs[0..15]:
  [valid, req_id, t0_local_ms]
```

### 7. Design Decisions

1) **Use `esp_console` for provisioning** (not hardcoded secrets).
   - Rationale: matches existing repo patterns; easy for operator workflows.

2) **Use a node protocol task with explicit state + scheduler** (not callbacks scattered across event handlers).
   - Rationale: makes protocol behavior deterministic and testable; mirrors the C reference implementation.

3) **Use epoch gating** as the primary safety rule.
   - Rationale: prevents “stale controller” traffic from causing actions after controller restarts.

4) **Prefer idempotency over perfect reliability** for `CUE_FIRE`.
   - Rationale: UDP multicast is lossy; sending `CUE_FIRE` multiple times is a practical mitigation. If repeated fires are idempotent, duplicates are harmless.

5) **Keep wire structs fixed-size and byte-packed**.
   - Rationale: stable on-wire contract for embedded devices; easy to implement in C.

### 8. Alternatives Considered

1) **HTTP polling / REST control** instead of UDP multicast.
   - Pros: easier debugging, browser-friendly.
   - Cons: per-node requests do not scale; synchronized triggers are harder; controller must track node IPs.

2) **TCP-based control** instead of UDP.
   - Pros: reliability and ordering.
   - Cons: connection management is complex; multicast is not available; synchronized fan-out becomes expensive.

3) **mDNS/Bonjour discovery** instead of `PING/PONG`.
   - Pros: standard service discovery.
   - Cons: more moving parts; still need application-level status; multicast DNS can be brittle on some networks.

The current protocol is intentionally minimal and purpose-built for “a show in a room”.

## Implementation Plan

Phase 1: Wi‑Fi + console provisioning (reuse proven code)
1) Lift/port `wifi_mgr.*` and `wifi_console.*` patterns from `0045`.
2) Add `node` console command stub with `node status`.

Phase 2: UDP multicast transport on ESP32
1) Implement `node_net_start()` invoked on got-IP:
   - create/bind socket :4626
   - join multicast group
2) Implement `node_net_stop()` invoked on lost-IP:
   - close socket(s)

Phase 3: Protocol parsing and state machine
1) Implement header/payload unpack and validation (byte-level helpers).
2) Implement handlers: `BEACON`, `PING`, `CUE_PREPARE`, `CUE_FIRE`, `CUE_CANCEL`, `TIME_REQ/TIME_RESP` (optional).
3) Implement cue store + pending fires with wrap-around-safe comparisons.

Phase 4: Tie cues to an effect engine
1) Reuse `led_task.*` from `0044` (or a similar single-owner effect task).
2) Implement `protocol_to_led_cfg()` mapping (wire → internal).
3) On due fire, send message to effect task.

Phase 5: Validation
1) Use the Python controller/simulator (from mled-web) to send `PING`, `BEACON`, `CUE_PREPARE`, `CUE_FIRE`.
2) Verify:
   - node appears in controller UI (PONG fields make sense),
   - time offset converges,
   - cue fires at expected time (within Wi‑Fi tolerance),
   - epoch change resets cues and ignores stale traffic.

## Open Questions

1) Should the ESP32 node implement `HELLO` on boot (multicast) or rely exclusively on periodic controller `PING`?
2) Should `CUE_FIRE` duplicates be deduplicated by `(cue_id, execute_at_ms)` or should the effect engine be strictly idempotent?
3) Do we want a lightweight authentication mechanism (e.g., shared secret HMAC) for non-trusted LANs?
4) Should show time be widened to `u64` on the node (internal), keeping `u32` only on the wire?

## References

- Ticket diary: `esp32-s3-m5/ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/reference/01-diary.md`
- Protocol sources index: `esp32-s3-m5/ttmp/2026/01/21/0049-NODE-PROTOCOL--esp32-c6-node-protocol-firmware/reference/02-protocol-sources-mled-web.md`
- mled-web protocol notes (imported): `2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/sources/local/protocol.md (imported).md`
- Python protocol review: `2026-01-21--mled-web/ttmp/2026/01/21/MO-001-MULTICAST-PY--multicast-python-app-protocol-review/design-doc/01-python-app-protocol-review.md`
- C reference node: `2026-01-21--mled-web/c/mledc/node.c`
- ESP32-C6 Wi‑Fi console provisioning patterns: `esp32-s3-m5/0045-xiao-esp32c6-preact-web/` and `esp32-s3-m5/0029-mock-zigbee-http-hub/`
- Single-owner WS281x effect task pattern: `esp32-s3-m5/0044-xiao-esp32c6-ws281x-patterns-console/`

## Design Decisions

<!-- Document key design decisions and rationale -->

## Alternatives Considered

<!-- List alternative approaches that were considered and why they were rejected -->

## Implementation Plan

<!-- Outline the steps to implement this design -->

## Open Questions

<!-- List any unresolved questions or concerns -->

## References

<!-- Link to related documents, RFCs, or external resources -->
