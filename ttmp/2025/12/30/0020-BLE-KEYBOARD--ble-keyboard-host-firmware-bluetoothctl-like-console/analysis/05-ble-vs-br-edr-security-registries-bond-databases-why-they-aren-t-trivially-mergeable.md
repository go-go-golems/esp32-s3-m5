---
Title: 'BLE vs BR/EDR security registries (bond databases): why they aren’t trivially mergeable'
Ticket: 0020-BLE-KEYBOARD
Status: active
Topics:
    - ble
    - keyboard
    - esp32s3
    - console
    - cardputer
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.3.4/components/bt/host/bluedroid/api/include/api/esp_gap_ble_api.h
      Note: Defines BLE bond key structs and BLE bond list APIs
    - Path: ../../../../../../../../../../esp/esp-idf-5.3.4/components/bt/host/bluedroid/api/include/api/esp_gap_bt_api.h
      Note: Defines Classic bond list APIs and Classic pairing events
    - Path: 0020-cardputer-ble-keyboard-host/main/ble_host.c
      Note: Shows current BLE bond-list usage and LE security event handling
ExternalSources: []
Summary: Explains BLE (SMP) vs Classic (BR/EDR) security databases, why their bond/key registries can’t be naively merged, and how to design a safe unified application-level peer registry (including CTKD caveats).
LastUpdated: 2025-12-31T19:01:16.679748594-05:00
WhatFor: ""
WhenToUse: ""
---



# BLE vs BR/EDR security registries (bond databases): why they aren’t trivially mergeable

## Summary

Even on “dual-mode” Bluetooth devices, BLE security state (SMP bonding) and Classic BR/EDR security state (link keys/SSP) are not the same thing and are not stored in the same format. You can build a unified application-level “device registry” that references both, but you can’t just “combine them into one registry” without losing critical invariants:

- the key material is different (LTK/IRK/CSRK vs Classic link key),
- the addressing model is different (LE address type + privacy/RPAs vs Classic BD_ADDR),
- the pairing protocols and events are different (SMP vs SSP/PIN),
- cross-transport key derivation (CTKD) is optional and not always allowed/supported.

This document explains how the two registries work, what ESP-IDF exposes for each, and how to design a “combined view” safely.

## Terms (short glossary)

- Bond: long-term security relationship (stores keys so future connections can encrypt/authenticate without re-pairing).
- Pairing: the interactive protocol run to establish a bond (passkey, numeric compare, “just works”, etc.).
- SMP: Security Manager Protocol (BLE pairing/bonding).
- SSP: Secure Simple Pairing (Classic pairing; includes numeric comparison/passkey).
- LTK: Long Term Key (BLE encryption key).
- IRK: Identity Resolving Key (BLE privacy; used to resolve RPAs).
- CSRK: Connection Signature Resolving Key (BLE signed data; less common for HID keyboards).
- BD_ADDR: Classic Bluetooth device address.
- RPA: Resolvable Private Address (BLE random address that rotates; resolvable using IRK).
- CTKD: Cross-Transport Key Derivation (deriving BLE keys from BR/EDR keys or vice versa; optional).

## A picture: two separate security stores behind one radio

```
                +------------------------------+
                |          Your App            |
                |  "device registry" / console |
                +---------------+--------------+
                                |
               +----------------+----------------+
               |                                 |
       (LE / BLE stack)                   (Classic stack)
               |                                 |
    +----------v-----------+           +----------v-----------+
    | SMP bond DB (LE)     |           | BR/EDR security DB   |
    | LTK/IRK/CSRK +       |           | link keys + auth     |
    | addr_type + identity |           | state (SSP/PIN)      |
    +----------------------+           +----------------------+
```

Even in dual-mode controller mode (`BTDM`), you still have two protocol worlds above the radio.

## What ESP-IDF exposes: BLE bond registry vs Classic bond registry

### BLE (LE) bond list: `esp_ble_get_bond_device_list`

