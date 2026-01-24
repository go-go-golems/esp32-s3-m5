# Tasks

## TODO

- [x] Create docmgr ticket `G10-ANALYZE-DEMO-UI`
- [x] Map key source files for both demos
- [x] Analyze `M5Cardputer-UserDemo` UI stack (Mooncake v0.x + Smooth_Menu)
- [x] Analyze `M5Cardputer-UserDemo-ADV` UI stack (Mooncake v2 abilities + smooth_ui_toolkit)
- [x] Write Norvig-style textbook doc in ticket
- [x] Maintain a detailed analysis diary

## Implementation TODO (Keyboard Unification)

- [x] Define one reusable keyboard scanner API in `components/cardputer_kb` that can support both Cardputer and Cardputer-ADV hardware
- [x] Add a TCA8418-backed scanner (event FIFO + picture-space remap) to `components/cardputer_kb`
- [x] Add an auto-detect wrapper that picks TCA8418 vs GPIO matrix scanning at runtime
- [x] Migrate a representative firmware (start with `0025-cardputer-lvgl-demo`) to use the unified scanner
- [x] Migrate remaining local consumers in this repo that directly use `MatrixScanner`
- [x] Update `components/cardputer_kb` docs (README/REFERENCE/TUTORIAL) to point to the unified component
- [x] Commit changes and record steps in diary
- [x] Migrate tutorial 0066 to UnifiedScanner (autodetect Cardputer vs Cardputer-ADV; remove local TCA driver)
- [x] Overwrite reMarkable PDF bundle with updated 0066 content
