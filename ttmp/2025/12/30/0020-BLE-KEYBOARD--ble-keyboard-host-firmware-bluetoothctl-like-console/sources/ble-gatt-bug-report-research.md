---
Title: BLE GATT bug report research
Ticket: 0020-BLE-KEYBOARD
Status: active
Topics:
  - ble
  - esp32s3
  - keyboard
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Drop-in research report on GATTC disconnect reason 0x0100 and HIDH open status 0x85.
LastUpdated: 2026-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---

Below is a “drop-in” research report for ticket **0020-BLE-KEYBOARD** (ESP-IDF **v5.4.1**, **Bluedroid**, **BLE HID Host / HOGP**) focused on the two codes you’re seeing:

* **GATTC disconnect reason `0x0100`**
* **BLE_HIDH open failed status `0x85`** (aka “GATT error 133”)

---

## 1) Meaning of the codes (authoritative mapping)

### A) `disconnect reason = 0x0100` (aka `rsn=0x100`)

**What it is:** a *GATT/L2CAP* “disconnect reason” value from the Bluedroid stack.

**Symbol / definition:**

* In Bluedroid’s GATT definitions, `0x0100` is **`GATT_CONN_CANCEL`** with the comment **“L2CAP connection cancelled”**. ([Android Go Source][1])
* ESP-IDF also exposes the same value as **`ESP_GATT_CONN_CONN_CANCEL = 0x0100`** (“L2CAP connection cancelled”). ([SourceVu][2])

**Plain English:** the local stack cancelled the LE connection attempt at the L2CAP/GATT layer. This often shows up when the connection establishment *never fully succeeds* (e.g., peer not reachable / not advertising / controller can’t start the connection / the procedure is aborted), and Bluedroid reports it upward as “cancel”.

---

### B) `BLE_HIDH: OPEN failed: 0x85` (status `0x85` / decimal `133`)

**What it is:** a *GATT status code*.

**Symbol / definition:**

* ESP-IDF’s GATT status list defines **`0x85` as `ESP_GATT_ERROR`**, described as a **generic error** (corresponding to **`BTA_GATT_ERROR`**). ([Espressif Systems][3])

**Plain English:** “generic GATT failure”. It’s *not HID-specific*—HID host is just reporting that its underlying GATT open / discovery path failed and the stack only provided the generic status.

**Important contextual clue:** `0x85` is *very commonly observed* when trying to **open/connect while scanning is still running** in Bluedroid; Espressif has an issue where `ESP_GATTC_OPEN_EVT` returns `status==133 (0x85)` if scanning isn’t stopped first. ([GitHub][4])

---

## 2) Most likely root-cause hypotheses (ranked)

### #1 — You’re connecting to the *wrong address* (privacy / RPA / directed advertising / not in pairing mode)

This best matches your explicit note: **“target address not present in scan results”** + direct connect fails.

**Why it fits:**

* Many BLE HID keyboards **do not continuously advertise** their “identity address” for new centrals. They may:

  * advertise only briefly after a keypress / when in pairing mode, or
  * use **LE Privacy** (rotating **Resolvable Private Addresses**, RPAs), or
  * use **directed advertising** to a previously bonded host.
* With LE Privacy, the device advertises using an RPA derived from an IRK; bonded peers can “resolve” it, but **the advertising address you must connect to is the *current* RPA**, not a stale identity value. The Bluetooth Core spec describes generating/resolving RPAs and the central’s privacy behavior. ([Bluetooth® Technology Website][5])
* Your provided address `DC:2C:26:FD:03:B7` starts with `0xDC` (binary `11xxxxxx`), which is consistent with a **random static identity address** (commonly used as the “identity” when privacy is enabled). A concise statement of the MSB patterns and identity/RPA relationship is given in NXP’s BLE privacy note (identity can be random static/public; RPA uses different MSB pattern). ([NXP Semiconductors][6])

**How this produces your exact symptoms:**

* If the keyboard is not advertising as `DC:…` right now (because it’s using an RPA or not advertising at all), a connection attempt can’t be completed → Bluedroid surfaces **`GATT_CONN_CANCEL (0x0100)`** and the HID layer reports **`ESP_GATT_ERROR (0x85)`**.

---

### #2 — You’re attempting the GATT open while scanning (Bluedroid limitation / race)

This is a classic Bluedroid behavior on ESP32: **open fails with 0x85 while scanning**.

**Evidence:**

* Espressif issue: `esp_ble_gattc_open` while scanning → `ESP_GATTC_OPEN_EVT open.status == 133 (0x85)`; workaround is stop scan before open and resume after. ([GitHub][4])

If your console flow allows `connect` while a scan is still active (or before `SCAN_STOP_COMPLETE`), this can be the entire explanation even without privacy complications.

---

### #3 — Controller/stack cannot start a new LE connection (state/resource issue)

Lower probability, but it’s a known failure mode where Bluedroid can get into a “can’t start connection” situation; the stack logs show L2CAP complaints and then `rsn=0x100`.

**Evidence pattern in the wild:**

* Logs in real deployments show sequences like L2CAP “cannot start new connection …” followed by `gattc_conn_cb … rsn=0x100`. ([GitHub][7])

This tends to happen with rapid connect/disconnect loops, too many concurrent connection attempts, or a stuck “disconnecting” state.

---

### #4 — Security/pairing requirements (bonding/whitelist) block the connection

