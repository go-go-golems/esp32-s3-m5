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

