# Changelog

## 2026-01-24

- Initial workspace created
- Added a full UI analysis “textbook” + running diary

## 2026-01-24

Step 15: migrate tutorial 0066 to cardputer_kb::UnifiedScanner (commit fd2a01f)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp — UnifiedScanner snapshot+edge keyboard task


## 2026-01-24

Step 16: make UnifiedScanner build cleanly on ESP-IDF 5.4.1 (commits e9ce104, 4b4c6d6, 35e6f35)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/scanner.h — Custom deleter for pimpl
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/cardputer_kb/tca8418.h — extern C wrappers for C driver
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/cardputer_kb/unified_scanner.cpp — Fix gpio_num_t casts

