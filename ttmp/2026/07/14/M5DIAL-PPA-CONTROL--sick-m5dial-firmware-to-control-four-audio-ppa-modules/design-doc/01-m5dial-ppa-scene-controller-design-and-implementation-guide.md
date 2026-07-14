---
Title: M5Dial PPA scene controller — design and implementation guide
Ticket: M5DIAL-PPA-CONTROL
Status: active
Topics:
    - m5dial
    - esp32-s3
    - firmware
    - udp
    - lvgl
    - wifi
    - audio
    - esp-idf
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0049-xiao-esp32c6-mled-node/tools/mled_ping.py
      Note: Host-side UDP tool pattern for the PPA module simulator
    - Path: repo://0072-m5dial-timer-demo/main/input_events.h
      Note: InputEvent queue types reused for encoder/button/touch
    - Path: repo://0072-m5dial-timer-demo/main/lvgl_port_m5dial.h
      Note: LVGL 8.3 display/tick port to copy
    - Path: repo://0072-m5dial-timer-demo/main/m5dial_board.h
      Note: Board bring-up class to copy (display, encoder, touch, button ISR)
    - Path: repo://0095-m5dial-wifi-bench/main/wifi_app.h
      Note: Native esp_wifi STA/AP manager adapted into wifi_mgr
    - Path: repo://ttmp/2026/07/14/M5DIAL-PPA-CONTROL--sick-m5dial-firmware-to-control-four-audio-ppa-modules/sources/m5dial-ppa-prototype/ANLEITUNG.md
      Note: German end-user flashing/provisioning instructions the rewrite must keep workable
    - Path: repo://ttmp/2026/07/14/M5DIAL-PPA-CONTROL--sick-m5dial-firmware-to-control-four-audio-ppa-modules/sources/m5dial-ppa-prototype/platformio.ini
      Note: Prototype build config (espressif32@6.7.0, M5Dial, ArduinoJson, LittleFS)
    - Path: repo://ttmp/2026/07/14/M5DIAL-PPA-CONTROL--sick-m5dial-firmware-to-control-four-audio-ppa-modules/sources/m5dial-ppa-prototype/src/main.cpp
      Note: Arduino prototype firmware; all protocol and behavior claims are line-anchored here
ExternalSources: []
Summary: 'Design for an ESP-IDF + LVGL rewrite of the PPA Dial prototype: an M5Stack Dial firmware that switches scenes on Four Audio PPA amplifier modules over a reverse-engineered UDP protocol (port 5001), with a polished encoder-driven UI, non-blocking protocol task, and web-based provisioning.'
LastUpdated: 2026-07-14T11:09:16.873137114-04:00
WhatFor: Implementation guide for building the production-quality M5Dial PPA scene controller firmware (project 0103-m5dial-ppa-dial).
WhenToUse: Read before implementing or reviewing the 0103 firmware; also the canonical reference for the reconstructed PPA UDP protocol.
---


# M5Dial PPA scene controller — design and implementation guide

## 1. Executive Summary