ESP-IDF exposes BLE bonded devices via `esp_ble_bond_dev_t` (in `esp_gap_ble_api.h`):

```c
typedef struct {
    esp_bd_addr_t           bd_addr;       // peer address
    esp_ble_bond_key_info_t bond_key;      // key mask + (penc/pcsrk/pid)
    esp_ble_addr_type_t     bd_addr_type;  // PUBLIC vs RANDOM
} esp_ble_bond_dev_t;
```

The important part: BLE bonding is tied to the LE address type and can involve privacy/identity keys.

In our firmware today (`esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c`), we list BLE bonds using:

```c
int n = esp_ble_get_bond_device_num();
esp_ble_bond_dev_t list[16];
esp_ble_get_bond_device_list(&n, list);
```

### Classic (BR/EDR) bond list: `esp_bt_gap_get_bond_device_list`

ESP-IDF exposes Classic bonded devices as a list of BD_ADDRs (in `esp_gap_bt_api.h`):

```c
int esp_bt_gap_get_bond_device_num(void);
esp_err_t esp_bt_gap_get_bond_device_list(int *dev_num, esp_bd_addr_t *dev_list);
```

Note what’s missing: ESP-IDF does not expose the Classic link key bytes via this API, only the addresses.

This is already a hint why “merge the registries” is not a clean operation: the data you can view/manage differs.

## Why you can’t “just combine them”

### 1) Different protocols → different state machines → different events

BLE security events (examples we already handle/log):

- `ESP_GAP_BLE_SEC_REQ_EVT`
- `ESP_GAP_BLE_PASSKEY_REQ_EVT`
- `ESP_GAP_BLE_NC_REQ_EVT`
- `ESP_GAP_BLE_AUTH_CMPL_EVT`

Classic security events are different:

- `ESP_BT_GAP_PIN_REQ_EVT` (legacy PIN)
- `ESP_BT_GAP_CFM_REQ_EVT` (SSP numeric confirm)
- `ESP_BT_GAP_KEY_REQ_EVT` / `ESP_BT_GAP_KEY_NOTIF_EVT`
- `ESP_BT_GAP_AUTH_CMPL_EVT`

If you “combine” registries naively, you’ll be tempted to “reuse” the same prompts/handlers for both — but the protocol steps differ enough that you can easily send the wrong response at the wrong time.

### 2) Different key material (and different semantics)

BLE bonding stores a bundle of keys:

- LTK for encryption (plus EDIV/RAND in some modes),
- IRK for identity resolution (privacy),
- CSRK for data signing (optional),
- ID key (device identity).

Classic bonding stores:

- a Classic link key (type varies: authenticated/unauthed, P-192/P-256, etc.),
- plus Classic pairing metadata and policy (trusted services, etc.).

These are not interchangeable keys.

### 3) Different addressing model

Classic:

- “device identity” is basically BD_ADDR.

BLE:

- a device can show up with:
  - a public address,
  - a static random address,
  - an RPA that rotates frequently.
- the “identity” can be different from “what you saw in the scan”.

So even if the same physical keyboard is dual-mode, you may not be able to map:

```
Classic BD_ADDR  <->  BLE advertising address
```

unless you have the right identity keys and have actually bonded in the LE world.

### 4) CTKD exists, but it’s optional and often blocked

The Bluetooth ecosystem has a concept called Cross-Transport Key Derivation (CTKD) where a dual-mode device can derive keys across transports in some cases.

Practical reality:

- It’s optional (both sides must support the feature and the relevant security level).
- It may be disallowed by policy (key strength / authentication requirements).
- In ESP-IDF BLE auth failures you even see explicit reasons like:
  - `ESP_AUTH_SMP_XTRANS_DERIVE_NOT_ALLOW`
  - `ESP_AUTH_SMP_BR_PARING_IN_PROGR`

So even if Linux presents “one device” in the UI, the underlying bond material may still be transport-specific.

