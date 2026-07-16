# Changelog

## 2026-07-16

- Initial workspace created


## 2026-07-16

Ticket created: intern guide (design-doc/01, 445 lines: architecture, component tour, mquickjs cookbook, v2 builder API design, gotcha list, diary index) plus 10 phases / 65 granular tasks for extracting s3paper components from 0112 and building the 0114 PULP OS v2 firmware

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/16/ESP-51-PULP-OS-V2--pulp-os-v2-native-builder-js-api-over-extracted-s3paper-components/design-doc/01-pulp-os-v2-intern-guide-analysis-design-and-implementation.md — The handoff guide


## 2026-07-16

Step 1: Phase 0 orientation complete - host suite 37989 green, 0112 builds (1008K), device console smoke transcript saved

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/16/ESP-51-PULP-OS-V2--pulp-os-v2-native-builder-js-api-over-extracted-s3paper-components/scripts/output/p0-baseline-smoke.log — Phase 0 device evidence


## 2026-07-16

Step 2: Phase 1 complete - components promoted to repo components/, 0112 re-pointed and device-validated (commit 9d80478)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0112-papers3-reader-primitives/CMakeLists.txt — EXTRA_COMPONENT_DIRS + trimmed COMPONENTS


## 2026-07-16

Step 3: Phase 2 complete - s3paper_storage extracted with injected pre-mount hook, seed book, and fault-injection console; full recovery battery proven on hardware (commit f018182)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/s3paper_storage/include/s3paper_storage/storage.h — Component API


## 2026-07-16

Step 4: Phase 3 complete - s3paper_runtime extracted; 0112 device gate green (trace EQUAL, region ticks, blitz 460x86) (commit c0f9eb7)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/s3paper_runtime/include/s3paper_runtime/runtime.h — Runtime component API