This document specifies a production-quality rewrite of the **PPA Dial** — an M5Stack Dial (ESP32-S3, round 1.28" GC9A01 touch display, rotary encoder) firmware that acts as a physical scene switcher for **Four Audio PPA** amplifier modules. The user rotates the dial to pick a scene, presses to switch it; the firmware sends preset-recall commands over a reverse-engineered UDP protocol (port 5001) to every module in the scene and confirms success on the display.

A working Arduino/PlatformIO prototype exists (preserved in this ticket under `sources/m5dial-ppa-prototype/`, 406 lines, single `main.cpp`). It proves the protocol and the interaction model but is a prototype in structure: blocking UI during scene recall, raw-text rendering without any visual polish, protocol handling interleaved with the render loop, and Arduino `String`-based config handling.

The rewrite targets **ESP-IDF 5.4.1 + LVGL 8.3**, following the conventions of the existing M5Dial firmware projects in this repository (`0072-m5dial-timer-demo`, `0073-m5dial-film-developer-timer`, `0095-m5dial-wifi-bench`). The headline goals:

1. **Polished LVGL UI** — a smooth encoder-driven scene carousel with animated transitions, an activation progress overlay, a status arc showing module reachability, and proper anti-aliased fonts on the round display.
2. **Non-blocking protocol engine** — a dedicated FreeRTOS task (`ppa_client`) owning the UDP socket, discovery cache, and the ack/busy/retry recall state machine; the UI never stalls.
3. **Functional parity for provisioning** — first-boot AP hotspot + web form (SSID/password + paste of the Mac app's `presets.json`), mDNS at `ppadial.local`, identical `presets.json` schema so the Mac app "PPA Group Control" remains the single source of truth for scenes.

The most durable content in this document is **section 4, the PPA UDP protocol reference** — the reconstructed wire format is not documented anywhere else.

## 2. Problem Statement and Scope

### 2.1 Problem

Four Audio PPA amplifier modules are normally controlled from the Mac app "PPA Group Control". For live operation (e.g. switching a venue between speech/music/off configurations) a dedicated physical controller is far better than opening a laptop: one knob, glanceable state, instant switching. The prototype demonstrates this works; now it needs to become a device that feels finished — responsive, animated, legible, and robust when modules are offline or busy.

### 2.2 In scope

- New ESP-IDF project `0103-m5dial-ppa-dial/` at the repository top level, following repo firmware conventions.
- LVGL 8.3 UI: scene carousel, activation flow, result feedback, connectivity states.
- `ppa_client` protocol component: discovery, recall, retry/busy handling, per-module reachability — all asynchronous.
- WiFi station with AP-fallback provisioning, web setup page (esp_http_server), mDNS, persistent config.
- `presets.json` compatibility with the Mac app (same schema, paste-in workflow).
- Host-side Python PPA module simulator for development and testing without real amplifiers.

### 2.3 Out of scope (explicitly deferred)

- OTA firmware updates.
- A dedicated per-module diagnostics page (RTT, error history) — the design leaves room for it but it is not part of this ticket.
- Captive-portal provisioning (DNS hijack); the prototype's "join AP, open 192.168.4.1" flow is kept.
- Any protocol extension beyond what the prototype uses (ping + preset recall). Volume/level control, mute, or status queries would require new packet captures.

## 3. Current State: the Prototype

Source: `sources/m5dial-ppa-prototype/src/main.cpp` (line references below are into that file). Stack: Arduino via PlatformIO, `platform = espressif32@6.7.0`, board `m5stack-stamps3`, libraries `M5Dial@^1.0.3` and `ArduinoJson@^7.0.4`, LittleFS filesystem (`sources/m5dial-ppa-prototype/platformio.ini`).

### 3.1 What the prototype does

- **Protocol framing** — `buildHeader()` (main.cpp:73–80) writes a 12-byte header; `sendPing()` (:82–86) emits a 16-byte ping; `sendRecall()` (:88–93) emits an 18-byte preset-recall with preset id/sub-id at offsets 13/14.
- **Receive path** — `pumpUdp()` (:96–123) drains the socket, learns `uid → IP` mappings from ping replies into a `found` vector, and matches awaited sequence numbers to classify replies as OK (1), busy (3), or error (9).
- **Discovery** — `broadcastDiscovery()` (:125–130) sends pings to both the /24 directed broadcast and 255.255.255.255, refreshed every 20 s from `loop()` (:399–403).
- **Recall with retry** — `recallAction()` (:140–156): up to 5 attempts, each with a 2.5 s ack deadline; busy replies back off 500 ms and retry; hard errors abort.
- **Scene activation** — `activateSelected()` (:315–333) iterates the scene's actions sequentially and **blocks the entire UI** until all modules answered or timed out (worst case ≈ 12.5 s per unreachable module).
- **Config** — LittleFS-backed `/config.json` holding SSID, password, and the raw `presets.json` text (`loadConfig`/`saveConfig`, :161–182). `parseScenes()` (:185–210) converts the Mac app's JSON into scenes.
- **Provisioning** — AP `PPA-Dial`/`ppadial123`, embedded HTML form (:213–229), `/save` handler persists and reboots (:239–249). In STA mode, mDNS registers `ppadial.local` (:362).
- **UI** — raw M5GFX text: scene name centered, "AKTIV" flag, `n/m online` counter, dot indicator strip (`drawMain`, :259–304). Encoder steps of ≥2 counts with an 80 ms time debounce select scenes (:380–390); encoder button or any touch triggers activation (:393–396). Beeps for step/success/failure via `M5Dial.Speaker`.

### 3.2 The presets.json contract (must remain compatible)

`parseScenes()` (:185–210) defines the de-facto schema shared with the Mac app:

```jsonc
{
  "scenes": {
    "<scene name>": {
      "uid_<hex-device-unique-id>": { "id": 2, "sub": 0, "name": "Speech" },
      "ip_192.168.1.40":            { "id": 5, "sub": 1, "name": "Music"  }
    }
  }
}
```

- Scene map keys are display names (order as encountered).
- Each member key is either `uid_<hex>` (module addressed via discovery) or `ip_<dotted-quad>` (fixed address, `uid == 0`).
- `id`/`sub` are the preset identifiers sent in the recall packet; `name` is informational.

### 3.3 Known prototype limitations (gap analysis)

| # | Limitation | Evidence | Consequence |
|---|-----------|----------|-------------|
| G1 | Scene recall blocks UI and input | `activateSelected` calls `recallAction` serially with `delay()` loops (:143–155, :324–331) | Dial is frozen for seconds; no cancel; no per-module progress |
| G2 | No render/protocol separation | `pumpUdp` shares `loop()` with drawing and encoder (:374–405) | Any slow path causes visible jank |
| G3 | Raw text UI, no animation | `drawMain` full-screen redraws (:259–304) | Flicker on every encoder step; no visual identity |
| G4 | Encoder feel is coarse | ±2 counts + 80 ms time debounce (:382) | Detent steps can be dropped or doubled |
| G5 | Blocking WiFi connect at boot | 15 s busy-wait in `setup()` (:354–356) | Blank "starte…" screen; no feedback or retry UI |
| G6 | Arduino `String` config plumbing | `cfgJson` re-parsed from a heap `String` (:159–210) | Fragmentation risk; presets size bounded by heap luck |
| G7 | No validation feedback on presets paste | `handleSave` stores anything and reboots (:239–249) | A JSON typo silently yields "Keine Szenen" |
| G8 | Discovery entries never expire | `found` only adds/refreshes, `lastSeen` unused (:109–115) | "online" count can show stale modules |

## 4. PPA UDP Protocol Reference (reconstructed)

> Reconstructed by packet capture of the Four Audio "PPA Group Control" Mac app (per the prototype's header comment, main.cpp:18–19). Everything below is inferred from observed traffic — treat unknown fields as opaque and preserve them exactly.

Transport: **UDP, port 5001**, both directions (the controller binds 5001 locally and modules reply to it). All multi-byte fields are **little-endian**.

### 4.1 Common 12-byte header

| Offset | Size | Field | Notes |
|--------|------|-------|-------|
| 0 | 1 | `type` | 0x00 = ping/presence, 0x04 = preset recall |
| 1 | 1 | `version?` | Always 0x01 in observed traffic |
| 2 | 2 | `status` | LE u16. Requests: 0x0006 (ping), 0x0102 (recall). Replies: low byte is the result kind (§4.4) |
| 4 | 4 | `uid` | LE u32 DeviceUniqueId. 0 in requests; modules fill in their own uid in replies (parsed at main.cpp:105) |
| 8 | 2 | `seq` | LE u16 sequence number; replies echo the request's seq. Prototype starts at 0x0100 and increments (:68, :71) |
| 10 | 1 | `comp` | Component selector? 0xFE for ping, 0xFF for recall |
| 11 | 1 | `res` | 0x00 for ping, 0x01 for recall |

### 4.2 Ping / discovery (type 0x00)

Request: 16 bytes — header with `type=0x00`, `status=0x0006`, `comp=0xFE`, `res=0x00`, followed by 4 zero bytes (main.cpp:82–86).

Sent unicast to a known module, to the /24 directed broadcast, and to 255.255.255.255 (:125–130). Every module that hears it replies with a packet whose header carries its `uid`; the reply's source IP establishes the `uid → IP` mapping. Reply kinds 0x01 and 0x09 are both accepted as "module present" (:109) — modules appear to answer pings with 0x09 in some states, so presence detection must not require 0x01.

### 4.3 Preset recall (type 0x04)

Request: 18 bytes — header with `type=0x04`, `status=0x0102`, `comp=0xFF`, `res=0x01`, then payload (main.cpp:88–93):

| Offset | Size | Field |
|--------|------|-------|
| 12 | 1 | 0x00 (unknown, always zero) |
| 13 | 1 | `presetId` |
| 14 | 1 | `presetSub` |
| 15 | 3 | zero padding |

Sent unicast to the module's IP. The module answers with the echoed `seq` and a result kind.

### 4.4 Reply result kinds (low byte of `status`)

| Kind | Meaning | Prototype handling (main.cpp:117–120) |
|------|---------|--------------------------------------|
| 0x01 | OK / ack | Recall confirmed |
| 0x09 | Error — if payload byte 12 == 0x03: **busy** | Busy → wait 500 ms, resend (new seq); other error → abort action |
| 0x41 | Wait / still processing | Keep waiting within the 2.5 s deadline |

### 4.5 Timing and retry parameters (validated by the prototype)

- Ack deadline per attempt: **2.5 s** (:146).
- Attempts per action: **5** (:143).
- Busy back-off: **500 ms** (:150).
- Discovery refresh: **20 s** (:34); the rewrite additionally expires entries not seen for 3 missed cycles (§6.4).
- Each retry uses a **fresh sequence number** — modules may still act on the earlier packet, so recalls must be idempotent (recalling an active preset is harmless in observed behavior).

### 4.6 Open protocol questions

- Header bytes 1/10/11 (`version?`, `comp`, `res`) semantics are guessed from constants; do not vary them.
- `uid` display order: the Mac app's `uid_<hex>` key is parsed as one LE u32 (main.cpp:198); whether Four Audio displays it byte-swapped is unknown — irrelevant for interop as long as both sides use the Mac app's file.
- No status-query message is known; "active scene" is therefore controller-local state, not read back from modules (a module changed by the Mac app will not be reflected on the dial).

## 5. Proposed Solution — Architecture

New top-level project **`0103-m5dial-ppa-dial/`** (ESP-IDF 5.4.1, target `esp32s3`, LVGL 8.3 via `lvgl/lvgl: "^8.3.0"` in `main/idf_component.yml`, same as `0072-m5dial-timer-demo/main/idf_component.yml`).

### 5.1 Component map

```
0103-m5dial-ppa-dial/
├── CMakeLists.txt / sdkconfig.defaults / partitions.csv
├── main/
│   ├── app_main.cpp            — boot orchestration, task wiring
│   ├── m5dial_board.{h,cpp}    — COPIED from 0072 (GC9A01/LovyanGFX, encoder, touch, button ISR)
│   ├── lvgl_port_m5dial.{h,cpp}— COPIED from 0072 (lvgl_port_m5dial_init + LvglPortM5DialConfig)
│   ├── input_events.h          — COPIED from 0072 (InputEvent: kEncoderDelta / kButtonShortPress / …)
│   ├── wifi_mgr.{h,c}          — ADAPTED from 0095 wifi_app (STA + AP fallback, status struct)
│   ├── config_store.{h,cpp}    — NVS blob: ssid, pass, presets.json text; parse → SceneModel
│   ├── ppa_proto.{h,c}         — pure packet encode/decode (no sockets; unit-testable on host)
│   ├── ppa_client.{h,cpp}      — FreeRTOS task: UDP socket, discovery cache, recall engine
│   ├── web_setup.{h,cpp}       — esp_http_server: setup form, /save with JSON validation
│   ├── scene_model.{h,cpp}     — scenes/actions parsed from presets.json (MVC "model")
│   ├── ui_controller.{h,cpp}   — input events + ppa_client events → screen updates (MVC "controller")
│   └── ui/                     — LVGL screens (MVC "view")
│       ├── ui_theme.{h,cpp}          — colors, fonts, styles (dark theme, green/red accents)
│       ├── ui_scene_carousel.{h,cpp} — main screen
│       ├── ui_activation.{h,cpp}     — progress overlay + result
│       └── ui_status.{h,cpp}         — WiFi-down / no-scenes / provisioning screens
└── tools/
    └── ppa_sim.py              — host-side PPA module simulator (see §8.1)
```

Board bring-up, LVGL port, and the input-event queue are lifted verbatim from `0072-m5dial-timer-demo/main/` (namespace renamed): `M5DialBoard::init/poll` + `read_touch` and `lvgl_port_m5dial_init(display, LvglPortM5DialConfig{...})` are already proven on this exact hardware. The MVC split (model / controller / screen) follows `0073-m5dial-film-developer-timer/main/`. The WiFi layer adapts `0095-m5dial-wifi-bench/main/wifi_app.{h,c}` (`wifi_app_start`, `wifi_app_get_status`, `wifi_app_set_credentials`, STA/AP state machine) rather than reinventing it.

### 5.2 Task model

| Task | Prio | Owns | Communicates via |
|------|------|------|------------------|
| `lvgl` (from lvgl_port) | 4 | display flush, lv_timer_handler | — |
| `input` (board poll) | 5 | encoder/button/touch sampling | `QueueHandle_t input_q` → controller |
| `ppa_client` | 6 | UDP socket (bound :5001), discovery cache, recall engine | command queue in, event queue out |
| `ui_controller` | 4 | scene model, UI state machine | consumes input_q + ppa event queue, mutates LVGL under the port lock |
| `httpd` (esp_http_server) | default | setup endpoints | calls `config_store`, triggers reboot |

Key invariant fixing G1/G2: **only `ppa_client` touches the socket; only the controller (holding the LVGL lock) touches widgets.** A scene activation is a command message; progress and completion come back as events. The UI stays at 60 fps regardless of module timeouts, and rendering can show per-module progress ("2/3…") and offer cancel (long-press) — both impossible in the prototype.

### 5.3 `ppa_client` API sketch

```cpp
// ppa_client.h
enum class PpaEventType { kDiscoveryUpdate, kRecallProgress, kRecallDone };

struct PpaEvent {
  PpaEventType type;
  int scene_index;      // for recall events
  int actions_done;     // confirmed so far
  int actions_total;
  bool ok;              // kRecallDone: all confirmed
  uint32_t online_mask; // kDiscoveryUpdate: bit per action of current scene
};

bool ppa_client_start(QueueHandle_t event_out);
void ppa_client_set_scenes(const SceneModel &model);   // called after (re)parse
void ppa_client_request_recall(int scene_index);        // async; queued
void ppa_client_cancel_recall();                        // long-press cancel
int  ppa_client_online_count(int scene_index);          // from discovery cache
```

Internally the recall engine is a small state machine per action — `SEND → AWAIT_ACK → (OK | BUSY_BACKOFF → SEND | FAIL)` — driven by a `select()`-style loop over the socket with a 50 ms tick, using the timing constants from §4.5. Packet encode/decode lives in `ppa_proto.{h,c}` with plain-struct in/out so it compiles host-side for unit tests. Discovery entries carry `last_seen` and expire after 3 missed refresh cycles (60 s), fixing G8.

Actions within a scene are recalled **concurrently** (one in-flight recall per module, distinct seq numbers) rather than the prototype's serial loop — a 3-module scene with one dead module completes in ~2.5 s instead of blocking the others behind 12.5 s of retries.

### 5.4 UI design (the "sick" part)

Visual identity: dark background (#111), white/grey text, PPA-green (#14A038) for active/ok, red (#C03020) for errors — inherited from the prototype so the dial matches the Mac app's look. LVGL theme in `ui_theme.cpp`; Montserrat 28/20/14 built-in fonts (anti-aliased, replacing M5GFX bitmap fonts, fixing G3).

**Scene carousel (main screen).** The 240×240 round display shows the selected scene name large in the center. Encoder rotation animates scene names sliding horizontally (`lv_anim`, 150 ms, ease-out), rather than a hard redraw. Around the rim:

- **Top arc: position dots** (as in the prototype's dot strip, but drawn on the circular rim where they belong on a round display) — selected dot enlarged, active scene's dot green.
- **Bottom arc: reachability gauge** — an `lv_arc` segment per module in the selected scene, green if discovered, red if not, updating live from `kDiscoveryUpdate` events. Replaces the "2/2 online" text with a glanceable ring (text retained beneath, smaller).

Encoder handling consumes `InputEvent::kEncoderDelta` from the 0072 input queue and accumulates raw counts to **4 counts = 1 detent** (the M5Dial encoder's mechanical detent), which fixes the dropped/doubled steps of G4 — no time-based debounce needed.

**Activation overlay.** On press: a full-screen overlay with the scene name, an `lv_arc` spinner that fills as `kRecallProgress` events arrive (per-module ticks), and a cancel hint. On `kRecallDone`: green check + "AKTIV" or red cross + "n/m Module" (localized strings in one table; German default to match the prototype's audience, see §7 D6). Overlay auto-dismisses after 900 ms back to the carousel. Buzzer feedback via the board speaker mirrors the prototype (short tick per detent, high beep on success, low buzz on failure).

**Connectivity states** (fix G5): boot shows a splash with an indeterminate spinner during WiFi connect (association runs async in `wifi_mgr`; UI is alive immediately). Failure → provisioning screen with AP name/password and `http://192.168.4.1` rendered as text + instructions; connected-but-no-scenes → "no scenes" screen pointing at `http://ppadial.local`.

### 5.5 Configuration and provisioning

- **Storage:** NVS (`config_store`), three entries: `ssid`, `pass`, `presets` (blob, the raw pasted JSON — kept verbatim so re-opening the setup page shows exactly what was pasted). NVS replaces LittleFS because config is the only persisted data and NVS needs no partition-format step or filesystem component (decision D3). Parsing into `SceneModel` uses cJSON (bundled with ESP-IDF) at boot and after save, fixing G6.
- **Web setup** (`web_setup.cpp`, esp_http_server): same two-field + textarea form as the prototype (§3.1), served in both AP and STA modes. `/save` now **validates the pasted JSON before persisting** — parse errors return the form with an inline error message and nothing is saved (fixes G7); success persists, replies with a confirmation page, then reboots after 800 ms.
- **mDNS:** `ppadial.local` in STA mode via ESP-IDF `mdns` component (parity with prototype main.cpp:362).
- AP credentials unchanged: `PPA-Dial` / `ppadial123` — the German ANLEITUNG.md instructions keep working as written.

## 6. Design Decisions

**D1 — ESP-IDF + LVGL rewrite instead of evolving the Arduino prototype.** *Status: accepted (user decision).* Context: prototype is PlatformIO/Arduino; every other firmware in this repo is ESP-IDF with shared, proven M5Dial bring-up code. Options: evolve prototype; rewrite on ESP-IDF; hybrid (Arduino as IDF component). Decision: rewrite. Rationale: reuse of 0072/0073/0095 components, first-class FreeRTOS task control needed for the non-blocking protocol engine, LVGL animation quality, repo consistency. Consequence: the "no programming, just click Upload in PlatformIO" end-user flashing story from ANLEITUNG.md is lost; mitigation is distributing a prebuilt binary flashable with `esptool`/web flasher (noted as follow-up, not in scope).

**D2 — LVGL 8.3 rather than raw LovyanGFX drawing.** *Status: accepted.* Animations, arcs, font rendering, and dirty-region redraws come free; the repo already has a working LVGL port for this display (`0072/main/lvgl_port_m5dial.{h,cpp}`). Raw LovyanGFX would mean hand-rolling animation timing and partial redraws. Cost: ~150 KB flash, one more task — irrelevant on ESP32-S3.

**D3 — NVS instead of LittleFS for config.** *Status: accepted.* The only persisted data is three strings (presets.json observed ≤ a few KB; NVS blob limit ~508 KB with a sized partition, default entry ~4000 B per blob chunk — sufficient, but the partition table will include a 64 KB dedicated `nvs` partition regardless). LittleFS would add a component dependency and a mount/format lifecycle for no benefit. Consequence: prototype configs don't migrate — acceptable, users re-provision once.

**D4 — Dedicated `ppa_client` task with message queues, not loop-polling.** *Status: accepted.* This is the structural fix for G1/G2 and the enabler for progress UI, cancellation, and concurrent per-module recalls. Alternative (prototype-style cooperative polling) rejected: it cannot express "recall 3 modules concurrently while animating" without a state-machine-in-a-loop that is harder than a task.

**D5 — Provisioning parity (AP + form at 192.168.4.1), no captive portal.** *Status: accepted.* Captive-portal DNS hijack adds a DNS server task and OS-specific quirks for marginal UX gain; the printed ANLEITUNG already teaches the manual flow. Revisit if end users stumble.

**D6 — UI strings localized via a single table, German default.** *Status: proposed.* The device's audience (per ANLEITUNG.md) is German-speaking; code and identifiers are English. A 10-entry string table costs nothing and keeps the door open for English.

## 7. Implementation Plan (phased, file-level)

**Phase 1 — Scaffold + board bring-up.** Create `0103-m5dial-ppa-dial/` from 0072's project shape (`CMakeLists.txt`, `sdkconfig.defaults`, `partitions.csv` with 64 KB `nvs`); copy `m5dial_board.*`, `lvgl_port_m5dial.*`, `input_events.h`; `idf.py set-target esp32s3 && idf.py build && idf.py flash monitor` renders a placeholder LVGL label. Uses ESP-IDF **5.4.1** (`source ~/esp/esp-idf-5.4.1/export.sh`, repo `.envrc` default; note the 5.3.4 pin in memory applies to PaperS3, not M5Dial).

**Phase 2 — WiFi + config.** Port `wifi_app.{h,c}` from 0095 into `wifi_mgr` (drop scan/bench extras, add AP-fallback-after-timeout); implement `config_store` (NVS) and `scene_model` (cJSON parse of §3.2 schema with unit-style host tests if convenient). Boot flow: load config → async connect → AP fallback.

**Phase 3 — Protocol engine + simulator.** Implement `ppa_proto.{h,c}` (encode ping/recall, decode header — table §4.1) and `tools/ppa_sim.py` (§8.1) first, then `ppa_client.cpp` against the simulator: discovery cache with expiry, concurrent recall state machines, event emission. Validate with simulator scripted for ok/busy/error/timeout.

**Phase 4 — Web setup.** `web_setup.cpp` on esp_http_server: GET `/` (form pre-filled from config_store), POST `/save` (validate JSON → persist → reboot). mDNS registration. Verify the ANLEITUNG flow end-to-end against a phone.

**Phase 5 — UI.** `ui_theme`, `ui_scene_carousel` (slide animation, rim dots, reachability arc), `ui_activation` overlay, `ui_status` screens; `ui_controller` state machine binding input events + ppa events. Encoder detent accumulation (4 counts/detent).

**Phase 6 — Integration + polish.** Full flow against simulator fleet, then real PPA modules; brightness, speaker feedback, long-press cancel; run the §8.3 manual checklist; README with build/flash/provisioning instructions.

Each phase ends with a diary entry and a changelog update in this ticket.

## 8. Testing and Validation Strategy

### 8.1 Host-side PPA module simulator (`tools/ppa_sim.py`)

Python UDP server (pattern precedent: `0049-xiao-esp32c6-mled-node/tools/mled_ping.py`) binding :5001 on the LAN, configurable as N virtual modules (distinct uids; multiple IPs via `--bind` on aliased interfaces, or a single-IP mode using `ip_` fixed addressing). Behaviors: answer pings with uid; answer recalls with configurable script — `ok`, `busy×k then ok`, `error`, `wait then ok`, `silent` (timeout). Logs every packet hex-decoded against §4.1. This is the primary development harness; no real amplifier needed until Phase 6.

### 8.2 Unit-ish tests

`ppa_proto` encode/decode round-trips compiled host-side (plain C, no ESP-IDF deps). `scene_model` parse tests over: prototype-shaped presets.json, empty scenes, malformed JSON, `uid_`/`ip_` mixes, oversized names.

### 8.3 Manual on-device checklist

1. First boot, no config → provisioning screen; AP join + form + save → reboots into STA.
2. Invalid JSON paste → inline error, config unchanged.
3. Scene rotation: every mechanical detent moves exactly one scene, animation smooth, dots track.
4. Activation with all modules simulated OK → progress fills, green AKTIV, dot turns green.
5. One module `silent` → progress stalls on that module, red result "n/m Module", UI never freezes, encoder still responsive during recall.
6. `busy×2` module → succeeds after back-off (observe simulator log: 3 sends, fresh seqs).
7. Long-press during recall → cancel, return to carousel.
8. Unplug WiFi AP → dial shows disconnected state, recovers on AP return.
9. Discovery expiry: kill a simulator module → its arc segment turns red within 60 s.
10. `http://ppadial.local` reachable in STA mode; re-paste presets → scenes update after reboot.

## 9. Risks, Alternatives, and Open Questions

- **Protocol is reverse-engineered.** Only ping + recall are known; unknown header fields must be sent verbatim (§4.6). Risk: firmware revisions of PPA modules change semantics. Mitigation: keep `ppa_proto` isolated so captures → adjustments are local; keep the simulator authoritative for regression.
- **No state read-back.** The dial's "AKTIV" marker is local; a scene changed from the Mac app won't reflect. Documented behavior; a status-query capture would be the future fix.
- **presets.json schema drift.** The Mac app owns the schema; a future app update could change it. Mitigation: strict-but-tolerant parser (ignore unknown keys), validation error surfaced in the web UI.
- **uid byte-order ambiguity** (§4.6) — interop-safe today; verify against a real module in Phase 6.
- **End-user flashing story** (D1 consequence): decide later between prebuilt-binary + `esptool` instructions or ESP Web Tools flasher page.
- **Concurrent recalls** are new behavior vs. the prototype's serial sends — if real modules misbehave under parallel recalls to *different* devices (unlikely; they are independent), fall back to serial with the same task architecture (one-line change in the engine).

## 10. References

**Prototype (preserved in this ticket):**
- `sources/m5dial-ppa-prototype/src/main.cpp` — full prototype firmware (406 lines)
- `sources/m5dial-ppa-prototype/platformio.ini` — Arduino build config
- `sources/m5dial-ppa-prototype/ANLEITUNG.md` — German end-user flashing/setup instructions

**Repo reuse sources (paths relative to repo root):**
- `0072-m5dial-timer-demo/main/m5dial_board.{h,cpp}` — M5Dial board bring-up (LGFX_M5Dial/GC9A01, ESP32Encoder, touch, button ISR, power hold)
- `0072-m5dial-timer-demo/main/lvgl_port_m5dial.{h,cpp}` — LVGL 8.3 display/tick port (`LvglPortM5DialConfig`)
- `0072-m5dial-timer-demo/main/input_events.h` — InputEvent queue types
- `0072-m5dial-timer-demo/main/idf_component.yml` — IDF ≥5.4 + `lvgl/lvgl ^8.3.0` pins
- `0073-m5dial-film-developer-timer/main/` — model/controller/screen MVC pattern
- `0095-m5dial-wifi-bench/main/wifi_app.{h,c}`, `wifi_console.{h,c}` — native esp_wifi STA/AP management
- `0049-xiao-esp32c6-mled-node/tools/mled_ping.py` — host-side UDP tool pattern for the simulator
- `docs/01-playbook-esp-idf-build-and-dev-environment.md` — ESP-IDF environment/versions playbook
