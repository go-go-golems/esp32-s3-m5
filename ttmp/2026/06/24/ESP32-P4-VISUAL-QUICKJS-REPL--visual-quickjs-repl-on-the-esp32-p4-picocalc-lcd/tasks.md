# Tasks

This task list is implementation-grade. Work top to bottom. Commit after each checkpoint that leaves the tree buildable, the docs materially clearer, or hardware behavior validated.

## Phase 0 — Ticket, evidence, and initial deliverables

- [x] **T0.1 — Create docmgr ticket.** Create `ESP32-P4-VISUAL-QUICKJS-REPL` with topics `esp32p4`, `quickjs`, `javascript`, `firmware`, `lcd`, `repl`, `picocalc`.
- [x] **T0.2 — Create primary design guide.** Write `design-doc/01-visual-quickjs-repl-analysis-design-and-implementation-guide.md` with architecture, APIs, pseudocode, decision records, phases, tests, risks, and file references.
- [x] **T0.3 — Create diary.** Add `reference/01-investigation-diary.md` and record initial evidence/design work.
- [x] **T0.4 — Relate evidence files.** Use `docmgr doc relate` for 0099 LCD/keyboard, 0101 QuickJS service, and the new docs.
- [x] **T0.5 — Update changelog.** Record ticket setup and design guide creation.
- [x] **T0.6 — Run `docmgr doctor`.** Validate the initial ticket.
- [x] **T0.7 — Upload initial design bundle to reMarkable.** Upload index, design guide, diary, tasks, and changelog.
- [x] **T0.8 — Commit initial ticket docs.** Commit focused ticket/doc changes.

## Phase 1 — Extract reusable PicoCalc hardware components

- [ ] **T1.1 — Create `components/picocalc_keyboard`.** Move/copy the proven 0099 keyboard module into a reusable component.
- [ ] **T1.2 — Create `components/picocalc_lcd`.** Extract the minimal LCD panel/SPI/rect/row primitives from 0099.
- [ ] **T1.3 — Preserve hardware constants.** Keep the tested PicoCalc GPIO mapping, 80 MHz SPLL SPI source, 32 KiB max transfer size, RGB565, and panel init sequence.
- [ ] **T1.4 — Add component READMEs.** Document source path, pin mapping, public API, and known performance constraints.
- [ ] **T1.5 — Build-test extraction.** Build through a minimal firmware or 0102 skeleton.
- [ ] **T1.6 — Diary/changelog/commit.** Record extraction decisions, failures, and validation.

## Phase 2 — Create `0102-esp32-p4-visual-quickjs-repl` skeleton

- [ ] **T2.1 — Create firmware directory.** Add top-level `CMakeLists.txt`, `README.md`, `sdkconfig.defaults`, optional `partitions.csv`, and `main/`.
- [ ] **T2.2 — Wire components.** Add `EXTRA_COMPONENT_DIRS` for `quickjs_native`, `qjs_service`, `picocalc_lcd`, `picocalc_keyboard`, and later `visual_repl`.
- [ ] **T2.3 — Add app startup.** Initialize LCD, keyboard, QuickJS service, and optional UART debug console.
- [ ] **T2.4 — Build for `esp32p4`.** Use ESP-IDF 5.4.2 and confirm binary size.
- [ ] **T2.5 — Hardware smoke.** Flash and verify boot logs for LCD/keyboard/QuickJS ready.
- [ ] **T2.6 — Diary/changelog/commit.** Record skeleton validation.

## Phase 3 — Implement text renderer and static visual screen

- [ ] **T3.1 — Add `components/visual_repl` or app-local visual REPL module.** Define model, renderer, palette, and event interfaces.
- [ ] **T3.2 — Add bitmap font.** Use a small readable monospace font suitable for 8×16 cells.
- [ ] **T3.3 — Implement row renderer.** Convert text + style into RGB565 row buffers.
- [ ] **T3.4 — Implement static screen demo.** Draw banner, prompt row, output row, error row, and status row.
- [ ] **T3.5 — Measure row/full redraw.** Log row repaint and full viewport redraw time.
- [ ] **T3.6 — Hardware visual check.** Confirm colors and text are readable on LCD.
- [ ] **T3.7 — Diary/changelog/commit.** Record renderer decisions and measurements.

