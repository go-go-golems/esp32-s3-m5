---
Title: "BLE 4.2 vs BLE 5.0 in ESP-IDF 5.4.1 (Bluedroid): Advertising APIs + How-To Guides"
Ticket: 0019-BLE-TEST
Status: active
Topics:
    - ble
    - esp-idf
    - bluedroid
    - esp32s3
    - cardputer
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/Kconfig.in
      Note: Defines mutual exclusivity between `BT_BLE_50_FEATURES_SUPPORTED` and `BT_BLE_42_FEATURES_SUPPORTED`.
    - Path: /home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/common/include/common/bt_target.h
      Note: Defines `BLE_42_FEATURE_SUPPORT` / `BLE_50_FEATURE_SUPPORT` feature gating used by Bluedroid APIs.
    - Path: /home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/api/include/api/esp_gap_ble_api.h
      Note: Declares legacy advertising APIs under `BLE_42_FEATURE_SUPPORT` and extended advertising APIs under `BLE_50_FEATURE_SUPPORT`.
    - Path: /home/manuel/esp/esp-idf-5.4.1/examples/system/ota/advanced_https_ota/main/ble_helper/bluedroid_gatts.c
      Note: Uses legacy advertising (`esp_ble_gap_config_adv_data` + `esp_ble_gap_start_advertising`) and documents the GAP event flow.
    - Path: /home/manuel/esp/esp-idf-5.4.1/examples/bluetooth/bluedroid/ble_50/periodic_adv/main/periodic_adv_demo.c
      Note: Uses BLE 5.0 extended advertising (`esp_ble_gap_ext_adv_*`) with a callback-driven sequencing pattern.
    - Path: /home/manuel/esp/esp-idf-5.4.1/examples/bluetooth/bluedroid/ble_50/ble50_throughput/throughput_server/main/example_ble_server_throughput.c
      Note: Uses extended advertising in a GATT server context (closest to this ticket’s use case).
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c
      Note: Ticket firmware currently uses legacy advertising helpers; links fail when BLE 5.0 is enabled and BLE 4.2 is disabled.
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0019-BLE-TEST--ble-temperature-sensor-logger-on-cardputer/sources/research-ble-advertising-error.md
      Note: Intern research; cross-validated here with ESP-IDF 5.4.1 source and examples.
ExternalSources: []
Summary: Explains how ESP-IDF 5.4.1 Bluedroid gates advertising APIs behind BLE 4.2 vs BLE 5.0 feature sets; provides practical “how-to” guides for both legacy and extended advertising flows, grounded in 5.4.1 source and examples.
LastUpdated: 2025-12-31T00:00:00Z
WhatFor: "Unblock BLE advertising bring-up and choose between legacy vs extended advertising in this repo."
WhenToUse: "When implementing BLE advertising (GATT server) on ESP32-S3 using ESP-IDF 5.4.1 Bluedroid."
---

# BLE 4.2 vs BLE 5.0 in ESP-IDF 5.4.1 (Bluedroid): Advertising APIs + How-To Guides

## Executive summary

ESP-IDF **5.4.1** (Bluedroid) forces you to pick a **BLE 4.2 feature set** *or* a **BLE 5.0 feature set**. This choice directly gates which advertising APIs exist in the compiled Bluedroid library:

- **BLE 4.2 feature set enabled** ⇒ you can use the legacy “helper” advertising APIs:
  - `esp_ble_gap_config_adv_data()`
  - `esp_ble_gap_start_advertising()`
- **BLE 5.0 feature set enabled** ⇒ those legacy helper APIs are compiled out; you must use **extended advertising** APIs:
  - `esp_ble_gap_ext_adv_set_params()`
  - `esp_ble_gap_config_ext_adv_data_raw()`
  - `esp_ble_gap_ext_adv_start()`

For this ticket (“simple GATT server + read + notify”), **both approaches are viable**:

- Choose **BLE 4.2** if you want the simplest bring-up and to reuse older Bluedroid example patterns.
- Choose **BLE 5.0** if you want to align with ESP-IDF 5.4.1 defaults and use the modern extended advertising stack.

## Source of truth (ESP-IDF 5.4.1)

### Mutual exclusivity in Kconfig

ESP-IDF 5.4.1 explicitly says BLE 4.2 and BLE 5.0 cannot be enabled simultaneously in Bluedroid:

- `/home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/Kconfig.in`
  - `BT_BLE_50_FEATURES_SUPPORTED`: default `y`
  - `BT_BLE_42_FEATURES_SUPPORTED`: default `n`
  - both have help text: “cannot be used simultaneously”

