---
Title: 'Investigation report: device rejoin loop + channel selection stuck on ch13'
Ticket: 0031-ZIGBEE-ORCHESTRATOR
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp-event
    - http
    - websocket
    - protobuf
    - nanopb
    - architecture
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0031-zigbee-orchestrator/main/gw_console_cmds.c
      Note: Console verbs for zb/gw/znsp; used to reproduce and debug
    - Path: 0031-zigbee-orchestrator/main/gw_registry.c
      Note: IEEE-keyed device registry used to see short address churn
    - Path: 0031-zigbee-orchestrator/main/gw_zb.c
      Note: Host↔NCP ZNSP transport
    - Path: 0031-zigbee-orchestrator/main/gw_zb_app_signal.c
      Note: Host-side decode of NCP signals into bus events; announce/rejoin observability
    - Path: thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: Vendored NCP request handlers + security/nvram/channel config; main debug surface
    - Path: thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/partitions.csv
      Note: Vendored partition layout (zb_storage/zb_fct) relevant to channel persistence
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/10-tmux-dual-monitor.sh
      Note: Repro helper for dual monitoring (H2 + host).
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/11-capture-serial.py
      Note: Capture H2 logs without idf.py monitor (no TTY/DTR issues)
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/13-dual-drive-and-capture.py
      Note: Repeatable pairing run capture (host+H2)
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/20-h2-erase-and-flash.sh
      Note: One-shot erase+flash for the H2 NCP.
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/21-h2-erase-zigbee-storage-only.sh
      Note: Targeted erase of zb_fct/zb_storage partitions.
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/Zigbee debug logs.md
      Note: External analysis decoding join/authorization logs; proposes TCLK + link-key exchange hypotheses.
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-05-pairing-lke-on-224457/h2.log
      Note: Pairing evidence (LKE=on)
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-05-pairing-lke-on-224457/host.log
      Note: Pairing evidence (LKE=on)
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-06-exp0b/h2-raw.log
      Note: 'Evidence: H2 received TCLK/LKE/permit_join frames during Experiment 0'
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-06-exp0b/host-exp0.log
      Note: 'Evidence: host console transcript for Experiment 0'
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T21:06:48.8288329-05:00
WhatFor: ""
WhenToUse: ""
---







# Investigation report: device rejoin loop + channel selection stuck on ch13

## Executive summary

We have a working host+NCP Zigbee coordinator path on Cardputer (ESP32‑S3 host) + Unit Gateway H2 (ESP32‑H2 NCP) over UART/SLIP: permit-join works, device announce events (including IEEE) are observed on both sides, and we can intermittently send ZCL OnOff commands to a test power plug and see it physically toggle.

However, the setup is currently unstable: the power plug appears to repeatedly rejoin (short address changes frequently), which makes “send command by short address” unreliable. The initial mitigation attempt was to move the coordinator to a different Zigbee channel (e.g. ch20/ch25) to reduce RF interference, but the NCP continues to form the network on channel 13 even after accepting and logging “applied” channel masks.

This report documents the observed symptoms, our debugging instrumentation, what we tried, and the next concrete experiments to determine whether the instability is caused by channel/Wi‑Fi interference, NCP/coordinator resets, Zigbee persistent storage overriding config, or security/commissioning loops.

## Current symptom(s)

### S1: Device appears to “keep joining”

- A joined device (test power plug) repeatedly issues joins/rejoins; its short address changes often.
- Sometimes a couple of OnOff commands work, then the device “moves” again (new short).

Expected behavior (baseline): after a successful join + key establishment, the device should remain on the network, rejoin should be rare, and the short address should generally remain stable unless the device actually leaves/rejoins or the coordinator re-forms a different network.

### S2: Coordinator formation channel stays on 13 even when we request ch20/ch25

- We can configure and persist a channel mask from the host.
- The NCP logs show it receives and applies the mask successfully.
- Despite that, the NCP forms a network on channel 13.

## Environment + topology

### Hardware

