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


## 2026-01-21

Step 3: Improve mqjs_service boot instrumentation (commit 6a562f3)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/mqjs_service.cpp — Log task name/priority/core before first xQueueReceive


## 2026-01-21

Analysis: explain why WDT started after extracting mqjs_service (stack-depth unit mismatch)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/21/0061-MQJS-SERVICE-BOOT-WDT--interrupt-wdt-timeout-in-mqjs-service-service-task-xqueuereceive-at-boot/analysis/02-why-boot-wdt-started-after-extracting-mqjs-service.md — Compares pre/post extraction; identifies stack words vs bytes

