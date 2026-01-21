---
Title: Design + implementation plan
Ticket: 0050-WIFI-MGR-COMPONENT
Status: active
Topics:
    - esp-idf
    - wifi
    - console
    - cardputer
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.c
      Note: Source implementation (0046)
    - Path: 0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.h
      Note: Public API (0046)
    - Path: 0048-cardputer-js-web/main/wifi_mgr.c
      Note: Source implementation (0048)
    - Path: 0048-cardputer-js-web/main/wifi_mgr.h
      Note: Public API (0048)
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:36:20.634947465-05:00
WhatFor: ""
WhenToUse: ""
---


# Design + implementation plan

## Executive Summary

Extract the duplicated “console-friendly Wi‑Fi STA manager” into a reusable ESP‑IDF component, `components/wifi_mgr`, and migrate existing firmwares (`0046-xiao-esp32c6-led-patterns-webui`, `0048-cardputer-js-web`) to consume it.

This component keeps the **public C API** stable (`wifi_mgr_start`, `wifi_mgr_get_status`, `wifi_mgr_scan`, …) while standardizing the Kconfig knobs and eliminating copy‑paste drift.

## Problem Statement

We currently have multiple nearly-identical `wifi_mgr.c/.h` implementations:

- `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.c`
- `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c` (explicitly adapted from 0046)

Problems this creates:

1) **Copy/paste divergence**: bug fixes (retry policy, NVS details, status fields) land in one firmware and never reach the others.
2) **Kconfig name drift**: 0046 uses `CONFIG_MO033_WIFI_*`, 0048 uses `CONFIG_TUTORIAL_0048_WIFI_*`. The semantics are the same, but the names are not, making reuse awkward and error-prone.
3) **No single “source of truth”**: future tickets will keep re-implementing the same STA provisioning story (scan/join/save/status) with slightly different edges.

The end result is that “Wi‑Fi STA + console provisioning” remains a recurring tax on every web/WS ticket.

## Proposed Solution

Create a shared ESP‑IDF component:

- `esp32-s3-m5/components/wifi_mgr/`
  - `include/wifi_mgr.h` (public API; keep stable)
  - `wifi_mgr.c` (implementation; derived from current 0048/0046)
  - `Kconfig` (standardize config names under `CONFIG_WIFI_MGR_*`)
  - `CMakeLists.txt` (idf_component_register)

### Public API (keep as-is)

Keep these symbols and signatures (used by multiple firmwares already):

- `esp_err_t wifi_mgr_start(void);`
- `esp_err_t wifi_mgr_get_status(wifi_mgr_status_t* out);`
- `esp_err_t wifi_mgr_set_credentials(const char* ssid, const char* password, bool save_to_nvs);`
- `esp_err_t wifi_mgr_clear_credentials(void);`
- `esp_err_t wifi_mgr_connect(void);`
- `esp_err_t wifi_mgr_disconnect(void);`
- `esp_err_t wifi_mgr_scan(wifi_mgr_scan_entry_t* out, size_t max_out, size_t* out_n);`
- `void wifi_mgr_set_on_got_ip_cb(wifi_mgr_on_got_ip_cb_t cb, void* ctx);`

Reference header today:

- `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.h` (same as 0046)

### Standardize configuration

Replace per-project config names with shared names:

- `CONFIG_WIFI_MGR_AUTOCONNECT_ON_BOOT` (bool)
- `CONFIG_WIFI_MGR_MAX_RETRY` (int)

Mapping:

- 0046 currently uses:
  - `CONFIG_MO033_WIFI_AUTOCONNECT_ON_BOOT`
  - `CONFIG_MO033_WIFI_MAX_RETRY`
- 0048 currently uses:
  - `CONFIG_TUTORIAL_0048_WIFI_AUTOCONNECT_ON_BOOT`
  - `CONFIG_TUTORIAL_0048_WIFI_MAX_RETRY`

Both are updated during migration (see Implementation Plan).

### State machine (pseudocode)

The runtime behavior is unchanged; we are extracting and renaming knobs:

