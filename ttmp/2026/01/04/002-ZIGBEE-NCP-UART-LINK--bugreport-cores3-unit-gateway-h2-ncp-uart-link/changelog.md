# Changelog

## 2026-01-04

- Initial workspace created


## 2026-01-04

Created bugreport + UART smoke-test firmware; after reseating cable, smoke test shows bidirectional PING/PONG over CoreS3 Port C (TX17/RX18) ↔ H2 (TX24/RX23).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/playbook/01-uart-smoke-test-firmware-cores3-port-c-unit-gateway-h2.md — Smoke test procedure + projects
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/reference/01-bugreport-cores3-h2-ncp-uart-link-appears-dead.md — Bugreport with deltas vs guide + evidence


## 2026-01-04

NCP now works: after adding uart_pattern_queue_reset() on host+NCP, UART_PATTERN_DET triggers, SLIP/ZNSP frames decode, and network formation + steering succeed.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/04/002-ZIGBEE-NCP-UART-LINK--bugreport-cores3-unit-gateway-h2-ncp-uart-link/reference/01-bugreport-cores3-h2-ncp-uart-link-appears-dead.md — Added resolved status + evidence logs

