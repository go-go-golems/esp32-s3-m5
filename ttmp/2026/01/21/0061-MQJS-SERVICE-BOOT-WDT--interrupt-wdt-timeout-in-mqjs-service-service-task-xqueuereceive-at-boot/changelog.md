# Changelog

## 2026-01-21

- Initial workspace created


## 2026-01-21

Step 1: Create bug ticket, consolidate WDT-at-xQueueReceive evidence, define next experiments

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/21/0061-MQJS-SERVICE-BOOT-WDT--interrupt-wdt-timeout-in-mqjs-service-service-task-xqueuereceive-at-boot/analysis/01-bug-report.md — Consolidated logs + hypotheses + reproduction script


## 2026-01-21

Step 2: Confirm flashed binary; force mqjs_service queue/semaphore internal RAM (commit 0b17809)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/mqjs_service.cpp — Use xQueueCreateWithCaps/xSemaphoreCreateBinaryWithCaps + vQueueDeleteWithCaps