- **Host:** Cardputer (ESP32‑S3)
- **NCP:** Unit Gateway H2 (ESP32‑H2)
- **Device under test:** Zigbee power plug (OnOff cluster)
- **UART wiring (Grove):**
  - H2 TX → Cardputer GPIO1 (RX)
  - H2 RX → Cardputer GPIO2 (TX)
  - Common GND

### Consoles / ports

- Host REPL + logs: USB Serial/JTAG (`/dev/ttyACM1`)
- NCP logs: USB Serial/JTAG (`/dev/ttyACM0`)
- Important: `idf.py monitor` holds the port; flashing requires stopping the monitor process (tmux sessions often need to be killed before reflashing).
- Important: monitor resets can corrupt the host↔NCP UART bus.
  - `idf.py monitor` toggles DTR/RTS by default, which can reset the H2.
  - If the H2 resets while its Grove UART is wired to the host, the H2 boot ROM banner (`ESP-ROM:esp32h2-...`) can spill onto the SLIP/ZNSP link and corrupt framing / in-flight requests.
  - Mitigation: always monitor with `--no-reset` (see `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/10-tmux-dual-monitor.sh`) and avoid hot-plugging USB on the H2 while the host is running.

### Firmware locations (source of truth)

- Host project: `esp32-s3-m5/0031-zigbee-orchestrator/`
- NCP example project (vendored): `thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/`
- NCP Zigbee request handlers (vendored): `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`

## What we can already do (confirmed working path)

### 1) Open permit join and observe status

On host console:

```text
gw post permit_join 180
```

Observed logs (host + NCP):

```text
[gw] GW_CMD_PERMIT_JOIN (1) req_id=3 seconds=180
I (...) gw_zb_0031: permit_join request seconds=180 req_id=3
...
I (...) gw_zb_0031: permit_join response status=0x00 (ESP_OK) req_id=3
[gw] GW_EVT_CMD_RESULT (100) req_id=3 status=0
...
I (...) gw_zb_sig_0031: ZB permit-join status: seconds=180
[gw] GW_EVT_ZB_PERMIT_JOIN_STATUS (200) seconds=180
```

NCP-side confirmation:

```text
ESP_NCP_ZB: Network(0x4641) is open for 180 seconds
```

### 1.1) Confirm security debug controls (TCLK + link-key exchange requirement) end-to-end

We can now drive the key commissioning knobs end-to-end across the host↔NCP boundary:

- Host console can set/get the Trust Center Link Key (“TCLK join key”) via ZNSP 0x0028/0x0027.
- Host console can toggle “link key exchange required” via ZNSP 0x002A/0x0029.

This matters because the external decode of our logs indicates the join instability is very likely a **TCLK authorization timeout** loop. Being able to flip these controls deterministically is the foundation for Experiment 0 (“turn LKE requirement off and see if the device stabilizes”).

Example host transcript (successful request/response status codes):

```text
zb tclk set default
zb tclk set: status=0x00 (ESP_OK)

zb tclk get
zb tclk: tc_ieee=48:31:b7:ff:fe:ca:45:5b key=5a6967426565416c6c69616e63653039

zb lke off
zb lke off: status=0x00 (ESP_OK)

zb lke status
zb lke: off (mode=0)

gw post permit_join 180
... permit_join response status=0x00 (ESP_OK) ...
```

NCP-side frame capture (no TTY required):

- Capture: `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/runs/2026-01-06-exp0b/h2-raw.log`
- Capture tool: `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/11-capture-serial.py`

Notes:

- The ZNSP permit-join request uses an 8-bit seconds field, so we currently clamp any requested value >255 down to 255 on the host. This previously caused confusion when scripts used 300s (it became 255s on-wire). The host now logs when clamping occurs.

### 2) Observe a device join/announce (including IEEE)

NCP:

```text
ESP_NCP_ZB: New device commissioned or rejoined (short: 0xa04d)
```

Host:

