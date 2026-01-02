# Changelog

## 2026-01-01

- Initial workspace created


## 2026-01-01

Added initial bug report + triage diary; identified createPng(miniz) likely stack-heavy and planned task-based fix

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0026-B3-SCREENSHOT-STACKOVERFLOW--cardputer-demo-screenshot-png-send-triggers-stack-overflow-in-main/analysis/01-bug-report-investigation-stack-overflow-during-createpng-screenshot-send.md — Bug report + investigation notes
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0026-B3-SCREENSHOT-STACKOVERFLOW--cardputer-demo-screenshot-png-send-triggers-stack-overflow-in-main/reference/01-diary.md — Triage diary


## 2026-01-01

Implemented task-based screenshot encode/send and validated host PNG capture

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp — Worker-task based PNG encode/send
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0026-B3-SCREENSHOT-STACKOVERFLOW--cardputer-demo-screenshot-png-send-triggers-stack-overflow-in-main/analysis/01-bug-report-investigation-stack-overflow-during-createpng-screenshot-send.md — Updated with fix+validation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/01/0026-B3-SCREENSHOT-STACKOVERFLOW--cardputer-demo-screenshot-png-send-triggers-stack-overflow-in-main/reference/01-diary.md — Added validation step

