Below is what I’d put in the ticket as the “research result / recommended fix” for ESP-IDF **5.4.1** + **Bluedroid** on **ESP32-S3**.

---

## Why you get the link error

Your analysis is consistent with how ESP-IDF gates Bluedroid features:

* In your `sdkconfig`, **BLE 5.0 features are enabled** and **BLE 4.2 features are disabled**.
* ESP-IDF’s Kconfig explicitly states **BLE 4.2 and BLE 5.0 cannot be used simultaneously**. ([Espressif Systems][1])
* The legacy (“classic helper”) GAP advertising APIs you’re calling (`esp_ble_gap_config_adv_data()`, `esp_ble_gap_start_advertising()`) are part of the “classic” GAP workflow, and (in your tree) they’re compiled out when `BLE_42_FEATURE_SUPPORT` is false → the linker can’t find the symbols.

So the fix is *not* a CMake “missing REQUIRES bt” problem; it’s a **feature gating/config selection** problem.

---

## 1) Correct Kconfig symbols + exact settings (copy/paste)

### Option A (recommended for your use-case): use BLE 4.2 features + legacy advertising APIs

For “simple GATT server + advertising + notify”, BLE 4.2 is usually the cleanest path because it keeps the legacy advertising APIs available and avoids extended-adv complexity.

**Set these in `sdkconfig.defaults`:**

```ini
# Bluedroid + BLE
CONFIG_BT_BLUEDROID_ENABLED=y
CONFIG_BT_BLE_ENABLED=y

# Use BLE 4.2 feature set (legacy advertising APIs)
CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y
CONFIG_BT_BLE_42_ADV_EN=y

# Disable BLE 5.0 feature set (mutually exclusive with BLE 4.2)
CONFIG_BT_BLE_50_FEATURES_SUPPORTED=n
```

Notes:

* `CONFIG_BT_BLE_42_FEATURES_SUPPORTED` is the top-level toggle for the BLE 4.2 feature set in Bluedroid Options. ([Espressif Systems][1])
* `CONFIG_BT_BLE_42_ADV_EN` is the specific advertising enable under that feature set. ([Espressif Systems][1])
* **Do not** set both 4.2 and 5.0: Espressif’s Kconfig reference says they cannot be used simultaneously. ([Espressif Systems][1])

### Option B: keep BLE 5.0 features (extended advertising path)

If you *need* BLE 5 features, then keep:

```ini
CONFIG_BT_BLE_50_FEATURES_SUPPORTED=y
CONFIG_BT_BLE_42_FEATURES_SUPPORTED=n
```

(again: mutually exclusive ([Espressif Systems][1]))

…and switch your code to the **extended advertising APIs** (next section).

---

## 2) If you keep BLE 5.0: which APIs replace the legacy helpers?

On ESP32-S3, Bluedroid exposes extended advertising APIs including:

* `esp_ble_gap_ext_adv_set_params()`
* `esp_ble_gap_config_ext_adv_data_raw()`
* `esp_ble_gap_config_ext_scan_rsp_data_raw()`
* `esp_ble_gap_ext_adv_start()`
* `esp_ble_gap_ext_adv_stop()`

You can see these names enumerated in the ESP32-S3 Programming Guide index. ([Espressif Systems][2])
The GAP callback also has extended-adv lifecycle events such as:
`ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT`, `ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT`, `ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT`, `ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT`, etc. ([Espressif Systems][3])

### Minimal bring-up flow (Bluedroid extended advertising)

**High-level state machine**

1. Register GAP callback.
2. Call `esp_ble_gap_ext_adv_set_params(instance, &params)`.
3. On `ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT` → push adv data:

   * `esp_ble_gap_config_ext_adv_data_raw(instance, len, data)`
   * optionally `esp_ble_gap_config_ext_scan_rsp_data_raw(instance, len, scan_rsp)`
4. When you receive “data set complete” (and scan-rsp complete if used) → call:

   * `esp_ble_gap_ext_adv_start(num_adv, ext_adv_array)`
5. Observe `ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT` for success/failure. ([Espressif Systems][3])

**Code skeleton (the part you paste into your GAP callback)**

