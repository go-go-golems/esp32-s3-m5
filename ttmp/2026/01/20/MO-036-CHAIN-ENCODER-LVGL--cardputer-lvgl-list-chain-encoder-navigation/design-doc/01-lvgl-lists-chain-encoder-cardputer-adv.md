---
Title: LVGL Lists + Chain Encoder (Cardputer ADV)
Ticket: MO-036-CHAIN-ENCODER-LVGL
Status: active
Topics:
    - cardputer
    - ui
    - display
    - uart
    - serial
    - gpio
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Chain_Function/base_function.c
      Note: CRC implementation and helper semantics used by protocol.
    - Path: M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Src/stm32g0xx_it.c
      Note: Executable spec for framing + Index_id routing/relay rules.
    - Path: M5Chain-Series-Internal-FW/Chain-Encder/protocol/M5Stack-Chain-Encoder-Protocol-V1-EN.pdf
      Note: Primary protocol contract (framing/CRC/commands/baud).
    - Path: esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_file_browser.cpp
      Note: Reference list implementation (manual rows
    - Path: esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb.cpp
      Note: LVGL indev driver pattern (queue + read_cb + register).
    - Path: esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/Kconfig.projbuild
      Note: Cardputer-ADV Grove UART defaults (G1/G2 GPIO mapping
    - Path: esp32-s3-m5/AGENTS.md
      Note: Repo guidance on USB Serial/JTAG console to avoid UART protocol contamination.
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-20T21:12:38.005318292-05:00
WhatFor: ""
WhenToUse: ""
---


# LVGL Lists + Chain Encoder (Cardputer ADV)

## Executive Summary

This document explains, in “textbook” detail, how to:

1. Display a list UI on **LVGL** on **Cardputer-ADV**, and
2. Navigate that list using an attached **M5Stack Chain Encoder (U207)** over the **Grove UART** pins.

It is grounded in two classes of evidence:

- The *published protocol contract* for the Chain Encoder:
  - `M5Chain-Series-Internal-FW/Chain-Encder/protocol/M5Stack-Chain-Encoder-Protocol-V1-EN.pdf`
- The *executable spec* (STM32 firmware source) that clarifies the semantics of fields like `Index_id` and the CRC:
  - `M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Src/stm32g0xx_it.c`
  - `M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Chain_Function/base_function.c`

For LVGL patterns on Cardputer, this doc references an in-repo LVGL bring-up that already solves “LVGL + M5GFX + focus groups + input device”:

- `esp32-s3-m5/0025-cardputer-lvgl-demo/`

For Cardputer-ADV Grove UART defaults (G1/G2 mapping and baud), it references:

- `esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/Kconfig.projbuild`

This is a design and analysis document: it aims to be copy/paste friendly (frames, pseudocode, API references), but it does not assume you have flashed custom firmware yet.

## Problem Statement

We want a list UI that feels “native”:

- **Visual:** clearly highlights the currently-selected item; scrolls smoothly; works within the small Cardputer screen.
- **Input:** rotating the Chain Encoder moves selection; pressing the encoder selects (or triggers an action); optional double-click/long-press can map to additional semantics (e.g., back, menu).
- **Robustness:** UART framing must tolerate noise, partial reads, and asynchronous packets (active reporting mode).
- **Maintainability:** the system should be decomposable: protocol driver ≠ LVGL indev driver ≠ list widget.

Constraints and implied requirements:

- Cardputer-ADV is an ESP32-S3 device with multiple possible console transports; UART pins are precious.
- The Chain Encoder speaks a binary, framed UART protocol at `115200 8N1`.
- The Chain protocol supports daisy-chaining devices; therefore, addressing (`Index_id`) is not a simple “device ID field” but part of a routing model.

## Proposed Solution

Break the design into three layers, each with a narrow contract:

1. **Transport + framing layer (UART + packet parser)**
   - Reads bytes from UART, finds frames (`0xAA 0x55 … 0x55 0xAA`), validates CRC, and emits typed packets.
2. **Chain Encoder device layer (protocol client)**
   - Implements `get_increment()`, `get_button()`, `set_mode()`, etc.
   - Optionally consumes `0xE0` “active reporting” packets as an event stream.
3. **LVGL integration layer**
   - Exposes the encoder as either:
     - (A) an LVGL **encoder input device** (`LV_INDEV_TYPE_ENCODER`), or
     - (B) a key-event generator that feeds the existing **keypad** input device with `LV_KEY_UP/DOWN/ENTER`.
4. **List view**
   - Option 1 (preferred for “list of actions”): LVGL `lv_list` with focused buttons.
   - Option 2 (preferred for “tight control / low overhead”): a custom list of N visible rows with manual highlight and scroll state (patterned after `demo_file_browser.cpp` in the LVGL demo).

The key architectural claim is that **UI navigation should be expressed in LVGL’s native input model** (groups + indev), and **UART/protocol complexity should not leak into LVGL callbacks**.

### Wiring (Cardputer-ADV ↔ Chain Encoder)

You wired:

- Chain Encoder `TX1` → Cardputer `G1`
- Chain Encoder `RX`  → Cardputer `G2`

This matches the common UART crossover rule:

| Signal role | Chain Encoder pin | Cardputer Grove pin | Cardputer GPIO default |
|---|---:|---:|---:|
| Device → Host (TX) | `TX1` | `G1` (RX) | `GPIO1` |
| Host → Device (TX) | `RX` | `G2` (TX) | `GPIO2` |

> **Fundamental: UART is asymmetric.**
> One side’s TX must land on the other side’s RX. Labeling like “G1/G2” is a board convention; the actual UART direction is a firmware configuration.

The repo’s Cardputer-ADV UART defaults (used by tutorial `0038`) are:

- UART controller: `UART1`
- Baud: `115200`
- TX pin: `GPIO2` (G2)
- RX pin: `GPIO1` (G1)

See: `esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/Kconfig.projbuild`.

### Protocol primer: what the encoder actually speaks

The Chain Encoder protocol is byte-oriented and framed. Every frame is:

```
0xAA 0x55  <len_lo> <len_hi>  <payload...>  0x55 0xAA
```

Payload layout is:

```
Index_id  Cmd  Data...  CRC
```

Where:

- `len` is little-endian and counts **from `Index_id` through `CRC` inclusive**.
- `CRC` is a simple 8-bit sum of the payload bytes **excluding the CRC itself**.

> **Fundamental: framing + length makes resynchronization possible.**
> With a start sentinel, explicit length, and end sentinel, you can scan a raw byte stream for frames even when you start reading mid-frame or when noise injects stray bytes.

#### CRC definition (and why we can trust it)

The protocol PDF gives the “sum of payload bytes” CRC rule, and the firmware implements it.

From the protocol PDF note:

- “When calculating CRC, the packet header, packet trailer, length, and the CRC field itself need to be excluded; only the remaining data is summed.”

In firmware, the CRC check is implemented as a sum over `Index_id + Cmd + Data...` and compared to the CRC byte:

- `M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Chain_Function/base_function.c`

#### `Index_id` semantics: hop-count addressing for daisy chains

The most important “implicit” rule (not stated as an algorithm in the PDF, but made explicit by the firmware) is:

- A device **handles** a packet only when `Index_id == 1`.
- Otherwise it **relays** the packet “down the chain” and decrements `Index_id` (and adjusts CRC accordingly).
- Packets traveling “up the chain” have their `Index_id` incremented at each relay hop (again, CRC adjusted).

This is visible in the packet routing logic:

- `USART1_IRQHandler` (incoming upstream direction): handles local only when `p[4] == 1`, else relays after decrement.
- `USART2_IRQHandler` (incoming downstream direction): relays upstream after increment.

See: `M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Src/stm32g0xx_it.c`.

> **Fundamental: Index is not an identity; it’s a distance.**
> For single-device setups, you almost never notice this because “distance to the first device” is always 1. For multi-device setups, this becomes a powerful routing primitive: to address the Nth device, you set `Index_id = N`.

#### Timing and UART mode

The protocol note in the PDF specifies:

- `115200`, `8` data bits, `1` stop bit, `no parity` (8N1).
- Max packet length `256` bytes.

### Command set overview (what you need for LVGL navigation)

For list navigation, you primarily need:

- **Encoder increment**: `0x11` (returns signed int16 delta, clears after read)
- **Button press state**: `0xE1` (pressed/not pressed)
- Optionally **button active reporting mode**: `0xE4` (enable/disable), and **async button events** `0xE0` (single/double/long press)

The full protocol also includes RGB LED control and version/UID queries.

#### Selected command reference (U207 Chain Encoder)

All packets are framed and CRC’d as described above.

| Command | Meaning | Request payload | Response payload |
|---:|---|---|---|
| `0x10` | Get absolute encoder value | none | `int16 encoder` |
| `0x11` | Get encoder increment value (delta, auto-clears) | none | `int16 delta` |
| `0x13` | Reset absolute encoder value | none | `uint8 status` |
| `0x14` | Reset increment value | none | `uint8 status` |
| `0x15` | Set AB direction (CW inc/dec) | `uint8 dir`, `uint8 save` | `uint8 status` |
| `0x16` | Get AB direction | none | `uint8 dir` |
| `0xE0` | Async button trigger event (active mode only) | none | `uint8 trigger`, `0x00` |
| `0xE1` | Query button pressed state | none | `uint8 pressed` |
| `0xE2` | Set double/long thresholds | `uint8 dbl`, `uint8 long` | `uint8 status` |
| `0xE4` | Set button mode | `uint8 mode` | `uint8 status` |
| `0xFB` | Query device type | none | `uint16 type` (`0x0001` for Encoder) |
| `0xFE` | Enumerate number of devices in chain | `uint8 send_num` | `uint8 receive_num` |

> **Fundamental: choose “delta” over “absolute” for UI navigation.**
> A list selection is naturally updated by increments. Using `0x11` avoids race conditions (“did I miss a tick?”) because the device accumulates deltas for you between reads.

### Example frames (copy/paste mental model)

These are illustrative; your driver should generate them rather than hardcoding.

**Get encoder increment** (`Cmd=0x11`, `Index_id=0x01`):

```
AA 55 03 00  01 11  12  55 AA
             ^  ^   ^
           idx cmd crc = 0x01 + 0x11 = 0x12
```

**Response** with `delta = +1` (`Encode_low=0x01`, `Encode_high=0x00`):

```
AA 55 05 00  01 11  01 00  13  55 AA
             idx cmd  lo hi  crc = 0x01+0x11+0x01+0x00 = 0x13
```

**Query button pressed** (`Cmd=0xE1`, `Index_id=0x01`):

```
AA 55 03 00  01 E1  E2  55 AA
```

**Response** with `pressed=1`:

```
AA 55 04 00  01 E1  01  E3  55 AA
```

**Async event**: single click (`Cmd=0xE0`, `Status=0`), only in active-reporting mode:

```
AA 55 05 00  01 E0  00 00  E1  55 AA
```

> **Fundamental: asynchronous packets mean you need a real parser.**
> If you assume “every read after a write is the response”, the first unsolicited `0xE0` will desynchronize your stream and break all subsequent queries.

### LVGL integration: two viable approaches

LVGL supports multiple input device types. The key ones here:

- `LV_INDEV_TYPE_KEYPAD`: delivers key codes (`LV_KEY_UP`, `LV_KEY_DOWN`, `LV_KEY_ENTER`, …).
- `LV_INDEV_TYPE_ENCODER`: delivers relative increments (`enc_diff`) plus a single button state (`pressed` / `released`).

Both can be used to drive a list. Which to choose depends on how “native” you want the encoder behavior to be.

#### Approach A: Create a real LVGL encoder indev (recommended)

Create an LVGL indev with `type = LV_INDEV_TYPE_ENCODER`. In its `read_cb`, you:

- Set `data->enc_diff` to the accumulated encoder delta since last call.
- Set `data->state` to `LV_INDEV_STATE_PR` when button is pressed, else `LV_INDEV_STATE_REL`.

Minimal API surface:

- `lv_indev_drv_init(&drv)`
- `drv.type = LV_INDEV_TYPE_ENCODER`
- `drv.read_cb = your_cb`
- `lv_indev_drv_register(&drv)`
- `lv_indev_set_group(indev, group)`

**Where to copy patterns from (in-repo):**

- `esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb.cpp`
  - This shows the idiom: `lv_indev_drv_init`, `read_cb`, and a local queue.

> **Fundamental: LVGL is not thread-safe.**
> Your UART task should never call LVGL directly. Instead, write decoded deltas/button states into atomics or a lock-free queue, and have the LVGL `read_cb` poll/consume them.

**Pseudocode: encoder indev read callback**

```c
static void chain_enc_indev_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void)drv;

    int32_t delta = atomic_exchange(&g_chain_delta_accum, 0);
    bool pressed = atomic_load(&g_chain_btn_pressed);

    data->enc_diff = (int16_t)clamp(delta, -32768, 32767);
    data->state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}
```

#### Approach B: Translate encoder into LVGL keypad keys (simple)

Instead of a new indev, interpret delta as repeated keys:

- `delta > 0` ⇒ enqueue `LV_KEY_DOWN` `delta` times
- `delta < 0` ⇒ enqueue `LV_KEY_UP` `abs(delta)` times
- button click ⇒ enqueue `LV_KEY_ENTER`

This leverages existing keypad-based navigation (as used by many of the demos in `0025`).

**Where to copy patterns from (in-repo):**

- `esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb_queue_key(...)`

This can be especially attractive if your list UI is already driven by `LV_EVENT_KEY` handlers (like `demo_file_browser.cpp`).

> **Fundamental: key translation loses “encoder editing” semantics.**
> LVGL’s encoder device supports a focus/editing split (common on embedded UIs). If you only generate keys, you’re opting into a “keyboard-like” UI model—which is often fine, but it’s a choice.

### List UI design: two patterns

#### Pattern 1: LVGL `lv_list` (built-in, focus-friendly)

Use LVGL’s list widget:

- `lv_list_create(parent)`
- `lv_list_add_btn(list, icon, text)` returns a button object
- Add each button to a focus group:
  - `lv_group_add_obj(group, btn)`

You can style focused items using LVGL states:

- `LV_STATE_FOCUSED`
- `LV_STATE_PRESSED`

And you can ensure scrolling follows focus:

- `lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS)`

This yields a canonical “scrollable list of selectable items” that works naturally with either keypad or encoder indev.

#### Pattern 2: Fixed visible rows (predictable and fast)

For tight control and minimal LVGL overhead, create a fixed number of row containers (e.g., 4–8), and render your current scroll window into them. Maintain:

- `selected_index`
- `scroll_offset`
- `entries[]`

This is already implemented for file browsing in the LVGL demo:

- `esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_file_browser.cpp`

It uses:

- A `render()` function that:
  - clamps selection and scroll,
  - updates visible labels, and
  - styles the selected row (background color).
- A `key_cb` that changes `selected` on `LV_KEY_UP/DOWN`.

To adapt it to the encoder:

- With Approach B (key translation): you can reuse the same `key_cb` unchanged.
- With Approach A (encoder indev): you can let LVGL drive focus, or you can keep manual selection logic and treat encoder increments as changes to `selected`.

**Pseudocode: list scroll invariant (Norvig-style “state machine”)**

```text
State:
  selected ∈ [0, n-1]
  scroll   ∈ [0, max(0, n - visible)]

Invariant:
  selected ∈ [scroll, scroll + visible - 1]  whenever n > 0

Update(selected ← selected + delta):
  selected ← clamp(selected, 0, n-1)
  if selected < scroll: scroll ← selected
  if selected ≥ scroll + visible: scroll ← selected - (visible - 1)
  scroll ← clamp(scroll, 0, max(0, n-visible))
```

> **Fundamental: a list is an algorithm, not just a widget.**
> Any list UI—LVGL list, custom rows, or a table—must solve the same problem: maintaining a selection cursor and a viewport over a potentially larger collection. Treating this as an explicit state machine makes edge cases (empty list, single item, wrap) obvious.

### Host-side protocol driver: a minimal but robust design

There are two design constraints that dominate:

1. UART reads are chunked and can start/end anywhere inside a frame.
2. The device may emit asynchronous frames (`0xE0`) while you’re waiting for a response.

This suggests a standard solution:

- A single UART RX task that:
  - reads bytes into a ring buffer,
  - extracts complete frames,
  - validates CRC and tail,
  - dispatches frames to:
    - a response-waiter (for synchronous request/response), or
    - an event handler (for async events like `0xE0`).

> **Fundamental: keep the UART “truth” single-threaded.**
> If multiple tasks read from the UART driver, you will create heisenbugs. The UART stream is a single sequence; it needs a single owner that parses and demultiplexes.

#### Framer pseudocode (byte stream → frames)

```c
// Input: append bytes to buffer B (ring or linear with compaction)
// Output: while a full frame exists, emit it and remove from B

while (B.size >= MIN_FRAME) {
  // Find header
  i = find_subsequence(B, [0xAA, 0x55]);
  if (i < 0) { B.clear(); break; }
  drop_prefix(B, i);

  if (B.size < 6) break; // need len and at least CRC+tail

  len = B[2] | (B[3] << 8);
  total = 6 + len;
  if (len > 256 || total < MIN_FRAME) { drop_prefix(B, 1); continue; }
  if (B.size < total) break;

  if (B[total-2] != 0x55 || B[total-1] != 0xAA) { drop_prefix(B, 1); continue; }

  frame = B[0:total];
  if (!crc_ok(frame)) { drop_prefix(B, 1); continue; }

  emit(frame);
  drop_prefix(B, total);
}
```

This is essentially the same logic used by the STM32 firmware to parse buffers in its UART idle interrupt:

- `M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Src/stm32g0xx_it.c`

#### Request/response matching rule

A simple match function for single-device setups:

- Match on `(Index_id, Cmd)`; assume 1 outstanding request at a time.

For more robustness (and multi-device support), also match:

- length expectations for each command,
- and possibly add an application-level request ID (not present in the protocol), implemented by “only one request at a time” in the host.

> **Fundamental: many embedded protocols omit explicit request IDs.**
> The usual replacement is “serialize requests” and rely on the invariant that only one command is in flight at a time. This is simpler and often correct for human-speed UI inputs.

### Mapping encoder input to list navigation

Given a driver API:

- `int16_t chain_enc_get_delta()` (from `0x11`)
- `bool chain_enc_is_pressed()` (from `0xE1`), or events from `0xE0`

You can define the UI mapping:

- Rotate: change selection (focus next/prev item)
- Press: activate selection
- Double-click: optional “back”
- Long-press: optional “menu” or “home”

The exact mapping depends on your application’s UX, but the important part is consistency: a physical control should have a stable interpretation across screens.

### Firmware examples to study (and why)

**LVGL bring-up + focus groups + input:**

- `esp32-s3-m5/0047-cardputer-adv-lvgl-chain-encoder-list/`
  - Minimal “LVGL list + Chain Encoder” firmware (this ticket’s implementation).

- `esp32-s3-m5/0025-cardputer-lvgl-demo/`
  - `main/lvgl_port_m5gfx.cpp`: display flush model (LVGL → M5GFX)
  - `main/lvgl_port_cardputer_kb.cpp`: indev driver pattern and queueing
  - `main/demo_manager.cpp`: group creation, wrapping, per-screen focus binding
  - `main/demo_file_browser.cpp`: “manual list” pattern with selection+scroll invariants

**Cardputer-ADV UART pin mapping (G1/G2 → GPIO1/GPIO2) and baud defaults:**

- `esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/Kconfig.projbuild`

**General safety note about consoles (UART protocol contamination):**

- `esp32-s3-m5/AGENTS.md`
  - Prefer USB Serial/JTAG for interactive console so UART protocol traffic stays clean.

## Design Decisions

1. **Use `0x11` (increment) for UI navigation**
   - Rationale: deltas align with list navigation; the device clears on read, acting as an accumulator between polls.
2. **Avoid parsing UART in LVGL callbacks**
   - Rationale: LVGL is single-threaded; UART parsing can block and is nondeterministic.
3. **Disable active reporting (`0xE4 mode=0`) unless you explicitly want `0xE0`**
   - Rationale: unsolicited packets complicate a synchronous request/response design. You can still query pressed state via `0xE1`.
4. **Keep USB Serial/JTAG console enabled; do not share the Chain UART with logs**
   - Rationale: logs corrupt binary protocols; the repo explicitly recommends USB Serial/JTAG for consoles on Cardputer/ESP32-S3.

## Alternatives Considered

1. **Poll absolute encoder value (`0x10`) and diff locally**
   - Rejected: invites wrap/overflow reasoning and race conditions; `0x11` already provides an atomic delta.
2. **Treat the Chain Encoder as a “keyboard” only (always translate to keys)**
   - Not rejected; valid as Approach B. But it sacrifices LVGL encoder semantics (focus/editing split) and can feel less “rotary-native”.
3. **Rely entirely on `0xE0` active events for button semantics**
   - Rejected as default: it requires robust async parsing anyway and creates subtle interactions with synchronous queries. Consider it later for richer UX once the base driver is stable.

## Implementation Plan

1. **UART bring-up on Cardputer-ADV**
   - Configure `UART1` at `115200 8N1`.
   - Pins: RX=`GPIO1` (G1), TX=`GPIO2` (G2) unless you intentionally remap.
2. **Build a frame parser**
   - Implement the header/len/tail scanner and CRC check.
   - Add unit-test-like host-side fixtures if you can (golden frames).
3. **Implement Chain Encoder client functions**
   - `get_delta()` via `0x11`
   - `get_pressed()` via `0xE1`
   - Optionally `set_button_mode(0)` via `0xE4` at startup
4. **Implement LVGL integration**
   - Option A: encoder indev; bind it to the app’s group.
   - Option B: key translation feeding existing keypad indev.
5. **Implement list view**
   - If using LVGL list: add items as focusable buttons and style focus.
   - If custom: implement `selected/scroll` invariant and rendering.
6. **Validate on hardware**
   - Turn the encoder: selection moves one step per detent (adjust scaling if needed).
   - Press: activates item.
   - Confirm no UART corruption from logs (keep console on USB Serial/JTAG).

## Open Questions

1. **Physical pin naming sanity check**
   - Confirm that the Chain Encoder’s `TX1/RX` pins are indeed the upstream/host UART corresponding to how the STM32 firmware expects traffic (the analysis assumes they are).
2. **Default button reporting mode**
   - The protocol PDF indicates button mode default is `1` (active reporting). If that is true on your firmware version, unsolicited `0xE0` frames will appear unless you explicitly set mode `0`.
3. **Detent-to-step scaling**
   - Determine whether one “click” of the rotary encoder produces `delta=±1` or a larger magnitude. If larger, decide whether to scale (e.g., divide by 2 or clamp to ±1 per LVGL tick).
4. **Multiple chain devices**
   - If you later daisy-chain devices, confirm addressing via `Index_id=N` and/or enumeration `0xFE` from the host behaves as expected.

## References

- Protocol (primary): `M5Chain-Series-Internal-FW/Chain-Encder/protocol/M5Stack-Chain-Encoder-Protocol-V1-EN.pdf`
- Firmware parsing and routing (executable spec): `M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Src/stm32g0xx_it.c`
- Firmware CRC + relay helpers: `M5Chain-Series-Internal-FW/Chain-Encder/code/APP/Core/Chain_Function/base_function.c`
- Cardputer-ADV Grove UART defaults: `esp32-s3-m5/0038-cardputer-adv-serial-terminal/main/Kconfig.projbuild`
- LVGL bring-up and list patterns on Cardputer: `esp32-s3-m5/0025-cardputer-lvgl-demo/`
