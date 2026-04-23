# Tasks

## Done

- [x] Create the Tab5 boot logo ticket workspace and initial design guide.
- [x] Scaffold `esp32-s3-m5/0051-tab5-boot-logo` from the Tab5 web text echo firmware.
- [x] Integrate the M5 logo asset, copied BSP component, LVGL port, and a larger app partition so the firmware builds.
- [x] Build and flash the tutorial firmware to the Tab5.
- [x] Capture the runtime failure and identify the current hang point in the DSI panel read path.
- [x] Build the original `M5Tab5-UserDemo/platforms/tab5` reference firmware as an environment sanity check.
- [x] Write a detailed intern-friendly bug report and bring-up failure analysis in the ticket.
- [x] Patch `display_app.c` to perform board preparation and switch from the custom low-level ST7123 path to the BSP display wrapper path.
- [x] Re-test and confirm that the display init hang is gone and the firmware reaches the final `ready` state.
- [x] Add ticket-local numbered scripts in `scripts/` for flashing, log capture, config comparison, and PSRAM tuning.
- [x] Enable the 200 MHz PSRAM path required for better display throughput and confirm it in runtime logs.
- [x] Write a detailed resolution report for the repaired display failure.
- [x] Upload the updated ticket bundle to reMarkable.

## Next

- [ ] Confirm with fresh human visual inspection whether the fluttering edge artifact is fully gone after the 200 MHz PSRAM build.
- [ ] If any residual artifact remains, compare additional factory DSI/LVGL runtime config in small controlled steps.
- [ ] Keep the diary, changelog, and design docs in sync with the next visual verification pass.
