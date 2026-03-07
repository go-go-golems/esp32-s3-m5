# Changelog

## 2026-03-06

- Initial workspace created

## 2026-03-06

Created ticket `ESP-27-M5DIAL-FILM-DEVELOPER`, added the primary design document and investigation diary, and mapped the existing `0072` M5Dial timer app as the implementation base for the future `0073` film developer timer app.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/app_main.cpp — Current event-bus and task split that should be reused
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo/main/m5dial_board.cpp — Current working M5Dial board wrapper
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/design-doc/01-m5dial-film-developer-timer-analysis-design-and-implementation-guide.md — Primary design deliverable
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/reference/01-investigation-diary.md — Chronological research diary

## 2026-03-06

Added ticket-local analysis scripts for `film_dev_times.json`, confirmed the raw dataset is too large and uneven for direct on-device browsing, and narrowed the proposed v1 scope to a curated starter catalog centered on common B/W developers plus limited explicit color-negative / C-41-like support.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json — Source dataset analyzed for size, schema, and category coverage
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/analyze_film_dev_times.py — General dataset summary script
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/scope_subset_report.py — Starter-scope summary script
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/tasks.md — Implementation checklist updated to reflect the scoped plan

## 2026-03-06

Created `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer` by copying the working `0072` timer app, applied a minimal identity rename, and verified the new scaffold still builds cleanly under IDF `5.4.1`.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/CMakeLists.txt — New app project name
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/README.md — Scaffold README updated for the film-developer app
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp — Visible log identity updated for the copied scaffold
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/sdkconfig.defaults — Scaffold defaults retitled for 0073

## 2026-03-06

Step 3: added a generated starter film catalog, wired a runtime FilmCatalog into 0073, and verified catalog initialization on /dev/ttyACM0 (commit eee5fd6)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_catalog.h — New runtime catalog contract
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/generated_film_catalog.cpp — Generated curated recipe dataset
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide/scripts/generate_film_catalog_cpp.py — Generator script for the runtime dataset

## 2026-03-06

Step 4: added selector-oriented catalog query helpers, introduced `RecipeSelectorModel`, and verified on `/dev/ttyACM0` that the new selector layer resolves a concrete starter recipe at boot (commit 453cada)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_catalog.cpp — Added filtered lookup helpers for film, developer, dilution, temperature, and push/pull queries
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/recipe_selector_model.cpp — New selector state machine for staged recipe selection
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp — Boot-time selector initialization and validation logging

## 2026-03-06

Step 5: replaced the inherited timer UI loop with a dedicated film-selector screen/controller pair, flashed the new selector build to `/dev/ttyACM0`, and confirmed the app boots into the selector path (commit dd86850)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_selector_screen.cpp — New selector-focused round-display UI
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/film_selector_controller.cpp — Event mapping from encoder/button/swipe input into selector actions
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer/main/app_main.cpp — UI task now runs the film selector instead of the inherited countdown screen
