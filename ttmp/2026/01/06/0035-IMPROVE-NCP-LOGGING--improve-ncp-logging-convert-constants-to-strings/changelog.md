# Changelog

## 2026-01-06

- Initial workspace created


## 2026-01-06

Created analysis document evaluating different approaches for constant-to-string conversion. Recommended hybrid approach: codegen for command IDs (60+ values), manual switch/case for small enums (4-6 values).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0035-IMPROVE-NCP-LOGGING--improve-ncp-logging-convert-constants-to-strings/analysis/01-ncp-logging-constant-conversion-analysis.md — Analysis document with approach evaluation


## 2026-01-06

Expanded analysis to include lower-level ESP_ZB stack symbols: ZDO signal types (~60+), ZDP status (16), ZCL status (~30), ZCL attribute types (~50+), APS addressing modes, and cluster IDs. Identified that esp_zb_zdo_signal_to_string() exists but is a stub - needs implementation. Recommended codegen approach for all large enum sets.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0035-IMPROVE-NCP-LOGGING--improve-ncp-logging-convert-constants-to-strings/analysis/01-ncp-logging-constant-conversion-analysis.md — Updated analysis with lower-level ESP_ZB symbols


## 2026-01-06

Clarified that lower-level ESP_ZB symbols are from the esp-zigbee-lib managed component (managed_components/espressif__esp-zigbee-lib), not just wrapper headers. Confirmed actual header locations and enum definitions in the managed component.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/06/0035-IMPROVE-NCP-LOGGING--improve-ncp-logging-convert-constants-to-strings/analysis/01-ncp-logging-constant-conversion-analysis.md — Updated to reference actual managed component headers

