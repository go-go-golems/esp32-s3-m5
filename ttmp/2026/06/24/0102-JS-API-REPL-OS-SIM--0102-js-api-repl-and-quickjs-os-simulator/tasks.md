# Tasks

## TODO

- [x] Restore or install a usable desktop `qjs` for this JS worktree by adding QuickJS as a submodule and building `qjs`.
- [x] Implement Phase 1: portable core utilities and 40-column screen buffer.
- [x] Implement Phase 2: deterministic OS simulation model.
- [x] Implement Phase 3: App/Layout/Panel and base widgets.
- [x] Implement Phase 4: examples and API self-tests.
- [x] Implement Phase 5: bundle/paste workflow for device handoff.
- [x] Add host-only interactive emulator for JS-side examples.
- [x] Add C++ native QuickJS host prototype with firmware-portable API layer split from host terminal glue.
- [x] Add deterministic native host QuickJS value cleanup and native smoke runner.
- [x] Add native layout binding and layout-native example.

## Done

- [x] Create docmgr ticket for the JS API REPL and OS simulator.
- [x] Read the JS README and current smoke-test files.
- [x] Attempt the smoke test and record the missing-qjs failure.
- [x] Gather firmware integration evidence without editing firmware files.
- [x] Write intern-facing design and implementation guide.
- [x] Write investigation diary.
- [x] Fix `run-smoke.sh` so it uses QuickJS `-I` to preload `host-shim.js`.