## Phase 4 — Keyboard input and line editing

- [ ] **T4.1 — Add keyboard polling task.** Poll `picocalc_keyboard_poll_event()` and publish semantic key events.
- [ ] **T4.2 — Implement key translation.** Map printable ASCII, Enter, Backspace, arrows, PageUp/PageDown, Home/End, Escape.
- [ ] **T4.3 — Implement input buffer.** Support insert, backspace, cursor left/right, home/end.
- [ ] **T4.4 — Render current input row and cursor.** Update dirty row(s) on key events.
- [ ] **T4.5 — Append input records without eval.** Pressing Enter should move the input line into scrollback before QuickJS integration.
- [ ] **T4.6 — Hardware input smoke.** Type text, backspace, move cursor, submit line.
- [ ] **T4.7 — Diary/changelog/commit.** Record keyboard mapping gaps and validation.

## Phase 5 — Connect visual input to QuickJS eval

- [ ] **T5.1 — Submit input to `qjs_service_eval`.** Use 1000 ms default timeout and `<lcd-repl>` filename.
- [ ] **T5.2 — Render output records.** Append captured output as green/normal output.
- [ ] **T5.3 — Render error records.** Append exceptions/service errors/timeouts as red error/status rows.
- [ ] **T5.4 — Add visual reset path.** Implement `/reset` or a function key that calls `qjs_service_reset`.
- [ ] **T5.5 — Add visual status path.** Implement `/status` or a function key that appends QuickJS/heap status.
- [ ] **T5.6 — Hardware eval smoke.** Validate `print(1+2)`, exception, timeout, reset-global behavior.
- [ ] **T5.7 — Diary/changelog/commit.** Record eval integration evidence.

## Phase 6 — Scrollback and viewport navigation

- [ ] **T6.1 — Implement bounded scrollback ring.** Store styled logical records with sequence IDs.
- [ ] **T6.2 — Implement wrapping.** Convert records into 40-column physical rows for the 8×16 default geometry.
- [ ] **T6.3 — Implement viewport offset.** Track bottom offset and visible row selection.
- [ ] **T6.4 — Implement PageUp/PageDown.** Navigate scrollback with PicoCalc keys.
- [ ] **T6.5 — Implement auto-scroll policy.** New output should keep bottom visible unless the user intentionally scrolled away.
- [ ] **T6.6 — Hardware scrollback smoke.** Generate more than one page of output and scroll through it.
- [ ] **T6.7 — Diary/changelog/commit.** Record scrollback behavior and any performance limits.

## Phase 7 — Polish, measurement, and final handoff

- [ ] **T7.1 — Add debug/status commands if useful.** UART `ui status` or `lcd repl perf` can remain for development.
- [ ] **T7.2 — Measure visual REPL performance.** Boot-to-ready, row repaint, full redraw, eval-to-visible-output, 100-line output, heap before/after.
- [ ] **T7.3 — Update design guide with implementation outcomes.** Record deviations and measured results.
- [ ] **T7.4 — Run `docmgr doctor --ticket ESP32-P4-VISUAL-QUICKJS-REPL --stale-after 30`.** Fix warnings.
- [ ] **T7.5 — Upload final bundle to reMarkable.** Force refresh if final docs changed after initial upload.
- [ ] **T7.6 — Final handoff commit.** Commit final docs and report validation/upload destination.

## Optional Phase 8 — Product features

- [ ] **T8.1 — Command history navigation.** Up/Down recall previous input lines.
- [ ] **T8.2 — Multi-line editing.** Allow longer scripts before eval.
- [ ] **T8.3 — Syntax highlighting.** Color JavaScript tokens in the input/editor area.
- [ ] **T8.4 — ANSI subset parser.** Convert selected escape codes into styled records.
- [ ] **T8.5 — Persistent scripts.** Add embedded examples or filesystem-backed snippets.
- [ ] **T8.6 — JavaScript display APIs.** Expose safe drawing primitives to QuickJS through `qjs_service_run` jobs.
