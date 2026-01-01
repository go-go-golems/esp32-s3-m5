---
Title: 'Design: Dual security registries + unified peer registry (LE + BR/EDR)'
Ticket: 0020-BLE-KEYBOARD
Status: active
Topics:
    - ble
    - keyboard
    - esp32s3
    - console
    - cardputer
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0020-cardputer-ble-keyboard-host/main/ble_host.c
      Note: Current BLE-only controller init; would become dual-mode host init
    - Path: 0020-cardputer-ble-keyboard-host/main/bt_console.c
      Note: Console surface that will grow scan le/bredr
    - Path: 0020-cardputer-ble-keyboard-host/main/hid_host.c
      Note: Receives HID reports via esp_hidh; expected to work for both transports
    - Path: 0020-cardputer-ble-keyboard-host/sdkconfig.defaults
      Note: Current BT config disables Classic; dual-mode requires BT_CLASSIC + HID host
    - Path: ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/analysis/05-ble-vs-br-edr-security-registries-bond-databases-why-they-aren-t-trivially-mergeable.md
      Note: Background on why the registries remain separate
    - Path: ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/tasks.md
      Note: Tracks the implementation steps for dual-mode support
ExternalSources: []
Summary: 'Design doc for supporting both BLE (LE) and Classic (BR/EDR) keyboards: dual discovery + dual bond registries with a unified peer view and explicit mapping rules.'
LastUpdated: 2025-12-31T19:06:51.925446039-05:00
WhatFor: ""
WhenToUse: ""
---



# Design: Dual security registries + unified peer registry (LE + BR/EDR)

## Executive Summary

We want the firmware to support keyboards that are discoverable on **Classic BR/EDR** (your current keyboard) and also continue to support **BLE HID over GATT (HOGP)** keyboards.

Key constraint: even in dual-mode (`BTDM`), Bluetooth security state is **not a single registry**. There are effectively **two security registries** (two bond/key databases) and **two discovery mechanisms**:

- **LE**: advertising scan → LE SMP bonding keys (LTK/IRK/CSRK, plus addr_type)
- **BR/EDR**: inquiry → Classic pairing keys (link key), stored/managed separately

This design keeps these registries separate (because the stack does), but exposes a **unified “peer registry”** to the console that merges:

- LE scan registry
- BR/EDR inquiry registry
- LE bond list
- BR/EDR bond list

…into one human-friendly view (`peers`) while preserving transport-specific semantics for pairing/bond management.

## Problem Statement

Today:

- Linux sees your keyboard immediately in `bluetoothctl scan bredr`, but our firmware does not see it at all.
- Root cause: our firmware is **BLE-only** (it releases Classic memory, enables BLE-only controller mode, and disables Classic in `sdkconfig.defaults`), so it cannot do inquiry or Classic HID host.

Even after we enable dual-mode:

- We must reconcile two discovery sources (LE vs BR/EDR).
- We must handle two different pairing prompt models (SMP vs SSP/PIN).
- We must present a coherent operator experience (scan → identify → connect/pair → keylog) without implying that “security is unified”.

This design addresses:

1) How to structure **dual registries** (discovery + security per transport).
2) How to build a **unified peer view** safely.
3) What data structures and invariants we rely on so we can implement dual-mode without accidental key/address confusion.

## Proposed Solution

### High-level architecture

Split “device knowledge” into four sources, then merge:

```
            (LE advertising)                    (BR/EDR inquiry)
                 |                                   |
                 v                                   v
      +----------------------+           +------------------------+
      | le_discovery_reg     |           | bredr_discovery_reg    |
      | (addr_type,rssi,name)|           | (bd_addr,cod,name,...) |
      +----------+-----------+           +-----------+------------+
                 |                                   |
                 +----------------+------------------+
                                  |
                                  v
                         +------------------+
                         | peer_registry    |
                         | (merged view)    |
                         +----+--------+----+
                              |        |
                    +---------+        +----------+
                    |                             |
                    v                             v
       +------------------------+     +---------------------------+
       | le_security_registry   |     | bredr_security_registry    |
       | (esp_ble_* bond list)  |     | (esp_bt_gap_* bond list)   |
       +------------------------+     +---------------------------+
```

The console reads from `peer_registry` for “what devices exist?” while transport-specific commands operate on the correct underlying security registry.

### “Two registries” clarification

When we say “security registries” we mean the bond databases (and security state machines) the stack maintains:

- **LE bond registry**: `esp_ble_get_bond_device_list` returns `esp_ble_bond_dev_t` including key masks and address type.
- **BR/EDR bond registry**: `esp_bt_gap_get_bond_device_list` returns a list of BD_ADDRs (Classic security database).

We do not attempt to physically merge these databases. Instead we create a merged *application-level* representation that can reference either/both.

