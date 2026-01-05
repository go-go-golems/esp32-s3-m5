# Changelog

## 2026-01-04

- Initial workspace created


## 2026-01-04

Added playbook for bringing up ESP Zigbee Gateway (Unit Gateway H2 + CoreS3) and archived the upstream guide.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/playbook/01-esp-zigbee-gateway-unit-gateway-h2-cores3.md — Primary bring-up procedure
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_gateway.txt — Source guide snapshot


## 2026-01-04

Backfilled diary and built ESP32-H2 ot_rcp firmware (ESP-IDF 5.4.1).

### Related Files

- /home/manuel/esp/esp-5.4.1/examples/openthread/ot_rcp/build/esp_ot_rcp.bin — Built RCP firmware artifact
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/01-diary.md — Detailed backfill and build notes


## 2026-01-04

Flashed ESP32-H2 ot_rcp firmware to Unit Gateway H2 on /dev/ttyACM0.

### Related Files

- /home/manuel/esp/esp-5.4.1/examples/openthread/ot_rcp/build/esp_ot_rcp.bin — Flashed image
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/01-diary.md — Step 5 flash command + output


## 2026-01-04

Added reference doc explaining RCP+Spinel vs NCP (ZNP-style) and compared the M5Stack zigbee_gateway vs zigbee_ncp guides; archived zigbee_ncp source snapshot.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/01-diary.md — Step 6 research notes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md — RCP vs NCP explanation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_ncp.txt — Archived source snapshot


## 2026-01-04

Uploaded ticket docs to reMarkable; built+flashed esp_zigbee_ncp (H2) per M5Stack zigbee_ncp; built ESP-IDF zigbee gateway example.

### Related Files

- /home/manuel/esp/esp-idf-5.4.1/examples/zigbee/esp_zigbee_gateway/build/esp_zigbee_gateway.bin — Built ESP-IDF example
- /home/manuel/esp/esp-zigbee-sdk/examples/esp_zigbee_ncp/build/esp_zigbee_ncp.bin — Flashed NCP firmware
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/01-diary.md — Steps 7-9 (reMarkable upload


## 2026-01-04

Step 12: Created bugreport ticket + UART smoke tests; after reseating Grove cable, confirmed bidirectional UART bytes on Port C pins.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/01-diary.md — Record cable reseat + smoke test validation


## 2026-01-04

Step 13: Zigbee NCP host↔NCP handshake succeeds; network formed and steering started after fixing UART pattern queue reset.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/01-diary.md — Record successful NCP run


## 2026-01-04

Step 15: Re-tested CoreS3 host with enclosed H2; host↔NCP UART ZNSP traffic and permit-join still works.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/various/logs/20260104-185952-cores3-host-only-retest-script-120s.log — Evidence log (host-only)


## 2026-01-04

Ticket closed


## 2026-01-04

Added architecture/onboarding design-doc for the NCP-based CoreS3 host + Unit Gateway H2 setup (ZNSP over SLIP/UART), including build steps, diagrams, API references, and test evidence.


## 2026-01-04

Uploaded developer guide PDF to reMarkable: ai/2026/01/04/001-ZIGBEE-GATEWAY/design-doc/01-developer-guide-cores3-host-unit-gateway-h2-ncp-znsp-over-slip-uart.pdf

### Related Files

- 2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/design-doc/01-developer-guide-cores3-host-unit-gateway-h2-ncp-znsp-over-slip-uart.md — Source markdown uploaded via remarkable_upload.py