```text
[gw] GW_EVT_ZB_DEVICE_ANNCE (201) short=0xa04d ieee=28:2c:02:bf:ff:e6:98:70 cap=0x8e
```

### 3) Send an OnOff command (intermittently succeeds)

Host console (example):

```text
gw post onoff 0xa04d 1 toggle
```

Result: the plug often toggles physically (at least for a short time window before it rejoins/changes short).

## Instrumentation we added specifically for this issue

### Host-side: “monitor”, device registry, and low-level ZNSP injection

We added tooling so we can debug without rewriting code every time:

- **Console monitor:** `monitor on|off|status`
  - When enabled, prints all bus traffic to the console (useful for correlating with NCP logs).
- **Device registry:** `gw devices`
  - In-memory registry keyed by IEEE, tracks latest short and announce count.
- **Raw ZNSP request injector:** `znsp req <id> <hex bytes...>`
  - Allows sending a specific request ID + payload bytes to the NCP.
  - All ZNSP requests are serialized with a mutex to avoid SLIP interleaving.

Relevant files:

- `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_console_cmds.c`
- `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_registry.c`
- `esp32-s3-m5/0031-zigbee-orchestrator/main/gw_zb.c`

### NCP-side: richer join/rejoin/leave logs

We extended the NCP’s Zigbee signal logging so we can answer: “is this really a rejoin, or is the coordinator rebooting, or is security failing?”

Added/expanded signals:

- `DEVICE_ANNCE` (now logs short + IEEE + capability)
- `DEVICE_UPDATE` (logs status + IEEE + short)
- `DEVICE_AUTHORIZED` (logs auth type/status + IEEE + short)
- leave indication (logs rejoin + IEEE)

Relevant file:

- `thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c`

## Channel selection work (what we tried, and what happened)

### Goal

Move coordinator off Zigbee channel 13 (likely near Wi‑Fi ch1) to test if rejoin instability is RF interference.

### Design: host owns the config; NCP applies it safely

High level:

```
Host console  ── "zb ch 25" ──>  host stores mask in NVS
     │
     ├─ on next boot / zb reboot:
     │    send ZNSP_NETWORK_CHANNEL_SET(0x0014) with mask
     │    send ZNSP_NETWORK_PRIMARY_CHANNEL_SET(0x0010) with mask
     │
     ▼
NCP receives requests, stores masks (if Zigbee not inited yet),
applies after esp_zb_init(), and again just before BDB formation.
```

### What worked

- Host: we can set and persist mask values and send them to the NCP.
- NCP: the channel mask application no longer crashes the Zigbee stack (after we deferred it until after init).
- NCP logs show the mask was applied successfully, e.g.:
  - `apply channel mask: 0x02000000 (ESP_OK)`
  - `apply primary channel mask: 0x02000000 (ESP_OK)`

### What failed / open problem

Even with the above, the network formation log still reports:

```text
Formed network successfully ... Channel:13
```

This mismatch (“applied mask OK” vs “formed on channel 13”) is the main unresolved technical issue right now.

### Important failure we hit (and fixed): applying masks too early crashes the NCP

When the host attempted to set the primary channel mask before the Zigbee stack was initialized on the H2, the NCP crashed (abort in the Zigbee init path).

Fix: store the requested masks when received early, but apply them only after `esp_zb_init()` (and then again immediately before formation).

## Working theories (ranked) + how to confirm each

## New evidence: “authorization timeout” strongly indicates a TCLK/link-key exchange problem

A separate decode of our logs (see `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/sources/local/Zigbee debug logs.md`) points out a key detail:

- `Device update status=0x01` means **standard unsecured join**
- `Device authorized type=0x01 status=0x01` means **authorization timeout** (TCLK / key exchange did not complete)

That combination explains the observed behavior perfectly: the device joins at MAC/NWK, but never finishes the security procedure, so it times out and retries → repeated “fresh joins” and short address churn for the same IEEE.

This shifts the highest-signal next steps from “channel only” to “security handshake first, then channel”.

