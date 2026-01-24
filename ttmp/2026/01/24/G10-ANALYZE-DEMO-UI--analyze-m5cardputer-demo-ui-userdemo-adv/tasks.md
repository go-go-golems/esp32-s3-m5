# Tasks

## TODO

- [x] Create docmgr ticket `G10-ANALYZE-DEMO-UI`
- [x] Map key source files for both demos
- [x] Analyze `M5Cardputer-UserDemo` UI stack (Mooncake v0.x + Smooth_Menu)
- [x] Analyze `M5Cardputer-UserDemo-ADV` UI stack (Mooncake v2 abilities + smooth_ui_toolkit)
- [x] Write Norvig-style textbook doc in ticket
- [x] Maintain a detailed analysis diary

## Implementation TODO (Keyboard Unification)

- [ ] Define one reusable keyboard scanner API in `components/cardputer_kb` that can support both Cardputer and Cardputer-ADV hardware
- [ ] Add a TCA8418-backed scanner (event FIFO + picture-space remap) to `components/cardputer_kb`
- [ ] Add an auto-detect wrapper that picks TCA8418 vs GPIO matrix scanning at runtime
- [ ] Migrate a representative firmware (start with `0025-cardputer-lvgl-demo`) to use the unified scanner
- [ ] Migrate remaining local consumers in this repo that directly use `MatrixScanner`
- [ ] Update `components/cardputer_kb` docs (README/REFERENCE/TUTORIAL) to point to the unified component
- [ ] Commit changes and record steps in diary
