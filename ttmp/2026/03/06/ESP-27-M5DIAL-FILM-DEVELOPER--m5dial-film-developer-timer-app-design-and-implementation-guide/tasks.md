# Tasks

## Completed

- [x] Create the ticket workspace for the M5Dial film developer timer app
- [x] Create the primary design document and investigation diary
- [x] Inspect the current `0072-m5dial-timer-demo` architecture for reuse
- [x] Inspect `film_dev_times.json` to understand schema shape, scale, and data quality
- [x] Save ticket-local analysis scripts in `scripts/`
- [x] Define a realistic v1 scope based on common B/W developers and limited explicit color-negative / C-41-like support
- [x] Write the detailed intern-focused analysis, design, and implementation guide

## Next Implementation Steps

- [x] Create `esp32-s3-m5/0073-m5dial-film-developer-timer/` by copying `0072-m5dial-timer-demo/`
- [x] Rename the copied scaffold enough that it has its own project name, README, and visible log identity
- [x] Verify the copied scaffold still builds under IDF 5.4.1 before film-specific changes begin
- [x] Keep the current M5Dial board layer, LVGL port, and input-event queue architecture intact in the new app
- [x] Add an app-local preprocessing script to build a compact curated catalog from `film_dev_times.json`
- [x] Decide whether the runtime catalog should be generated JSON or generated C/C++ data
- [x] Implement `main/film_catalog.h` and `main/film_catalog.cpp`
- [x] Normalize temperatures so both `{celsius, fahrenheit}` and `{raw}` inputs become one runtime `temp_c`
- [x] Normalize push/pull labels into user-facing display strings while preserving the source value
- [ ] Filter the starter catalog to:
  - [x] common B/W developers
  - [x] explicit color-negative / C-41-like rows that are actually present and usable
- [x] Implement `main/recipe_selector_model.h` and `main/recipe_selector_model.cpp`
- [x] Flash the app to `/dev/ttyACM0` and verify the selector layer resolves a concrete starter recipe at boot
- [ ] Implement `main/film_timer_model.h` and `main/film_timer_model.cpp`
- [ ] Implement `main/film_timer_controller.h` and `main/film_timer_controller.cpp`
- [x] Create a selector UI screen tuned for encoder-driven browsing on the round display
- [x] Replace the inherited generic timer text with film-selection labels and option values
- [x] Wire encoder turns into selector field value changes through the existing event queue
- [x] Wire short press into selector field advance / confirm through the existing event queue
- [x] Wire long press into selector back / cancel behavior through the existing event queue
- [ ] Create a recipe review / ready screen
- [ ] Show the resolved recipe time, film, developer, dilution, temperature, and push/pull on the review screen
- [ ] Create a running timer screen that shows recipe metadata plus countdown
- [ ] Decide what completion behavior happens when the countdown reaches zero
- [ ] Auto-hide or auto-lock single-choice fields such as developer or dilution when appropriate
- [ ] Decide whether v1 should expose film format (`35mm`, `120`, `sheet`) or default to the best available time
- [ ] Smooth out awkward raw push/pull labels such as fractional pull values before the final v1 polish pass
- [ ] Update `README.md` with build, flash, and control instructions for the new app
- [x] Build the new app with `idf.py build`
  Note: the app still boots the inherited timer UI, but the curated film catalog now initializes successfully at runtime with `1548` recipes across `19` films and `9` developers.
- [ ] Flash to `/dev/ttyACM0` and validate on-screen selector behavior on real hardware
- [ ] Confirm a few chosen development times against raw source rows manually
- [ ] Capture photos or screenshots for the eventual ticket update and README
- [ ] Upload the implementation follow-up to this ticket once code exists
