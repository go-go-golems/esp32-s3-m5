# Changelog

## 2025-12-23

- Created `esp32-s3-m5/0012-cardputer-typewriter/` chapter scaffold and verified `idf.py build` (commit `2110b78c349256bb7769ee2e30dfa696e5745c8e`).
- Ported Cardputer keyboard scan (from tutorial `0007`) into `0012` and added an on-screen “last key token” UI (commit `3c34342f540b6c11dd6a434f57458d0af3d744a1`).
- Implemented MVP typewriter behavior in `0012`: token → buffer edits (enter/backspace) + redraw last N lines with underscore cursor (commit `1995080b24a3ccbc7c3d43a965127aaa82dc2cdc`).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp — Display bring-up scaffold (M5GFX autodetect + canvas)
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--cardputer-typewriter-keyboard-on-screen-text/reference/01-diary.md — Diary Steps 2–4 (commit + build notes)

## 2025-12-22

- Initial workspace created


## 2025-12-22

Created setup/architecture analysis doc and diary Step 1; related key reference files (from 005/0007/0011/vendor).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--cardputer-typewriter-keyboard-on-screen-text/analysis/01-setup-architecture-cardputer-typewriter-keyboard-on-screen-text.md — Architecture + implementation checklist
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--cardputer-typewriter-keyboard-on-screen-text/reference/01-diary.md — Diary Step 1


## 2025-12-22

Expanded ticket task breakdown for upcoming typewriter implementation session.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--cardputer-typewriter-keyboard-on-screen-text/tasks.md — Added detailed implementation tasks


## 2025-12-22

Validated 0012 typewriter on hardware (flash via /dev/serial/by-id) and closed ticket; note: idf.py monitor requires a TTY in this environment.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--cardputer-typewriter-keyboard-on-screen-text/reference/01-diary.md — Diary Step 5 validation notes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp — Typewriter MVP implementation (commit 1995080...)


## 2025-12-22

Closed: 0012 typewriter MVP complete and validated on device.


## 2026-01-02

Closing: all tasks complete; ready for reference/use as baseline.

