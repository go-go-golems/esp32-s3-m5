# Tasks

## TODO

- [ ] Collect official ESP-IDF docs links: host apps (linux target), linux host testing, and QEMU; add to analysis doc External sources section
- [ ] Decide host iteration strategy for typewriter: ESP-IDF linux target vs plain CMake host runner; document decision + rationale
- [ ] Define typewriter_core API boundary: KeyEvent/Token model + text buffer/cursor invariants; ensure it has zero ESP-IDF/M5GFX deps
- [ ] Create host runner UX plan: stdin raw mode/ncurses mapping for Cardputer key labels; decide display model (terminal grid)

## Buildable deliverables (when this ticket is “done enough”)

- [ ] Create `typewriter_core` as a small portable library with **no ESP-IDF/M5GFX includes** (C or C++), plus host tests
- [ ] Create a minimal `typewriter_host` runner (Linux) that drives `typewriter_core` and renders a terminal UI (ncurses or simple grid)
- [ ] Create / update a device adapter that maps Cardputer keyboard scan → `typewriter_core` tokens and renders via M5GFX
- [ ] Write a playbook: “fast host iteration loop” (build/run/test) + “device validation loop” (flash/monitor)

## Documentation hygiene

- [ ] Maintain `reference/01-diary.md` with step-by-step entries as we work
- [ ] Update `changelog.md` for meaningful milestones/decisions (especially the chosen host strategy)
