# Changelog

## 2025-12-23

- Initial workspace created


## 2025-12-23

Created ticket to port AnimatedGIF into ESP-IDF: detailed porting task list, harness plan, and related starting references (009 decision record, AtomS3R display, flash-mmap pattern).


## 2025-12-23

Step 1: Created starting-map analysis + diary; captured repo orientation and key integration points.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/0010-PORT-ANIMATED-GIF--port-animatedgif-bitbank2-into-esp-idf-project-as-a-component/analysis/02-starting-map-files-symbols-and-integration-points.md — Starting map doc
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/0010-PORT-ANIMATED-GIF--port-animatedgif-bitbank2-into-esp-idf-project-as-a-component/reference/01-diary.md — Implementation diary


## 2025-12-23

Step 2: Created standalone AtomS3R harness project (esp32-s3-m5/0014-atoms3r-animatedgif-single) with M5GFX display+canvas present scaffold; no AnimatedGIF integration yet.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp — Harness entrypoint


## 2025-12-23

Step 3: Vendored AnimatedGIF as ESP-IDF component in 0014 harness and implemented single-GIF playback (builds under ESP-IDF 5.4.1). Code commit: 91a3eacd972b2...

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/components/animatedgif/src/AnimatedGIF.h — ESP-IDF millis/delay shims + component deps
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp — GIFDraw + playback loop


## 2025-12-23

Step 4: Fixed harness bug: AnimatedGIF::open() returns 1 on success (not GIF_SUCCESS). Updated logs to use getLastError() only on real failure. Code commit: 65c6cd6cbcfd...

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp — Fix open() return handling and improve playFrame error logging


## 2025-12-23

Step 5: Fixed playFrame EOF semantics (present even when prc==0 if last_err==GIF_SUCCESS) and added yield on reset path to prevent task WDT starvation. Code commit: 515a6cc0a0bd...

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp — Present-on-EOF + yield to avoid WDT


## 2025-12-23

Step 6: Switched harness test asset to multi-frame homer_tiny and added GIFINFO logging to confirm animation; code commit 360f3770cf49...

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/main/assets/homer_tiny.h — Multi-frame test GIF header
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp — Select homer_tiny + log GIFINFO


## 2025-12-23

Step 7: Added Kconfig options to fix RGB565 byte order (swap) and scale GIF to full 128x128 canvas for harness validation; code commit 994d5afbe13a...

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/main/Kconfig.projbuild — GIF render toggles
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp — Scaling + byte-swap in GIFDraw
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/sdkconfig.defaults — Enable swap+scale defaults


## 2025-12-23

Step 8: Enabled AnimatedGIF COOKED mode with allocFrameBuf() to improve disposal/transparency correctness and reduce artifacty 'flutter' look. Code commit d166462dae3b...

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/main/Kconfig.projbuild — CONFIG_TUTORIAL_0014_GIF_USE_COOKED
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0014-atoms3r-animatedgif-single/main/hello_world_main.cpp — COOKED mode setup + GIFDraw cooked scanline path


## 2025-12-23

Step 9: Correction — 'DMA flutter' was a false alarm; kept Step 8 history and reverted COOKED-mode change in harness.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/23/0010-PORT-ANIMATED-GIF--port-animatedgif-bitbank2-into-esp-idf-project-as-a-component/reference/01-diary.md — Append correction entry (keep Step 8


## 2025-12-23

Ticket complete: vendored AnimatedGIF as ESP-IDF component, built standalone harness (0014) proving decode+draw+present on AtomS3R, and documented usage guide + integration diary. Remaining work (console integration, flash asset bundling) moved to ticket 008.


## 2025-12-24

Integrated “Phase B” into `0013`: real GIF playback via AnimatedGIF, selected through `esp_console` + button, with GIF assets loaded from a FATFS `storage` partition. Added reproducible host scripts (`fatfsgen.py` + `parttool.py`) to build and flash a `storage.bin` image and documented the workflow in the diary.

- Added a new playbook capturing the end-to-end “assets → storage.bin → flash → play” workflow and the real-world failure modes we hit (mount/WL mismatch, long filenames, heap exhaustion).

