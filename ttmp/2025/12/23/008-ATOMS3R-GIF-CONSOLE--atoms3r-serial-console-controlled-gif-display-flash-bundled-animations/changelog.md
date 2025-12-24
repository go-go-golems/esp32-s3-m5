# Changelog

## 2025-12-23

- Initial workspace created


## 2025-12-23

Created ticket and initial docs: AtomS3R serial-controlled animation architecture + GIF->flash asset pipeline; related AtomS3R display bring-up and AssetPool partition workflow references.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md — Architecture doc
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/reference/01-asset-pipeline-gif-flash-bundled-animation-playback.md — Asset pipeline doc


## 2025-12-23

Expanded docs with non-library research: AssetPool deep dive (mmap + field-addressed assets), esp_console capabilities, and button→next animation control pattern.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/ATOMS3R-CAM-M12-UserDemo/main/usb_webcam_main.cpp — AssetPool mmap injection reference
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/components/esp-protocols/components/console_simple_init/console_simple_init.c — esp_console REPL init helper
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0003-gpio-isr-queue-demo/main/hello_world_main.c — GPIO ISR + queue button pattern


## 2025-12-24

Validated Phase A MVP builds for `esp32-s3-m5/0013-atoms3r-gif-console/` against ESP-IDF 5.4.1 (USB Serial/JTAG console + mock animations + button). Recorded the `set-target esp32s3` `sdkconfig` update in git (commit `a7df43a`) and updated ticket tasks + diary to reflect the now-clean compile.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp — Phase A MVP implementation (console + button + mock playback)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild — Phase A MVP Kconfig knobs (button/backlight/display)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0013-atoms3r-gif-console/sdkconfig — Target-pinned config (commit `a7df43a`)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/tasks.md — Task status updated
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/reference/02-diary.md — Step 5 build/commit diary entry


## 2025-12-24

Documented how to run `esp_console` over AtomS3R’s GROVE port UART (`G1/G2`) in ESP-IDF 5.x and captured the investigation as a diary step (so we don’t re-derive pinout + Kconfig names later).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/playbooks/01-esp-console-repl-usb-serial-jtag-quickstart.md — Added GROVE UART console configuration + wiring notes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/reference/02-diary.md — Added Step 6 (GROVE UART console investigation)


## 2025-12-24

Read ESP-IDF 5.4.1 `esp_console` implementation sources (console core, REPL glue, VFS backends) and wrote a contributor-focused internals guidebook explaining where global state lives and what that implies for multi-REPL designs.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/reference/03-esp-console-internals-guidebook.md — New guidebook (ESP-IDF 5.4.1 esp_console internals)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/reference/02-diary.md — Added Step 7 (esp_console internals deep dive)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/23/008-ATOMS3R-GIF-CONSOLE--atoms3r-serial-console-controlled-gif-display-flash-bundled-animations/scripts/extract_idf_paths_from_compile_commands.py — Script to locate ESP-IDF paths from compile_commands.json

