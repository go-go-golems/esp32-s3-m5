Your log is actually *very* diagnostic: the plug is **joining**, but it is **never completing authorization / key establishment**, so it keeps falling back into “unsecured join → timeout → try again”, and each new attempt gets a new short address.

## What your lines mean (decoded from your own headers)

In your tree, `components/zb_host/include/zdo/esp_zigbee_zdo_common.h` documents these fields:

### `Device update (status=0x01 …)`

`status=0x01` = **Standard device unsecured join**
So the device is getting on the network at the MAC/NWK level, but not secured yet.

### `Device authorized (type=0x01 status=0x01 …)`

`type=0x01` = **r21 device authorization through TCLK**
For this type, `status` is:

* `0x00` = Authorization success
* `0x01` = **Authorization timeout**
* `0x02` = Authorization failed

You are getting **status=0x01 every time** → **authorization timeout**. That’s why you see the same IEEE (`28:2c:…:70`) repeatedly “join/rejoin” and churn through short addresses (`0x9294 → 0xcc92 → 0xae79 …`).

## The two most likely root causes

### 1) Trust Center Link Key mismatch (very common)

If your coordinator/NCP is **not using the expected TCLK** (many Zigbee 3.0 devices expect the well-known default `ZigBeeAlliance09` unless you’re doing install-code joining), the device won’t be able to decrypt the transported keys, and the coordinator will eventually time out waiting for the security procedure to complete.

**Quick test with your existing console:** your private ZNSP header defines:

* `ESP_ZNSP_NETWORK_LINK_KEY_GET = 0x0027`
* `ESP_ZNSP_NETWORK_LINK_KEY_SET = 0x0028`

So try:

* Read current link key:

  ```
  znsp req 0x0027
  ```

  (Expect 16 bytes back, or a status + 16 bytes depending on the NCP implementation.)

* Set the default Zigbee Alliance key (`"ZigBeeAlliance09"` ASCII hex = `5a 69 67 42 65 65 41 6c 6c 69 61 6e 63 65 30 39`)

  ```
  znsp req 0x0028 5a 69 67 42 65 65 41 6c 6c 69 61 6e 63 65 30 39
  ```

Then **factory reset the plug** and re-join.

**Success criterion in logs:** you want to see `Device authorized … status=0x00` at least once, and after that you should stop seeing repeated “unsecured join” churn.

---

### 2) RF/channel trouble causing key exchange packets to get lost (also fits your repo notes)

Authorization is a multi-message exchange. If you’re forming on a bad channel (your own report mentions “formation stuck on ch13”), you can absolutely get “timeout” rather than “failed”.

The fix path is what you already started documenting:

* **Erase Zigbee persistent storage on the H2** (`zb_storage` / `zb_fct`) so you aren’t reusing an old formed network/channel/key material.
* Ensure your **channel mask is applied before formation** (and confirm the formed channel is what you asked for).
* Put the plug **very close** to the coordinator for the first secure join (to eliminate marginal LQI as a variable).

**Success criterion:** once the device authorizes successfully once, it should remain stable (no new short addresses every ~10–30s).

## What I would do next (fast, decisive)

1. **Set/verify the Trust Center Link Key** via `znsp req 0x0027/0x0028` as above.
2. **Hard reset plug**, open join, try again.
3. If you *still* only see `authorization timeout`:

   * erase the H2 Zigbee storage partitions and re-form on a cleaner channel (20/25 are common choices depending on your Wi-Fi environment),
   * retry joining with the plug right next to the coordinator.

If you paste the `znsp req 0x0027` response bytes (and tell me whether your NCP returns a leading status byte or raw key), I can tell you immediately whether your coordinator is currently using the expected TCLK and how to interpret that response format.


---

I dug through your repo + the diary you zipped, and the log you pasted actually contains the smoking gun: **the plug is joining, but it is *not getting authorized*** — so it immediately cycles and joins again, which is why you see the **same IEEE address with constantly changing short addresses**.

## What your log is really saying

You have the same device IEEE each time:

`ieee=28:2c:02:bf:ff:e6:98:70`

…but it gets **three different short addresses** in quick succession:

* `0x9294`
* `0xcc92`
* `0xae79`

That’s classic **join/rejoin loop** behavior.

Now the important part is the status codes:

### 1) `Device update (status=0x01 …)`

In ESP Zigbee SDK terms, `status=0x01` is:

> **Standard device unsecured join** (fresh join, not a secured rejoin). ([Espressif Systems][1])

So the plug is repeatedly doing a *fresh unsecured join*.

### 2) `Device authorized (type=0x01 status=0x01 …)`

Espressif documents this field explicitly:

* `authorization_type = 0x01` = “r21 device through TCLK”
* for that type, `authorization_status = 0x01` = **Authorization timeout** ([Espressif Systems][1])

So your log line:

`Device authorized (type=0x01 status=0x01 …)`

means:

✅ coordinator *started* the Trust Center link key (TCLK) authorization flow
❌ but it **timed out** (key exchange didn’t complete)

That is enough to explain the instability: the device never reaches a clean “authorized and stable” state, so it tries again.

---

## Why this happens (most likely)

There are two common causes for **TCLK authorization timeout**:

1. **RF reliability issue during the key exchange**
   If packets are getting dropped right after join (bad RSSI, interference), the multi-message TCLK/key exchange is a prime place to timeout.

   Your separate “stuck on Zigbee channel 13” situation matters here because ch13 overlaps heavily with 2.4GHz Wi‑Fi (often Wi‑Fi ch1). If you’re sitting on a noisy channel, key exchange failures are *much* more likely.

