# Changelog

## 2026-07-16

- Initial workspace created


## 2026-07-16

Ticket created: connectivity intern guide (13 sections, 8 phases, 19 tasks) designed from go-go-goja/widgetdsl DSL patterns adapted to mquickjs constraints; uploaded to reMarkable /ai/2026/07/16/ESP-53-PULP-CONNECTIVITY

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/16/ESP-53-PULP-CONNECTIVITY--pulp-os-connectivity-and-peripherals-wifi-http-fetch-web-serving-filesystem-buzzer/design-doc/01-connectivity-intern-guide-analysis-design-and-implementation.md — The guide


## 2026-07-16

Added design-doc/02: full-system onboarding guide (hardware, component stack, present pipeline, MicroQuickJS facts, binding layer, bytecode toolchain, JS API reference, console/validation harness, gotcha catalog) as prerequisite reading for design-doc/01

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/07/16/ESP-53-PULP-CONNECTIVITY--pulp-os-connectivity-and-peripherals-wifi-http-fetch-web-serving-filesystem-buzzer/design-doc/02-pulp-os-system-onboarding-guide-every-part-of-the-system-for-a-new-intern.md — The new onboarding guide


## 2026-07-16

P0+P1 complete: buzzer module app_buzzer.{h,cpp} (lazy LEDC GPIO21, owner-tick note sequencer, 16-note melody parser), buzzer JS singleton (tone/beep/stop/melody), buzz console command via ConsoleOp::Buzz, product chimes (tea READY melody, postcard seal click, 2048 merge tone). Hardware gate passed: beep/tone/melody audible, melody 5/5 notes sequenced, js exceptions=0 (commit f57c61b)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/app_buzzer.cpp — Buzzer module with owner-tick sequencer


## 2026-07-16

P2+P3.1 complete: ModuleDone event kind + PostModuleDone + per-module pending-cb registry (JsModuleDone, cleared by resetTree); files module app_files.{h,cpp} (sanitizer denying dot-segments//sdcard/.s3paper, list/read/write/append/remove with mailboxes, exists sync); js_files.cpp bindings; probe 15 green on hardware: denials, 16KiB cap, busy rejection, async write-read-list-remove chain

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0114-papers3-pulp-os/main/app_files.cpp — Files module with path sanitizer and mailboxes

