# Changelog

## 2026-01-02

- Initial ticket workspace created (analysis/design pending).


## 2026-01-01

Step 1: created 0023 scancode calibrator scaffold + raw matrix scanner + live matrix UI (commit 1fc1f77).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/app_main.cpp — UI showing pressed keynums
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/kb_scan.cpp — Scanner + autodetect


## 2026-01-01

Step 2: flashed calibrator + established tmux monitor sniff loop; extracted reusable cardputer_kb scanner component (commit cdb68fa).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/app_main.cpp — Consumer UI using the component
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/cardputer_kb/matrix_scanner.cpp — New shared scanner