### Proposed console UX (design target)

Transport-specific commands remain explicit:

- Scans:
  - `scan le on 10`
  - `scan bredr on 10`
- Device tables:
  - `devices le [substr]`
  - `devices bredr [substr]`
- Bonds:
  - `bonds le`
  - `bonds bredr`
  - `unpair le <addr>`
  - `unpair bredr <addr>`

Unified view:

- `peers [substr]` shows merged records with columns indicating which transports are known, seen recently, and bonded.

Connection/pairing:

- `connect <peer-id-or-idx>` selects transport based on peer record (or requires `connect le|bredr ...` if ambiguous).
- `pair <peer-id-or-idx>` similarly selects correct pairing flow.

### Controller mode change (precondition)

Dual-mode support requires both build-time and runtime changes:

Build-time (Kconfig):

- enable Classic: `CONFIG_BT_CLASSIC_ENABLED=y`
- enable Classic HID Host: `CONFIG_BT_HID_ENABLED=y`, `CONFIG_BT_HID_HOST_ENABLED=y`

Runtime (controller mode):

- stop calling `esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)`
- enable `ESP_BT_MODE_BTDM`

Current BLE-only init in `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c`:

```c
ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
```

Dual-mode target:

```c
ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BTDM));
```

## Data structures

### Transport and identity types

We need a stable way to refer to “a peer”, but the address models differ.

```c
typedef enum {
    PEER_TRANSPORT_LE = 1,
    PEER_TRANSPORT_BREDR = 2,
} peer_transport_t;

typedef struct {
    uint8_t bda[6];          // identity address
    uint8_t addr_type;       // public|random (esp_ble_addr_type_t)
} le_identity_t;

typedef struct {
    uint8_t bda[6];          // classic BD_ADDR
} bredr_identity_t;
```

### Discovery records (ephemeral)

LE discovery records are derived from advertising reports.

```c
typedef struct {
    uint8_t adv_addr[6];     // what we saw on the air
    uint8_t addr_type;       // pub|rand
    int rssi;
    uint32_t last_seen_ms;
    char name[32];           // from ADV+SCAN_RSP EIR
} le_discovered_t;
```

BR/EDR discovery records are derived from inquiry results + optional name discovery.

```c
typedef struct {
    uint8_t bda[6];
    uint32_t cod;            // Class of Device (helps identify keyboards)
    int8_t rssi;             // optional / if available
    uint32_t last_seen_ms;
    char name[64];           // from EIR or remote name request
} bredr_discovered_t;
```

### Security (bond) snapshots

LE bond snapshot is richer:

```c
// From esp_gap_ble_api.h:
// typedef struct { esp_bd_addr_t bd_addr; esp_ble_bond_key_info_t bond_key; esp_ble_addr_type_t bd_addr_type; } esp_ble_bond_dev_t;
typedef struct {
    le_identity_t id;
    uint8_t key_mask;        // bond_key.key_mask (presence)
} le_bond_snapshot_t;
```

BR/EDR bond snapshot is a list of BD_ADDRs:

```c
typedef struct {
    bredr_identity_t id;
} bredr_bond_snapshot_t;
```

### Unified peer record (the “merged registry”)

Core idea: a peer record can have LE info, BR/EDR info, or both — but both are optional.

```c
typedef struct {
    // Chosen by the app. The simplest working option:
    // - if classic-only: id = bredr BDA
    // - if LE-only: id = LE identity (addr + type)
    peer_transport_t primary_transport;

    bool has_le;
    le_identity_t le_id;
    le_discovered_t le_last_seen;       // optional snapshot
    bool le_bonded;
    uint8_t le_key_mask;

    bool has_bredr;
    bredr_identity_t bredr_id;
    bredr_discovered_t bredr_last_seen; // optional snapshot
    bool bredr_bonded;

    // Operator-facing fields
    char alias[64];      // app-managed (optional)
    char display_name[64]; // best-effort name (see mapping rules)
} peer_record_t;
```

## Mapping rules (how “one physical device” relates to the two transports)

### Non-goal: “automatic perfect mapping”

We explicitly do not assume:

- a dual-mode keyboard uses CTKD,
- a Classic BD_ADDR equals a BLE identity address,
- a BLE advertising address stays stable (RPAs rotate).

### Default mapping strategy

1) If we only ever discover the keyboard in BR/EDR inquiry:
   - create a peer record with `has_bredr=true`, `has_le=false`.
2) If we only ever discover it in LE scan:
   - create a peer record with `has_le=true`, `has_bredr=false`.
3) If we discover both transports and can prove identity:
   - merge into one peer record.

“Prove identity” candidates (in descending confidence):

