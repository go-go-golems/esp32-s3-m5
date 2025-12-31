# Changelog

## 2025-12-30

- Initial workspace created


## 2025-12-30

Created comprehensive analysis documents: BLE implementation analysis, temperature sensor analysis, debugging playbook, and implementation guide. Identified Bluedroid stack usage, SHT3x sensor recommendation, and key implementation patterns from existing codebase.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/ble_keyboard_wrap.cpp — Reference implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c — Sensor example


## 2025-12-30

Added implementation task breakdown: start with mock temperature source, bring up Bluedroid GATT+notify, add host-side bleak client + stress tests, then integrate real SHT3x I2C sensor and finalize docs.


## 2025-12-30

Added Linux Bluetooth keyboard pairing playbook (scan/pair/trust/connect) and recorded host-side debugging diary step; includes rfkill/Powered troubleshooting, agent usage, and GUI tool suggestions.


## 2025-12-30

Added troubleshooting section for 'Connected but keyboard not working' issue: ClassicBondedOnly blocks HID from unbonded devices; documented fix (disable ClassicBondedOnly or force bonding).


## 2025-12-30

Updated playbook: emphasized interactive bluetoothctl for PIN entry (required for bonding), added bonding verification steps (check /var/lib/bluetooth directory), clarified Paired vs Bonded distinction and why bonding matters for HID.


## 2025-12-30

Added detailed explanation of Pairing vs Bonding distinction to playbook: pairing exchanges keys in memory, bonding persists keys to disk. Documented that some legacy keyboards never achieve Bonded: yes even with successful pairing, and disabling ClassicBondedOnly is the practical workaround.

## 2025-12-31

Created an ESP-IDF 5.4.1 (Bluedroid) analysis document comparing BLE 4.2 vs BLE 5.0 feature sets and how that choice gates legacy vs extended advertising APIs. Includes copy/paste how-to guides and cross-validation notes against intern research.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0019-BLE-TEST--ble-temperature-sensor-logger-on-cardputer/analysis/03-ble-4-2-vs-ble-5-0-advertising-apis-esp-idf-5-4-1-bluedroid.md — New analysis doc (how-to guides)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0019-BLE-TEST--ble-temperature-sensor-logger-on-cardputer/sources/research-ble-advertising-error.md — Intern research (cross-validated)

## 2025-12-31

Added an end-to-end test playbook (tmux workflow) for flashing + monitoring firmware and validating scan/connect/read/notify from Linux using the provided `bleak` client script.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0019-BLE-TEST--ble-temperature-sensor-logger-on-cardputer/playbook/02-e2e-test-cardputer-ble-temp-logger--tmux.md — E2E test playbook

## 2025-12-31

Implemented and validated the BLE 5.0 baseline firmware for 0019 (Bluedroid GATTS + extended advertising) and committed the project.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c — Firmware implementation (commit 1c57e6fb5aa9cce1f13a95721422d0417648b011)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/tools/run_fw_flash_monitor.sh — Firmware runner script (tmux-friendly)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/tools/run_host_ble_client.sh — Host runner script (scan/connect/read/notify)

