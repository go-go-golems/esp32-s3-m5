# Tasks

## Completed

- [x] Map M5Dial hardware and tutorial reference files
- [x] Relate key files and update changelog
- [x] Write intern-focused M5Dial timer design and implementation guide
- [x] Validate ticket and upload bundle to reMarkable

## Next Implementation Steps

- [x] Decide the graphics stack for the new tutorial: direct `LovyanGFX` board wrapper or an `M5GFX`-style integration that matches the rest of `esp32-s3-m5`
- [x] Create `esp32-s3-m5/0072-m5dial-timer-demo/` with root `CMakeLists.txt`, `README.md`, `sdkconfig.defaults`, `partitions.csv`, and `main/CMakeLists.txt`
- [x] Point the new tutorial at the chosen vendored graphics component with explicit `EXTRA_COMPONENT_DIRS`
- [x] Copy and document the M5Dial hardware constants for power hold, encoder pins, touch I2C pins, buzzer pin, and display pins
- [x] Implement `main/m5dial_board.h` and `main/m5dial_board.cpp` to own board bring-up and normalized hardware access
- [x] Implement power-hold initialization first in the board layer so the device remains on during boot
- [ ] Implement display initialization for the 240x240 round panel and verify backlight control works
- [ ] Implement touch-controller initialization for the FT3267 and verify raw touch coordinates are readable
- [x] Implement encoder setup and center-button input handling with a normalized `take_delta()` and `take_press()` API
- [ ] Decide whether sound is included in v1; if yes, implement a minimal buzzer API and completion/start/pause beep patterns
- [ ] Implement `main/lvgl_port_m5dial.h` and `main/lvgl_port_m5dial.cpp` based on the `0025-cardputer-lvgl-demo` display flush and tick pattern
- [ ] Register an LVGL encoder input device for rotary navigation and press events
- [ ] Decide whether touch is part of v1 UI interaction; if yes, register an LVGL pointer input device and map screen coordinates correctly
- [ ] Implement `main/timer_model.h` and `main/timer_model.cpp` as a small non-blocking countdown engine with `idle`, `running`, `paused`, and `complete` states
- [ ] Reuse the `esp_timer_get_time()`-based timing model from `0071` instead of any blocking countdown logic
- [ ] Implement `main/ui_timer_screen.h` and `main/ui_timer_screen.cpp` with a large central timer label, progress arc, status label, and minimal action controls
- [ ] Choose the v1 visual palette and typography so the UI feels intentional on the round screen rather than like a rectangular layout cropped into a circle
- [ ] Implement `main/timer_controller.h` and `main/timer_controller.cpp` to map encoder and touch actions into timer-model actions
- [ ] Decide the encoder adjustment step size, for example `5s` per detent at short durations and a larger step at long durations
- [ ] Implement `app_main.cpp` to wire board init, LVGL init, timer model, timer screen, and the main event loop together
- [ ] Keep LVGL ownership in one task only and ensure no background task mutates LVGL objects directly
- [x] Build the project with `idf.py build` using `esp32-s3-m5/.envrc`
- [ ] Flash the project to real M5Dial hardware and verify boot, display, encoder, press, touch, and reset behavior
- [ ] Verify there are no runtime regressions such as I2C driver conflicts or LEDC timer clock conflicts
- [ ] Measure binary size and confirm the tutorial still fits the configured app partition with reasonable headroom
- [ ] Capture one or more screenshots or photos of the running timer UI for the tutorial README and ticket
- [ ] Update the ticket design doc with implementation deltas if the actual code diverges from the proposed architecture
- [ ] Add a short operator-facing README section with exact build, flash, and monitor commands for the new tutorial
- [ ] Upload the implementation update and screenshots to the same ticket once the firmware exists
