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


## 2026-07-16

Step 5: Phase 4 complete - 0114 skeleton boots: owner/console/input/power/native home on shared components (commit f722b77)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/app_owner.cpp — Boot flow and owner loop


## 2026-07-16

Step 6: Phases 5+6 - v2 builder API live on hardware (native classes, closures, direct dispatch, dyn values); fixed TTF-only MeasureText core bug (commit e5f6313)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/s3paper_core/src/text.cpp — MeasureText/BreakLines TTF-only guard fix


## 2026-07-16

Step 7: Phase 7 - all PULP apps live on v2 from one bytecode image; operator-driven UX fixes (tap targets, separator margins, Cyrillic seed book) (commit c9119b0)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/tools/js/apps/pulp.js — PULP OS v2 apps


## 2026-07-16

Step 8: Phase 8 - boot-to-launcher, JS sleep image, deep+rtc-off wake matrix, kFontTitle shelf, 2048 re-blank (commit c33e1c7)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/app_power.cpp — JS sleep image path


## 2026-07-16

Step 9: P9 hardening - fault battery (deadline 804ms, storm contained), trace-equivalence EQUAL 831B, 258-command mixed soak heap-flat (commits 0715fdd, b2704a8)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/js_probes.cpp — Fault + trace-equivalence probes


## 2026-07-16

Global margin toggle: all 17 horizontal pad sites hoisted to var M; long-press on the launcher flips 40px <-> 0 and persists via storeSet('margin')

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/tools/js/apps/pulp.js — Margin variable + toggle

