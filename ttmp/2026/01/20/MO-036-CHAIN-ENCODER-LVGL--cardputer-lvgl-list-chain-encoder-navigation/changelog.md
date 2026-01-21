# Changelog

## 2026-01-20

- Initial workspace created


## 2026-01-20

Added textbook-style design doc: Chain Encoder UART protocol, Index_id hop-count semantics, and LVGL list navigation integration approaches; updated diary with step-by-step findings.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-036-CHAIN-ENCODER-LVGL--cardputer-lvgl-list-chain-encoder-navigation/design-doc/01-lvgl-lists-chain-encoder-cardputer-adv.md — Primary analysis and implementation plan.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-036-CHAIN-ENCODER-LVGL--cardputer-lvgl-list-chain-encoder-navigation/reference/01-diary.md — Work log with commands and evidence links.


## 2026-01-20

Uploaded bundled PDF (design doc + diary) to reMarkable: /ai/2026/01/21/MO-036-CHAIN-ENCODER-LVGL/ — "MO-036 — Chain Encoder + LVGL (Cardputer-ADV)".

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-036-CHAIN-ENCODER-LVGL--cardputer-lvgl-list-chain-encoder-navigation/design-doc/01-lvgl-lists-chain-encoder-cardputer-adv.md — Bundled into uploaded PDF.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-036-CHAIN-ENCODER-LVGL--cardputer-lvgl-list-chain-encoder-navigation/reference/01-diary.md — Bundled into uploaded PDF.


## 2026-01-20

Implemented firmware 0047 (LVGL list + Chain Encoder over Grove UART). Build succeeds; flashing over USB Serial/JTAG was flaky (mid-flash disconnect), needs retry after device re-enumerates.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0047-cardputer-adv-lvgl-chain-encoder-list/main/app_main.cpp — UI + LVGL encoder indev.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0047-cardputer-adv-lvgl-chain-encoder-list/main/chain_encoder_uart.cpp — UART protocol client for U207.