## What “a combined registry” *can* mean (and how to do it safely)

You can build a combined application-level view by defining a single “peer record” that has two optional sub-records.

### Data model (pseudocode)

```text
PeerIdentityKey := one stable identifier chosen by the app
  - simplest: "bt:<bd_addr>" for BR/EDR-only devices
  - for LE: "le:<identity_addr>/<addr_type>" once bonded (or if public addr)

PeerRecord:
  id: PeerIdentityKey
  display_name: string?

  classic:
    bd_addr: 48-bit
    bonded: bool
    last_seen: time?
    pairing_state: enum?

  le:
    identity_addr: 48-bit
    identity_addr_type: public|random
    bonded: bool
    keys_present: {LTK, IRK, CSRK, ...}
    last_seen_adv_addr: 48-bit?
    last_seen_adv_addr_type: public|random?
```

### Safe rule: “unify UX, not storage”

- Provide one console command like `peers` that prints merged records.
- But keep separate commands/state for:
  - `bonds le` (uses `esp_ble_get_bond_device_list`)
  - `bonds bredr` (uses `esp_bt_gap_get_bond_device_list`)
  - `unpair le` vs `unpair bredr` (different APIs).

### Mapping logic (where it gets hard)

If you want “this Classic keyboard and that LE keyboard are the same physical thing”, you need a mapping strategy:

- If the device is Classic-only (your current case), mapping is trivial: only the Classic sub-record exists.
- If the device is LE-only, mapping is LE-only.
- If the device is dual-mode, mapping requires either:
  - CTKD (both sides support it), or
  - out-of-band heuristics (name match, vendor data, manual “link these two”).

There is no universal, always-correct automatic mapping.

## How this should influence our firmware design (dual-mode plan)

If we add Classic HID support (and maybe dual-mode):

1) Treat “security” as two layers:
   - BLE security: LE SMP bonding/encryption events + bond list.
   - Classic security: SSP/PIN events + bond list.
2) Expose both in the console (even if we later add a merged `peers` view).
3) Do not assume CTKD works for a given keyboard.

## Concrete ESP-IDF API pointers (so you can read the stack)

### BLE bond registry API and key structures

- `/home/manuel/esp/esp-idf-5.3.4/components/bt/host/bluedroid/api/include/api/esp_gap_ble_api.h`
  - `esp_ble_get_bond_device_num`
  - `esp_ble_get_bond_device_list`
  - `esp_ble_remove_bond_device`
  - `esp_ble_bond_dev_t` (contains `bond_key` + `bd_addr_type`)

### Classic bond registry API and pairing primitives

- `/home/manuel/esp/esp-idf-5.3.4/components/bt/host/bluedroid/api/include/api/esp_gap_bt_api.h`
  - `esp_bt_gap_get_bond_device_num`
  - `esp_bt_gap_get_bond_device_list`
  - `esp_bt_gap_remove_bond_device`
  - `esp_bt_gap_set_pin` / `esp_bt_gap_pin_reply` (legacy PIN)
  - Classic GAP events: `ESP_BT_GAP_PIN_REQ_EVT`, `ESP_BT_GAP_CFM_REQ_EVT`, `ESP_BT_GAP_AUTH_CMPL_EVT`, etc.

## Appendix: why Linux looks unified

BlueZ (Linux) presents a single “Device” object in many UIs because that’s a convenient user abstraction, but internally it can maintain separate security properties per transport (and it will attempt CTKD in some cases). That’s why “it pairs fine on Linux” doesn’t automatically mean “the LE bond should exist” or “the BR/EDR bond should exist”.

## If you want: what we’d implement next

If we proceed with dual-mode firmware:

- Add `bonds bredr` (Classic) in addition to existing BLE `bonds`.
- Add Classic pairing prompts/handlers.
- Add a `peers` merged view that groups:
  - Classic-only keyboards (your current case),
  - BLE keyboards,
  - dual-mode devices (if we can identify them reliably).