Some HID peripherals require pairing/bonding and may reject unknown centrals. This can *also* manifest as “not discoverable unless pairing mode” (which collapses back into #1). It’s plausible, but the key discriminator is whether the device is actually advertising to you at all.

---

## 3) Concrete mitigation options (what to try next)

### Option A — **Only connect using a scan result** (address + address type from the scan callback)

**Idea:** stop trying to connect to a “known” MAC that you didn’t just see on-air. Instead:

1. scan (prefer **active scan**)
2. pick the device based on **name / appearance / service UUID 0x1812 / manufacturer data**
3. connect using **the exact `bda` + `addr_type` reported in that scan result**.

**Pros**

* Solves **RPA/rotating address** and “not currently advertising” problems.
* Prevents stale-address connects that inevitably fail.

**Cons**

* Requires your CLI to support “connect last-seen <n>” or “connect by name”.

**Why this is likely the “real fix”:** your own note says the target address wasn’t present in scan results. If it’s not on-air, you can’t connect to it.

---

### Option B — **Enforce “stop scan → wait for stop complete → open/connect”**

Given the known Bluedroid behavior, make `connect` do:

* `esp_ble_gap_stop_scanning()`
* wait for `ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT`
* then `esp_hidh_dev_open(...)` / `esp_ble_gattc_open(...)`

This is exactly the workaround described by others seeing `0x85` while scanning. ([GitHub][4])

**Pros**

* Minimal code/config change.
* Directly targets the most common trigger of `0x85`.

**Cons**

* Doesn’t solve “keyboard not advertising / RPA address changed” by itself; it just removes one failure mode.

Minimal sketch (illustrative):

```c
// Pseudocode for your CLI "connect":
esp_ble_gap_stop_scanning();
// wait for ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT
// then open/connect using addr+addr_type (ideally from scan cache)
esp_hidh_dev_open(bda, addr_type /* from scan */, /* ... */);
```

---

### Option C — **Put the keyboard into pairing/discoverable mode and verify you can see it**

Operational mitigation that often matters for BLE keyboards:

* ensure the keyboard is in *BLE pairing mode* (vendor-specific gesture)
* press a key to “wake” advertising
* confirm it shows up in your scan list *right then*

**Pros**

* Fast reality check: if you can’t see it advertising, no firmware tweak will make direct connect work.

**Cons**

* Manual step; not a “code fix”, but often the missing piece.

---

### Option D — **If LE Privacy is involved, treat `DC:…` as an identity and connect to the current RPA**

If you confirm the keyboard uses RPA:

* Expect the advertised address to rotate; you must connect to the current advertised address.
* If you want “recognize it as the same device across rotations”, you need bonding and the IRK-based resolving flow described in the Core spec. ([Bluetooth® Technology Website][5])

**Pros**

* Correct long-term architecture for modern peripherals.

**Cons**

* More moving pieces (bonding database, resolving list behavior, privacy mode).

---

## 4) A tight “next experiment” checklist (to collapse ambiguity fast)

1. **During scan output, print address type + RSSI + name**, and mark anything that looks like HID (UUID 0x1812 / appearance keyboard).
2. If you ever see the keyboard by name, attempt connect using that scan entry immediately:

   * if it works → stale address / privacy / “not advertising unless awake” was the cause (#1).
3. If connect still fails but only when scanning was running → you just validated the scan/connect sequencing issue (#2). ([GitHub][4])
4. If you see repeated `rsn=0x100` after rapid attempts, add a backoff and ensure prior connections are fully closed (guard against #3). ([GitHub][7])

---

## TL;DR mapping (for the ticket header)

* **`0x100`** = `ESP_GATT_CONN_CONN_CANCEL` / `GATT_CONN_CANCEL` = **L2CAP connection cancelled**. ([Android Go Source][1])
* **`0x85`** = `ESP_GATT_ERROR` (generic GATT error / “133”). ([Espressif Systems][3])
* Most likely: you’re connecting to an address the keyboard is **not currently advertising**, often due to **pairing mode / privacy RPA**, *and/or* you’re connecting **while scanning** (known to yield `0x85`). ([Bluetooth® Technology Website][5])

If you want, paste a ~30–50 line log that includes: “scan start/stop”, any “HCI disc complete reason”, and the exact moment you call `connect`, and I’ll pin it down to #1 vs #2 vs #3 with high confidence.

[1]: https://android.googlesource.com/platform/external/bluetooth/bluedroid/%2B/master/stack/include/gatt_api.h?utm_source=chatgpt.com "stack/include/gatt_api.h - platform/external/bluetooth ..."
[2]: https://sourcevu.sysprogs.com/espressif/esp-idf/symbols/esp_gatt_conn_reason_t/ESP_GATT_CONN_CONN_CANCEL?utm_source=chatgpt.com "ESP_GATT_CONN_CONN_CAN..."
[3]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/esp_gatt_defs.html?utm_source=chatgpt.com "GATT Defines - ESP32 - — ESP-IDF Programming Guide ..."
[4]: https://github.com/espressif/esp-idf/issues/4562 "esp_ble_gattc_open failed when scanning (IDFGH-2449) · Issue #4562 · espressif/esp-idf · GitHub"
[5]: https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-54/out/en/host/generic-access-profile.html?utm_source=chatgpt.com "Part C Generic Access Profile"
[6]: https://www.nxp.com/docs/en/application-note/AN12928.pdf?utm_source=chatgpt.com "AN12928 Bluetooth Low Energy Privacy and NXP QN9090"
[7]: https://github.com/esphome/issues/issues/6701?utm_source=chatgpt.com "BLE client \"DISCONNECTING\" state is terminal and leaks ..."
