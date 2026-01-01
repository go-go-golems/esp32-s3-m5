# Changelog

## 2025-12-30

- Initial workspace created
- Added in-depth analysis docs for BLE HID host firmware + USB `esp_console` design
- Started implementation diary with API surface mapping and console decisions

## 2025-12-30

Documented esp_hidh init crash-loop signature (gattc_app_register failed + pxQueue assert) and expanded Step 6 diary with verbatim logs + fix notes.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/design-doc/02-bug-report-ble-hid-host-init-crash-loop-gattc-app-register-failed-queue-assert-pxqueue.md — Dedicated bug report for the crash-loop
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/playbook/02-intern-research-instructions-esp-hidh-init-crash-loop-gattc-app-register-failed-pxqueue-assert.md — Internet research playbook for confirming root cause/contract
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/reference/01-diary.md — Step 6 now includes detailed crash-loop timeline


## 2025-12-30

Added operator playbook and improved connect diagnostics: decode GATTC open status (0x85→ESP_GATT_ERROR) and disconnect reason (0x0100→ESP_GATT_CONN_CONN_CANCEL); connect now waits for scan stop complete (commit 6a96946).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c — Decoded logs + scan-stop→open sequencing (commit 6a96946)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/playbook/03-using-ble-console-to-scan-connect-pair-and-keylog-a-ble-keyboard.md — How to use ble> to scan/connect/pair/keylog
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/reference/01-diary.md — Step 7 records conclusions + changes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/sources/ble-gatt-bug-report-research.md — Intern research mapping the error codes


## 2025-12-30

Centralized BLE/GATT code decoding (esp_gatt_status_t / esp_gatt_conn_reason_t / esp_bt_status_t), added ble> codes command to print all mappings, and upgraded logs to include name+description (commit 005f98f).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c — Decoded logging for scan/gatt open/close/disconnect (commit 005f98f)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/bt_console.c — New codes command to dump tables (commit 005f98f)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/bt_decode.c — New decoder tables + print_all helper (commit 005f98f)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/playbook/03-using-ble-console-to-scan-connect-pair-and-keylog-a-ble-keyboard.md — Playbook updated to mention codes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/reference/01-diary.md — Step 8 documents the decoding module and codes command


## 2025-12-30

Uploaded 0020 BLE keyboard host research/bug reports/playbooks/diary to reMarkable as PDFs under ai/2025/12/30/0020-BLE-KEYBOARD/ (mirrored ticket structure; used --force to refresh existing entries).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/reference/01-diary.md — Step 9 records the reMarkable upload and verification commands


## 2025-12-31

Step 10-12: Improve BLE discovery UX (filter/clear/events + better names), fix pair/encrypt sequencing, add scripts for plz-confirm pairing and tmux monitor.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c — Discovery events + scan name parsing + pair connect-first
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/bt_console.c — devices clear/events + pair-by-index
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/scripts/pair_debug.py — Automated pairing flow to avoid timing gaps


## 2025-12-31

Analysis: explain why keyboard appears in bluetoothctl but not ESP32 BLE scan; add LE vs BR/EDR confirmation steps.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/analysis/03-why-keyboard-shows-in-bluetoothctl-but-not-on-esp32-scan.md — Investigation guide and decision tree


## 2025-12-31

Analysis: explain Classic (BR/EDR) vs BLE for keyboards and what it takes to add Classic HID host / dual-mode in ESP-IDF.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/analysis/04-adding-classic-br-edr-hid-keyboard-support-and-dual-mode-in-esp-idf.md — Architecture + config + API pointers for Classic HID support


## 2025-12-31

Analysis: explain BLE vs BR/EDR security registries (bond databases), CTKD caveats, and how to build a safe unified peer registry.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/analysis/05-ble-vs-br-edr-security-registries-bond-databases-why-they-aren-t-trivially-mergeable.md — Security model deep dive


## 2025-12-31

Design: document dual discovery + dual security registries and unified peer registry for LE+BR/EDR; add tasks to implement dual-mode.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/design-doc/03-design-dual-security-registries-unified-peer-registry-le-br-edr.md — Dual-mode registry design and console UX
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/tasks.md — Added dual-mode implementation tasks