### Feature gating of APIs

The macro gating ultimately comes from:

- `/home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/common/include/common/bt_target.h`

Key logic:
- `BLE_50_FEATURE_SUPPORT` is true when `UC_BT_BLE_50_FEATURES_SUPPORTED` is true.
- `BLE_42_FEATURE_SUPPORT` is true when either:
  - `UC_BT_BLE_42_FEATURES_SUPPORTED` is true, **or**
  - `BLE_50_FEATURE_SUPPORT` is false.

Practical takeaway:
- If BLE 5.0 is enabled and BLE 4.2 is disabled, then **`BLE_42_FEATURE_SUPPORT == FALSE`**, and all legacy advertising APIs/types guarded by that macro disappear from the build.

### Where the APIs live

- Legacy advertising (BLE 4.2) declarations / event union fields are guarded by `#if (BLE_42_FEATURE_SUPPORT == TRUE)` in:
  - `/home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/api/include/api/esp_gap_ble_api.h`
- Extended advertising (BLE 5.0) declarations are guarded by `#if (BLE_50_FEATURE_SUPPORT == TRUE)` in the same header.

## Cross-validation vs intern research (`sources/research-ble-advertising-error.md`)

### Correct / confirmed

- **Root cause diagnosis is correct**: the undefined references are a *feature gating / config selection* problem, not a missing CMake dependency.
- **Mutual exclusivity is correct**: Bluedroid Kconfig says BLE 4.2 and BLE 5.0 cannot be enabled simultaneously (ESP-IDF 5.4.1 source confirms this).
- **Extended advertising function names + event names are correct** (the list of `esp_ble_gap_ext_adv_*` and `ESP_GAP_BLE_EXT_ADV_*` events matches ESP-IDF 5.4.1 headers and the bundled examples).
- **Config-application advice is directionally correct**: `sdkconfig.defaults` does not overwrite an existing `sdkconfig`.

### Corrections / updates needed

1) **`CONFIG_BT_BLE_42_ADV_EN` does not exist in ESP-IDF 5.4.1 Bluedroid Kconfig**

- The intern doc suggests setting `CONFIG_BT_BLE_42_ADV_EN=y`.
- In the ESP-IDF 5.4.1 tree, there is **no** `BT_BLE_42_ADV_EN` symbol under Bluedroid’s Kconfig.
- For “legacy helper advertising APIs available”, the relevant knob is:
  - `CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y` **and**
  - `CONFIG_BT_BLE_50_FEATURES_SUPPORTED=n`

2) **Event parameter field names in the skeleton are slightly off**

- In ESP-IDF 5.4.1, the union fields in `esp_ble_gap_cb_param_t` are named like:
  - `param->ext_adv_set_params.status`
  - `param->ext_adv_data_set.status`
  - `param->ext_adv_start.status`
  - (see `periodic_adv_demo.c` and `example_ble_server_throughput.c`)

3) **Some example `sdkconfig.defaults` lines use `CONFIG_BTDM_CTRL_MODE_BLE_ONLY` which can warn on ESP32-S3**

- In the ESP-IDF 5.4.1 tree itself, Espressif CI even has an ignore rule for:
  - `warning: unknown kconfig symbol 'BTDM_CTRL_MODE_BLE_ONLY' ...`
- Reason: for ESP32-S3, `components/bt/controller/esp32s3/Kconfig.in` includes the ESP32-C3 controller Kconfig, and the classic `BTDM_CTRL_MODE_BLE_ONLY` symbol is not always defined for this target.
- Recommendation for this repo: prefer:
  - `CONFIG_BT_CLASSIC_ENABLED=n` (project-level)
  - plus `esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)` (runtime)
  - and avoid relying on `CONFIG_BTDM_CTRL_MODE_BLE_ONLY` for ESP32-S3 unless verified in your exact Kconfig set.

4) **Doc version drift**

- The intern doc links “stable” docs (currently 5.5.x). For this ticket we should treat the **ESP-IDF 5.4.1 source + in-tree examples** as authoritative.

## Option A — BLE 4.2 feature set + legacy advertising APIs (how-to)

### When to choose this

- You want the simplest / most familiar Bluedroid advertising flow.
- You don’t need BLE 5.0 features like extended advertising / periodic advertising / some PHY features.

### Config recipe (project-level)

In `sdkconfig.defaults`:

```ini
# Bluedroid + BLE enabled
CONFIG_BT_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=y
CONFIG_BT_BLE_ENABLED=y

# Choose BLE 4.2 feature set (mutually exclusive with BLE 5.0)
CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y
CONFIG_BT_BLE_50_FEATURES_SUPPORTED=n

# Optional: disable classic BT at a higher level if present
CONFIG_BT_CLASSIC_ENABLED=n
```

Then regenerate config (pick one deterministic workflow):

- Delete the existing config and reconfigure:

```bash
rm -f sdkconfig sdkconfig.old && idf.py reconfigure
```

or run `idf.py set-target esp32s3` (which re-generates config for a target).

### API flow (what you implement)

Core calls:
- `esp_ble_gap_config_adv_data(&adv_data)`
- `esp_ble_gap_config_adv_data(&scan_rsp_data)` (optional)
- `esp_ble_gap_start_advertising(&adv_params)`

Core GAP events to sequence on:
- `ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT`
- `ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT`
- `ESP_GAP_BLE_ADV_START_COMPLETE_EVT`
- `ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT`

Reference example (legacy adv + GATTS):
- `/home/manuel/esp/esp-idf-5.4.1/examples/system/ota/advanced_https_ota/main/ble_helper/bluedroid_gatts.c`

## Option B — BLE 5.0 feature set + extended advertising APIs (how-to)

### When to choose this

- You want to stick with ESP-IDF 5.4.1 defaults (BLE 5.0 enabled by default in Bluedroid).
- You want modern advertising features (extended/periodic/multi-adv).

### Config recipe (project-level)

In `sdkconfig.defaults`:

```ini
# Bluedroid + BLE enabled
CONFIG_BT_ENABLED=y
CONFIG_BT_BLUEDROID_ENABLED=y
CONFIG_BT_BLE_ENABLED=y

# Choose BLE 5.0 feature set (mutually exclusive with BLE 4.2)
CONFIG_BT_BLE_50_FEATURES_SUPPORTED=y
CONFIG_BT_BLE_42_FEATURES_SUPPORTED=n

# Optional: disable classic BT at a higher level if present
CONFIG_BT_CLASSIC_ENABLED=n
```

### API flow (what you implement)

Extended advertising is callback-driven. A minimal, robust sequencing pattern is:

1. `esp_ble_gap_ext_adv_set_params(instance, &params)`
2. (optional) `esp_ble_gap_ext_adv_set_rand_addr(instance, rand_addr)` if using random addresses
3. `esp_ble_gap_config_ext_adv_data_raw(instance, len, data)`
4. (optional) `esp_ble_gap_config_ext_scan_rsp_data_raw(instance, len, scan_rsp)`
5. `esp_ble_gap_ext_adv_start(num_adv, ext_adv_array)`

Key GAP events (ESP-IDF 5.4.1):
- `ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT`
- `ESP_GAP_BLE_EXT_ADV_SET_RAND_ADDR_COMPLETE_EVT` (if used)
- `ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT`
- `ESP_GAP_BLE_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT` (if used)
- `ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT`
- `ESP_GAP_BLE_EXT_ADV_STOP_COMPLETE_EVT`

Reference examples (extended adv in 5.4.1):

- Closest to “GATT server + extended advertising”:
  - `/home/manuel/esp/esp-idf-5.4.1/examples/bluetooth/bluedroid/ble_50/ble50_throughput/throughput_server/main/example_ble_server_throughput.c`
- Clean, explicit sequencing with semaphores:
  - `/home/manuel/esp/esp-idf-5.4.1/examples/bluetooth/bluedroid/ble_50/periodic_adv/main/periodic_adv_demo.c`
  - plus `/home/manuel/esp/esp-idf-5.4.1/examples/bluetooth/bluedroid/ble_50/periodic_adv/tutorial/Periodic_adv_Example_Walkthrough.md`

### Data format difference vs legacy adv

Legacy advertising often uses structured `esp_ble_adv_data_t` (IDF builds the payload).

Extended advertising in these examples uses **raw AD structures** (you build the payload bytes) and passes them to:
- `esp_ble_gap_config_ext_adv_data_raw(...)`
- `esp_ble_gap_config_ext_scan_rsp_data_raw(...)`

## Recommendation for this repo (pragmatic)

- If the goal is to get a **known-good baseline firmware compiling quickly**, choose **Option A (BLE 4.2 + legacy advertising)**.
- If the goal is to align with ESP-IDF 5.4.1’s **default** and avoid toggling the feature set, choose **Option B (BLE 5.0 + extended advertising)** and port the advertising portion of the ticket firmware to follow `ble50_throughput`’s pattern.