```text
wifi_mgr_start():
  ensure_nvs()
  ensure_netif_and_default_event_loop()
  create default STA netif
  esp_wifi_init()
  register wifi_event_handler for WIFI_EVENT + IP_EVENT
  load_saved_credentials() -> s_has_runtime/s_has_saved
  s_autoconnect = CONFIG_WIFI_MGR_AUTOCONNECT_ON_BOOT && s_has_runtime
  esp_wifi_set_mode(STA); esp_wifi_start()

wifi_event_handler(WIFI_EVENT_STA_START):
  if s_autoconnect && creds_present:
    esp_wifi_connect(); status=CONNECTING
  else:
    status=IDLE

wifi_event_handler(WIFI_EVENT_STA_DISCONNECTED):
  status=IDLE; reason=...
  if s_autoconnect && creds_present && retry < MAX_RETRY:
    retry++; esp_wifi_connect(); status=CONNECTING

wifi_event_handler(IP_EVENT_STA_GOT_IP):
  status=CONNECTED; retry=0; ip4=...
  call on_got_ip_cb(ip4)
```

## Design Decisions

1) **Keep the C API stable.**
   - This component is glue that many projects will want; stability beats cleverness.

2) **Standardize Kconfig names, do not attempt “compat shims”.**
   - Supporting both `CONFIG_MO033_*` and `CONFIG_TUTORIAL_0048_*` inside a shared component would reintroduce drift and confusion.
   - The migration updates the affected firmwares explicitly.

3) **Keep the NVS namespace and keys stable (`wifi` / `ssid` / `pass`).**
   - This avoids silent “lost credentials” regressions across upgrades.

4) **Expose behavior via config, not via more runtime API.**
   - The existing `wifi_console` UX already provides runtime actions (connect/disconnect/clear).

## Alternatives Considered

1) Keep copy/pasting `wifi_mgr.c` per firmware.
   - Rejected: guarantees repeated debugging and inconsistent behavior.

2) Use ESP-IDF provisioning frameworks (SoftAP, BLE, Wi‑Fi provisioning component).
   - Rejected: larger surface area and a different UX than the “console-driven” model used in these tickets.

3) Create a “super” wifi_mgr that owns the console too.
   - Rejected: console provisioning is better extracted as a separate component (`0051-WIFI-CONSOLE-COMPONENT`) with an extension hook.

## Implementation Plan

### 1) Create the component skeleton

Create:

- `esp32-s3-m5/components/wifi_mgr/CMakeLists.txt`
- `esp32-s3-m5/components/wifi_mgr/Kconfig`
- `esp32-s3-m5/components/wifi_mgr/include/wifi_mgr.h`
- `esp32-s3-m5/components/wifi_mgr/wifi_mgr.c`

### 2) Port code and normalize config/logging

Start from:

- `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c` (already a cleaned-up variant)

Edits:

- Replace config references:
  - `CONFIG_TUTORIAL_0048_WIFI_MAX_RETRY` → `CONFIG_WIFI_MGR_MAX_RETRY`
  - `CONFIG_TUTORIAL_0048_WIFI_AUTOCONNECT_ON_BOOT` → `CONFIG_WIFI_MGR_AUTOCONNECT_ON_BOOT`
- Use a stable log tag:
  - `static const char* TAG = "wifi_mgr";`

### 3) Migrate affected firmwares

#### 0046-xiao-esp32c6-led-patterns-webui

- Remove local copies:
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.c`
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.h`
- Update includes (if necessary) to use component header.
- Update build wiring:
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/CMakeLists.txt` to depend on `wifi_mgr`.
- Update Kconfig symbols:
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/Kconfig.projbuild`
    - remove `CONFIG_MO033_WIFI_*`
    - set defaults for `CONFIG_WIFI_MGR_*` if needed.

#### 0048-cardputer-js-web

- Remove local copies:
  - `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c`
  - `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.h`
- Update build wiring:
  - `esp32-s3-m5/0048-cardputer-js-web/main/CMakeLists.txt`
- Update Kconfig symbols:
  - `esp32-s3-m5/0048-cardputer-js-web/main/Kconfig.projbuild`
    - remove `CONFIG_TUTORIAL_0048_WIFI_*`
    - use `CONFIG_WIFI_MGR_*`

### 4) Validate

- Build 0046 (esp32c6) and 0048 (esp32s3).
- Manual smoke:
  - `wifi scan`
  - `wifi join <idx> <pass> --save`
  - reboot and confirm autoconnect behavior matches config.

## Open Questions

1) Should `wifi_mgr` support multiple netifs (STA + AP) in one instance?
   - For now: no; keep single STA netif behavior.

2) Should we add `wifi_mgr_stop()`?
   - Not required for current use; can be added later as an additive API.

## References

- Prior art implementations:
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.c`
  - `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c`
- Consumers:
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/app_main.c` (`wifi_mgr_start`)
  - `esp32-s3-m5/0048-cardputer-js-web/main/app_main.cpp` (`wifi_mgr_start`)