### H1: Zigbee persistent storage overrides our channel config (likely `zb_fct` / `zb_storage`)

Why it fits:

- The NCP accepts the new masks, but formation still uses an old channel.
- The esp-zigbee-ncp example uses dedicated partitions (`zb_fct`, `zb_storage`) that may persist coordinator/network parameters (including channel).

How to test:

- Erase Zigbee storage partitions on the H2 (preferably only those partitions), then re-form:
  - Option A: full erase: `idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp -p /dev/ttyACM0 erase-flash`
  - Option B: targeted erase: use `parttool.py` to erase only `zb_storage` and `zb_fct` (recommended once we confirm names/offsets).
- After erase, set mask (`zb ch 25`) and reboot NCP, then confirm formation channel.

What “success” looks like:

- `Formed network successfully ... Channel:25`

### H0 (new, highest-signal): Trust Center Link Key (TCLK) mismatch OR link-key exchange not completing

Why it fits:

- Our logs indicate repeated “unsecured join” and “authorization timeout” rather than “authorized success”.
- Many Zigbee 3.0 devices expect the default preconfigured Trust Center key `ZigBeeAlliance09` unless using install codes.

How to test (fast, decisive):

1) Check and (re)apply the preconfigured Trust Center key (TCLK) on the NCP:

   - `zb tclk get`
   - `zb tclk set default` (sets `ZigBeeAlliance09`)

2) Confirm diagnosis by temporarily disabling link-key exchange requirement:

   - `zb lke off`

   If the device stabilizes (short stops changing; no more repeated announces), that confirms the join loop is caused by the authorization/key-exchange step.

3) Once stable, re-enable for “secure final” behavior:

   - `zb lke on`

Notes:

- These are implemented via ZNSP requests to the H2:
  - `0x0027/0x0028` (link key get/set)
  - `0x0029/0x002A` (secure mode get/set → mapped to “link key exchange required”)

### H2: We are applying the mask at the wrong time relative to BDB formation

Why it fits:

- Zigbee BDB formation/start sequencing is timing-sensitive.
- Some “signal handlers” fire after formation has started, which would make “apply before formation” a no-op.

How to test:

- Move the apply point earlier in the NCP lifecycle:
  - apply masks directly in the NCP example’s `app_main()` after `esp_zb_init()` but before `esp_zb_start(...)`.
- Add explicit logging around BDB start:
  - dump current mask values right before and right after calling start/formation.

### H3: The NCP forms on channel 13 due to an internal fallback

Why it fits:

- Some stacks silently fall back if a requested channel is invalid/unavailable, or if they interpret “primary” vs “mask” differently.

How to test:

- Confirm requested mask is valid at the Zigbee layer (not just the NCP transport):
  - call an API to read back the configured channel mask / primary mask (if available).
- Try a different single-channel mask (e.g. ch20) and observe whether formation changes at all.

### H4: The instability is not RF/channel-related; device is actually leaving/rejoining for security reasons

Why it fits:

- The plug’s behavior feels like “join, works briefly, then rejoin”.
- Security/authorization failures can cause rejoin loops.

How to test:

- Use the new `DEVICE_AUTHORIZED` and `DEVICE_UPDATE` logs to see whether authorization completes successfully.
- Close permit join after the device joins (`gw post permit_join 0`) and observe whether the device remains stable.
- If available, log key establishment / trust center status on the NCP side (may require deeper Zigbee SDK hooks).

### H5: The coordinator or NCP is rebooting (power/pins/WD), causing the device to rejoin

Why it fits:

- If the coordinator reboots or reforms a network, devices will rejoin and may receive new shorts.

How to test:

- Watch the NCP logs for repeated “formed network successfully” and ROM banners.
- Add a monotonic boot counter to the NCP log to make reboots obvious.

## What to do next (concrete experiment plan)

### Experiment 0: Fix/confirm the authorization step (before deep channel work)

