# Changelog

## 2026-08-17

- Initial workspace created


## 2026-08-17

Step 1: ticket created, evidence maps collected (sources/01, sources/02), diary step 1 (commit 68fb191b)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/17/ESP-55-PULP-APP-LOADER--pulp-os-app-modularization-split-pulp-js-into-per-app-modules-dynamic-app-loading-from-sd-card-and-over-http-and-a-launcher/sources/01-native-side-map.md — native map


## 2026-08-17

Step 2: host experiments — trial split bytecode sizes, source-eval vs bytecode arena/time harness; second image fails with 'too many rom atom tables'

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/17/ESP-55-PULP-APP-LOADER--pulp-os-app-modularization-split-pulp-js-into-per-app-modules-dynamic-app-loading-from-sd-card-and-over-http-and-a-launcher/scripts/02-host-eval-harness.c — harness


## 2026-08-17

Step 3: intern guide written (design-doc/01), diary steps 2-3 (commit c5bb6e1a)

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/ttmp/2026/08/17/ESP-55-PULP-APP-LOADER--pulp-os-app-modularization-split-pulp-js-into-per-app-modules-dynamic-app-loading-from-sd-card-and-over-http-and-a-launcher/design-doc/01-intern-guide-pulp-os-app-modularization-dynamic-app-loading-sd-http-and-launcher-analysis-design-implementation.md — guide


## 2026-08-17

Step 4: bookkeeping + doctor clean; reMarkable upload blocked by cloud 'invalid root schema' (rmapi v4 root index rejected) — task left open


## 2026-08-17

Step 6: remarquee fixed (root index sorted); bundle uploaded to /ai/2026/08/17/ESP-55-PULP-APP-LOADER as 'ESP-55 PULP App Loader Intern Guide.pdf'


## 2026-08-19

Step 7: design extended — multi-context runtime (§6.11), page-script browser with UI-only sandbox stdlib (§6.12), R-MULTICTX/R-UISANDBOX/R-PAGESCRIPT, Phases 8–10


## 2026-08-19

Step 8 (P0): js measure op + JS_GetHeapUsed + arena_used flashed (4d59929a); device wedged in ROM download mode (force-download latch), needs power-on reset; hold-open console client added


## 2026-08-19

Step 9 (P0 done): measured on device — dice 35 ms/+3.0 KB, settings 81 ms/+7.5 KB retained, x10 flat, boot retention 7.5 KB; gate passed; guide §4.3 updated


## 2026-08-19

Phases 1-3 implemented and hardware-validated: pulp.js split (17557c9a), descriptors+facade+loader (9ee1fd6f), native load()+ROM assets, image 45.3->10.7 KB, +34.7 KB internal RAM, all apps load in 15-35 ms, probe 23, ten-launch flatness

### Related Files

- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os/main/app_js.cpp — real js_load with deadline save/restore and load counters
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os/main/js_assets.cpp — ROM app asset registry (EMBED_TXTFILES)
- /home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os/tools/js/os/30-loader.js — gc-load-validate-enter-main loader


## 2026-08-19

Phases 4-7 complete: SD catalog+seeding (5d969185), HTTP push+hot reload (7c6c65da), pull install + load NUL fix + list cap (52d0c827), soak 25 cycles/275 loads/0 exceptions, README updated