```c
// Pseudocode-ish skeleton: the EVENT NAMES are authoritative (from ESP-IDF docs),
// and the FUNCTION names are the extended-adv replacements.
// Fill params/data formats per esp_gap_ble_api.h for your exact IDF version.

static bool ext_adv_data_ready = false;
static bool ext_scan_rsp_ready = false;

static void gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
        // 1) set adv payload
        esp_ble_gap_config_ext_adv_data_raw(ADV_INSTANCE, sizeof(ext_adv_raw), ext_adv_raw);
        // 2) optionally set scan response payload
        esp_ble_gap_config_ext_scan_rsp_data_raw(ADV_INSTANCE, sizeof(ext_scan_rsp_raw), ext_scan_rsp_raw);
        break;

    case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
        ext_adv_data_ready = true;
        if (ext_adv_data_ready && ext_scan_rsp_ready) {
            esp_ble_gap_ext_adv_start(1, ext_adv);  // ext_adv is an array of esp_ble_gap_ext_adv_t
        }
        break;

    case ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        ext_scan_rsp_ready = true;
        if (ext_adv_data_ready && ext_scan_rsp_ready) {
            esp_ble_gap_ext_adv_start(1, ext_adv);
        }
        break;

    case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
        // check param->ext_adv_start_cmpl.status
        break;

    default:
        break;
    }
}
```

If you want this to behave like “classic” advertising (discoverable in typical phone scanners), the extended-adv params need to be set accordingly. The ESP-IDF “Device Discovery” guide explains what extended advertising is and how it differs from legacy advertising at the packet level. ([Espressif Systems][4])

---

## 3) Cleanest way to apply config changes in an existing project

Key point: `sdkconfig.defaults` does **not** retroactively overwrite an existing `sdkconfig`; it’s used when generating a new config.

The most deterministic workflows:

### Workflow A (simple): regenerate `sdkconfig`

```bash
rm -f sdkconfig sdkconfig.old
idf.py reconfigure
```

### Workflow B (what you’re already doing): `set-target` regenerates config

Espressif docs: `idf.py set-target <target>` **clears the build dir and re-generates `sdkconfig` from scratch**, saving the previous one as `sdkconfig.old`. ([Espressif Systems][5])
So after editing `sdkconfig.defaults`, run:

```bash
idf.py set-target esp32s3
idf.py build
```

### Note on `fullclean`

`idf.py fullclean` deletes the build directory **but does not delete project configuration** (i.e., doesn’t remove `sdkconfig`). ([Espressif Systems][5])

---

## 4) Confirm expectations about symbol availability

**You should expect `esp_ble_gap_config_adv_data()` and `esp_ble_gap_start_advertising()` to be linkable when:**

* Bluedroid BLE is enabled, **and**
* the BLE 4.2 feature set is enabled (`CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y`), **and**
* BLE 4.2 advertising is enabled (`CONFIG_BT_BLE_42_ADV_EN=y`). ([Espressif Systems][1])

If `CONFIG_BT_BLE_50_FEATURES_SUPPORTED=y` and BLE 4.2 features are disabled, you should plan to use the **extended advertising** APIs (`esp_ble_gap_ext_adv_*`, `esp_ble_gap_config_ext_*`). ([Espressif Systems][2])

(As a sanity check from Espressif docs: their BLE FAQ uses `esp_ble_gap_start_advertising()` as the canonical “advertise again after connect” action in Bluedroid examples. ([Espressif Systems][6]))

---

## Ticket-ready “Recommended Path”

### ✅ Recommended Path (Option A): enable BLE 4.2 features, disable BLE 5.0 features

* Pros: keeps your current legacy advertising code unchanged; fastest bring-up.
* Apply `sdkconfig.defaults` settings from Option A, regenerate config (`set-target` or delete `sdkconfig` + `reconfigure`), rebuild.

### Alternate (Option B): keep BLE 5.0 features, switch to extended advertising APIs

* Use `esp_ble_gap_ext_adv_set_params()` → `esp_ble_gap_config_ext_adv_data_raw()` → `esp_ble_gap_ext_adv_start()` with the extended GAP events shown above. ([Espressif Systems][2])

If you paste your current `ble_temp_logger_main.c` advertising section here, I can rewrite it into either (A) a minimal legacy-adv block that matches your existing structure, or (B) an extended-adv block with the right event gating and data setup.

[1]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/kconfig-reference.html "Configuration Options Reference - ESP32-S3 -  — ESP-IDF Programming Guide v5.5.2 documentation"
[2]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/genindex.html?utm_source=chatgpt.com "Index - ESP32-S3 - — ESP-IDF Programming Guide v5.5.1 ..."
[3]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/bluetooth/esp_gap_ble.html "GAP API - ESP32-S3 -  — ESP-IDF Programming Guide v5.5.2 documentation"
[4]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/ble/get-started/ble-device-discovery.html "Device Discovery - ESP32-S3 -  — ESP-IDF Programming Guide v5.5.2 documentation"
[5]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-py.html "IDF Frontend - idf.py - ESP32 -  — ESP-IDF Programming Guide v5.5.2 documentation"
[6]: https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/bt/ble.html?utm_source=chatgpt.com "Bluetooth LE & Bluetooth - - — ESP-FAQ latest documentation"
