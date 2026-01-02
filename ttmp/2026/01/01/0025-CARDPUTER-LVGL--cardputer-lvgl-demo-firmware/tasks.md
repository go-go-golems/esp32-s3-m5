# Tasks

## TODO


- [x] Scaffold ESP-IDF project 0025-cardputer-lvgl-demo (CMakeLists, sdkconfig.defaults, partitions, build script)
- [x] Add LVGL dependency (ESP-IDF Component Manager) and tune LVGL Kconfig defaults (disable examples, set LVGL heap size)
- [x] Implement LVGL display port for M5GFX (draw buffers + flush callback + tick source + handler loop)
- [x] Implement LVGL keypad indev adapter backed by CardputerKeyboard events (nav + text entry)
- [x] Create initial LVGL demo screen (title + textarea + Clear button + small status label) and wire focus/navigation
- [x] Document build/flash usage and key mapping in the project README; add quick validation steps
- [x] Build+flash smoke test; record results (and any failures) in the diary
- [x] Add LVGL menu screen to choose demos (Baselines + Pomodoro) and allow returning to menu
- [x] Add Pomodoro demo screen (arc ring + big time + keyboard controls) as a selectable demo
- [x] Update README with menu navigation + Pomodoro controls; verify build compiles