1) Set and verify the Trust Center preconfigured key:
   - `zb tclk set default`
   - `zb tclk get` (confirm the key is `5a6967426565416c6c69616e63653039`)
2) Pair the plug again (factory-reset the plug first).
3) If you still see `authorization timeout`, do a diagnostic run:
   - `zb lke off`
   - pair again with the plug physically close to the coordinator
4) If this stabilizes the device, proceed to Experiment 1 to resolve channel selection and then re-enable `zb lke on`.

### Experiment 1: Erase Zigbee storage partitions on H2 and retest channel selection

1) Stop monitors so the ports are free.
2) Erase flash on H2:
   - `idf.py -C thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp -p /dev/ttyACM0 erase-flash`
3) Flash H2, monitor, and confirm it forms again.
4) On host:
   - `zb ch 25`
   - `zb reboot`
5) Confirm formation channel in NCP log.

Notes / helpers:

- Full erase + flash helper: `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/20-h2-erase-and-flash.sh`
- Targeted storage wipe (preferred once firmware is stable): `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/21-h2-erase-zigbee-storage-only.sh`
- Dual monitor helper: `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/10-tmux-dual-monitor.sh`
- Offline PDFs for reMarkable/manual transfer: `ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/exports/remarkable-pdf/`

Alternative (debug, “erase every start”):

- Use `zb nvram on` to enable `esp_zb_nvram_erase_at_start(true)` on the H2 (clears `zb_storage` before parameters are loaded at `esp_zb_start()`), then re-form/restart and observe the channel again.

### Experiment 2: After channel is fixed, measure “rejoin frequency” objectively

1) `gw post permit_join 180`, pair plug, then `gw post permit_join 0`.
2) Leave system running for 10+ minutes.
3) Track:
   - how many `DEVICE_ANNCE` events occurred
   - how many unique short addresses were used for the same IEEE
4) If rejoin still frequent, treat as a security/authorization issue rather than RF.

### Experiment 3: Add one more debug verb: “dump NCP channel + pan” (readback)

We should add an explicit readback request on the NCP side (or a `znsp` response) so the host can query:

- current channel
- PAN ID / extended PAN ID
- network open/closed status

If there is no existing ZNSP request for this, we can add a small custom request ID for debugging.

## Appendix A: Where to look in code (symbols and responsibilities)

### Host (Cardputer)

- `gw_zb.c`
  - NCP transport bring-up, ZNSP request/response wrappers, serialized transaction mutex
  - channel mask persistence and apply path
- `gw_zb_app_signal.c`
  - decodes NCP “signals” into `GW_EVT_*` bus events
- `gw_registry.c`
  - stores IEEE→short mapping and join statistics
- `gw_console_cmds.c`
  - `gw post ...`, `gw devices`, `zb ...`, `znsp req ...`, `monitor ...`

### NCP (H2)

- `esp_ncp_zb.c`
  - request handlers (permit-join, channel mask set, primary channel set)
  - Zigbee signal handler logs
- `examples/esp_zigbee_ncp/partitions.csv`
  - defines `zb_fct` and `zb_storage` partitions (likely relevant to channel persistence/override)

## Appendix B: Quick diagram of data flow (join + command)

```
                 (USB Serial/JTAG)                  (USB Serial/JTAG)
 Dev PC ───────────────┐                          ┌────────────────── Dev PC
                       │                          │
                 Cardputer host             Unit Gateway H2 (NCP)
                 (ESP32‑S3)                 (ESP32‑H2 + Zigbee stack)
                       │                          │
 console: gw/zb/znsp    │   ZNSP over SLIP UART    │ Zigbee RF
 ──────────────────────┼──────────────────────────┼───────────────>
                       │                          │
 esp_event bus          │                          │
  - GW_CMD_PERMIT_JOIN  │                          │  permit join open
  - GW_EVT_DEVICE_ANNCE │                          │  device announces
  - GW_CMD_ONOFF        │                          │  ZCL OnOff cmd
  - GW_EVT_CMD_RESULT   │                          │
```
