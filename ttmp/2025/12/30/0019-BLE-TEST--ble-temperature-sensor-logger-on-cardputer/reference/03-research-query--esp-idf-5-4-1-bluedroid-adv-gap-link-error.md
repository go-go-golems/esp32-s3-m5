---
Title: "Research Query: ESP-IDF 5.4.1 Bluedroid advertising GAP APIs link error"
Ticket: 0019-BLE-TEST
Status: active
Topics:
    - ble
    - esp-idf
    - esp32s3
    - cardputer
DocType: reference
Intent: short-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c
      Note: Uses `esp_ble_gap_config_adv_data()` + `esp_ble_gap_start_advertising()` (fails to link)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/sdkconfig
      Note: Generated config currently enables BLE 5.0 features, disables BLE 4.2 features
ExternalSources: []
Summary: Ask a research agent to find the correct ESP-IDF 5.4.1 (Bluedroid) configuration/API changes to resolve missing GAP advertising symbols during link.
LastUpdated: 2025-12-31T00:00:00Z
WhatFor: "Provide a clear internet research task and expected output"
WhenToUse: "When debugging BLE advertising bring-up in ESP-IDF 5.4.1 Bluedroid"
---

# Research Query: ESP-IDF 5.4.1 Bluedroid advertising GAP APIs link error

## Context

We are building a new ESP-IDF app for ESP32-S3 (Cardputer) at:

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/`

The firmware implements a Bluedroid GATT server and attempts to start advertising using the “classic” helper APIs:

- `esp_ble_gap_config_adv_data(...)`
- `esp_ble_gap_start_advertising(...)`

Build command:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger && \
idf.py set-target esp32s3 && idf.py build
```

Current failure is **link-time** (not compile-time):

```text
undefined reference to `esp_ble_gap_start_advertising'
undefined reference to `esp_ble_gap_config_adv_data'
```

Observations already made (local code inspection):

- In ESP-IDF 5.4.1, these functions are implemented in `components/bt/host/bluedroid/api/esp_gap_ble_api.c`.
- Their definitions are guarded by `#if (BLE_42_FEATURE_SUPPORT == TRUE)`.
- The project’s generated `sdkconfig` currently has:
  - `CONFIG_BT_BLE_50_FEATURES_SUPPORTED=y`
  - `# CONFIG_BT_BLE_42_FEATURES_SUPPORTED is not set`

So the build likely excludes those symbols from `libbt.a`, making the linker fail.

## What the research agent should search for (internet query)

Search terms (use multiple variations):

- “ESP-IDF 5.4.1 undefined reference esp_ble_gap_start_advertising”
- “ESP-IDF esp_ble_gap_config_adv_data BLE_42_FEATURE_SUPPORT”
- “ESP-IDF 5.4 bluedroid advertising API esp_ble_gap_ext_adv_start example”
- “CONFIG_BT_BLE_42_FEATURES_SUPPORTED vs CONFIG_BT_BLE_50_FEATURES_SUPPORTED bluedroid advertising”
- “ESP-IDF 5.4 bluedroid legacy advertising disabled when BLE 5 enabled”

## Questions to answer (deliverable)

### 1) Correct Kconfig symbols + exact settings

- Which `sdkconfig` options (ESP-IDF 5.4.1) control `BLE_42_FEATURE_SUPPORT` for Bluedroid GAP advertising?
- Is it safe/valid to set both BLE 4.2 and BLE 5.0 feature flags simultaneously?
- If only one can be enabled: what is the recommended choice for a **simple GATT server + advertising + notify** use case?
- Where is the authoritative documentation for these config options? (ESP-IDF docs / Kconfig docs / examples)

### 2) If we keep BLE 5.0 features: what APIs should we use instead?

If BLE 4.2 helper APIs are intentionally excluded when BLE 5.0 features are enabled:

- What is the correct **Bluedroid** advertising bring-up flow in ESP-IDF 5.4.1 using extended advertising?
- Which function calls replace:
  - `esp_ble_gap_config_adv_data`
  - `esp_ble_gap_start_advertising`
- Provide a minimal example snippet (GAP event handling + set data + start advertising).

### 3) What is the cleanest way to apply config changes in an existing project?

We discovered that editing `sdkconfig.defaults` did not affect the already-generated `sdkconfig`.

- What’s the recommended workflow to ensure a project adopts new defaults?
  - `idf.py reconfigure` vs `idf.py fullclean` vs deleting `sdkconfig`?
  - How do Espressif examples handle this in practice?

### 4) Confirm expectations about symbol availability

- Under what configs should `libbt.a` contain `esp_ble_gap_start_advertising`?
- If possible, point to official example(s) in ESP-IDF 5.4.x that still call these helper APIs successfully.

## Output format requested from the research agent

- Short “recommended path” (Option A: enable BLE 4.2 helper APIs, or Option B: switch to extended adv APIs)
- The exact config symbols and values to set (copy/paste ready)
- Minimal working advertising code snippet (copy/paste ready)
- Links to official docs and/or example source (no “hand-wavy” answers)