2. **Security/key configuration mismatch**
   Less common than RF, but if the trust-center preconfigured key / join key expectations don’t match what the plug expects, you can also end up with timeouts/failures.

---

## Fastest way to confirm it’s the TCLK/key-exchange step

Espressif provides a knob specifically for this:

`esp_zb_secur_link_key_exchange_required_set(false)`

Docs note (this is basically “let it stay even if TC-link-key exchange doesn’t happen”):

> “The ZigBee coordinator with `disabled` allows the end device to stay in network without a TC-link key exchange.” ([Espressif Systems][2])

### What to do

Temporarily disable the requirement and try pairing again.

**Expected result if TCLK exchange is your blocker:**
The plug should stop “re-joining” and the short address should stabilize.

### Where to put it in your project

In your host code, `gw_zb.c` inside `zb_task()`, put it **after** `esp_zb_init(&zb_cfg);` and **before** `esp_zb_start(true);`.

Pseudo-patch (minimal):

```c
#include "esp_zigbee_secur.h"

// inside zb_task(), after esp_zb_init(&zb_cfg); and before esp_zb_start(...)

esp_zb_secur_link_key_exchange_required_set(false);  // DEBUG: allow device to stay without TCLK exchange
```

If that stabilizes the device: you’ve proven the loop is **authorization/TCLK exchange failing**, not your registry, not your on/off command path.

*(You can then re-enable it once you fix RF/channel/security.)*

---

## Fix the channel problem (because it likely causes the timeout)

Your diary already flagged “channel selection stuck on 13”. That’s very consistent with a known pattern: **the stack restores an existing network (and its channel) from persistent storage**, so “setting the mask” doesn’t move an already-formed network.

Espressif explicitly calls out:

* Channel masks must be set **after `esp_zb_init()` and before `esp_zb_start()`** ([Espressif Systems][3])
* The stack loads parameters from NVRAM during startup ([Espressif Systems][3])
* You can force a clean slate by erasing Zigbee storage:

  * `esp_zb_nvram_erase_at_start(true)` erases `zb_storage` at startup ([Espressif Systems][3])
  * `esp_zb_factory_reset()` “completely erase[s] the `zb_storage` partition and then restart[s]” ([Espressif Systems][3])

### Practical recommendation

To **force the coordinator to actually form a brand-new network** using your requested channel mask:

Option A (debug, heavy hammer): erase every boot

```c
esp_zb_nvram_erase_at_start(true);
```

([Espressif Systems][3])

Option B (cleaner): do a one-time factory reset path

```c
esp_zb_factory_reset();  // wipes zb_storage and restarts
```

([Espressif Systems][3])

Then your existing `esp_zb_set_channel_mask(...)` / `esp_zb_set_primary_network_channel_set(...)` calls have a fair shot at taking effect during *formation*.

Also: there’s a very similar symptom reported publicly (“sets mask to channel 11 but coordinator still runs on channel 13”). ([GitHub][4])
Even if your root cause is NVRAM restore, it’s worth knowing you’re not imagining it.

---

## What “success” should look like in your logs

Right now you have:

* `Device update status=0x01` (unsecured join) ([Espressif Systems][1])
* `Device authorized type=0x01 status=0x01` (timeout) ([Espressif Systems][1])
* repeat…

After you fix it, you should see **authorization success**:

* `Device authorized type=0x01 status=0x00` (success) ([Espressif Systems][1])

…and the short address should stop changing constantly.

---

## If you want a “secure final” setup (after you stabilize)

Once you’ve proven stability, you can re-enable link-key exchange and make sure the trust center uses the expected join key. Espressif exposes:

* `esp_zb_secur_TC_standard_preconfigure_key_set(...)` ([Espressif Systems][2])

That’s the right place to ensure the coordinator is using the standard preconfigured Trust Center key when needed.

---

## My recommended order of operations

1. **Confirm diagnosis quickly**

   * Add `esp_zb_secur_link_key_exchange_required_set(false)`
   * Pair the plug
   * If it stabilizes → you’ve confirmed it’s the TCLK authorization step

2. **Fix channel selection**

   * Wipe `zb_storage` via `esp_zb_nvram_erase_at_start(true)` (debug) or `esp_zb_factory_reset()` (one-time)
   * Form network on a “quieter” channel (20/25 are common choices)
   * Try pairing again **with link key exchange re-enabled**

3. **Lock in the secure solution**

   * Re-enable link-key exchange requirement
   * If needed, set the TC preconfigured key via `esp_zb_secur_TC_standard_preconfigure_key_set(...)`

If you paste one full join attempt *including any leave indications* (you already added those signals in your NCP logging), I can tell you whether the device is explicitly leaving/rejoining or whether the trust-center is dropping it after timeout.

[1]: https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32/api-reference/zdo/esp_zigbee_zdo_common.html "ZDO Common - ESP32 -  — ESP Zigbee SDK latest documentation"
[2]: https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32/api-reference/esp_zigbee_secur.html "Security API - ESP32 -  — ESP Zigbee SDK latest documentation"
[3]: https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32/api-reference/esp_zigbee_core.html "Core API - ESP32 -  — ESP Zigbee SDK latest documentation"
[4]: https://github.com/espressif/esp-zigbee-sdk/issues/435 "coordinator ignores *_network_channel_set (TZ-1155) · Issue #435 · espressif/esp-zigbee-sdk · GitHub"