- shared public address *and* consistent manufacturer/company identifiers (rare)
- explicit CTKD-derived relationship (if stack exposes it; often not)
- manual operator link (best pragmatic solution)

### Manual linking (recommended for reliability)

Expose a console command:

- `peer link <bredr-addr> <le-addr>/<type>`

Store that mapping in app-owned NVS so future runs “know” the association without guessing.

## Security prompt handling (two different flows)

### BLE pairing prompts (SMP)

Events we already handle in `ble_host.c`:

- `ESP_GAP_BLE_SEC_REQ_EVT` → accept/reject
- `ESP_GAP_BLE_PASSKEY_REQ_EVT` → user enters passkey
- `ESP_GAP_BLE_NC_REQ_EVT` → user confirms numeric compare

### Classic pairing prompts (SSP/PIN)

Dual-mode will add handling for:

- `ESP_BT_GAP_PIN_REQ_EVT` (legacy PIN)
- `ESP_BT_GAP_CFM_REQ_EVT` (numeric confirm)
- `ESP_BT_GAP_KEY_REQ_EVT` / `ESP_BT_GAP_KEY_NOTIF_EVT` (passkey entry/notify)
- `ESP_BT_GAP_AUTH_CMPL_EVT` (auth complete)

Design decision: we keep separate command families to avoid mixing semantics:

- `le passkey ...` / `le confirm ...` (existing)
- `bredr pin ...` / `bredr confirm ...` / `bredr passkey ...` (new)

We can later add a unified “pending prompt” system that prints:

```
[AUTH] transport=bredr addr=... type=PIN_REQ
```

…and allows a transport-agnostic response command, but it must dispatch correctly.

## Design decisions

### Keep registries separate; merge only at the view layer

Rationale:

- the stack stores different key material,
- the APIs differ (BLE bond list includes addr_type and key mask; Classic is BD_ADDR list),
- the pairing state machines are distinct.

### Make transport explicit whenever ambiguity exists

If a peer record has both transports, or if an address could be interpreted as either:

- require `connect le ...` / `connect bredr ...`
- or require a peer-id that encodes transport.

### Avoid “both scans simultaneously” initially

First implementation should allow only one active scan mode at a time:

- `scan le on`
- `scan bredr on`

Rationale:

- radio time-sharing contention is real,
- it simplifies operator expectations and debugging.

## Alternatives Considered

### Alternative A: “Just unify the security DB”

Rejected: not feasible without CTKD guarantees and stack-level support, and it would be incorrect for many devices. See analysis doc `analysis/05-...security-registries...`.

### Alternative B: “Only support Classic (BR/EDR) keyboards”

Viable if the project goal is strictly “my keyboard works”, but it abandons BLE keyboards and discards existing work (LE scan registry, LE pairing UX, etc.).

### Alternative C: “Two separate firmwares”

Viable operationally (one build for Classic, one for BLE), but increases maintenance and makes the console UX inconsistent.

## Implementation Plan (tasks only; no implementation in this step)

This design corresponds to tasks added to `tasks.md` under the `[Dual-mode]` prefix:

1) Enable Classic + HID Host in `sdkconfig.defaults` (and keep BLE enabled if dual-mode).
2) Switch controller mode to BTDM.
3) Add Classic GAP discovery callback + inquiry registry.
4) Add Classic pairing prompts and response commands.
5) Add Classic bonds list and unpair.
6) Add `peers` merged view.
7) Add explicit mapping rules + optional manual linking.
8) Validate against a Classic-only keyboard and a BLE keyboard.

## Open Questions

- What should be the stable “peer id” encoding in the console?
  - `bt:AA:BB:...` vs `le:AA:BB.../rand` etc.
- Do we want CTKD research as part of MVP, or treat it as “nice to have”?
- How much Classic name discovery should we do (it can create connections during inquiry, which can surprise users)?
- How to avoid index instability when mixing pruning + two registries + merged view?

## References

- Analysis: why keyboard shows in `bluetoothctl` but not LE scan:
  - `esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/analysis/03-why-keyboard-shows-in-bluetoothctl-but-not-on-esp32-scan.md`
- Analysis: how to add Classic HID host / dual-mode:
  - `esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/analysis/04-adding-classic-br-edr-hid-keyboard-support-and-dual-mode-in-esp-idf.md`
- Analysis: why security registries aren’t trivially mergeable:
  - `esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/analysis/05-ble-vs-br-edr-security-registries-bond-databases-why-they-aren-t-trivially-mergeable.md`
- ESP-IDF headers:
  - `/home/manuel/esp/esp-idf-5.3.4/components/bt/host/bluedroid/api/include/api/esp_gap_ble_api.h`
  - `/home/manuel/esp/esp-idf-5.3.4/components/bt/host/bluedroid/api/include/api/esp_gap_bt_api.h`

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
