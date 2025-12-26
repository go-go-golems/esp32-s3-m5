# Tasks

## Milestone A — Capture the bug (report + reproduction)

- [x] Add crash log snippet + backtrace to the bug report doc
- [x] Confirm which git commit(s) are actually flashed (`idf.py build` version string, `-dirty` state)
- [ ] Reproduce on current `main` HEAD with a clean tree (no `-dirty`)

## Milestone B — Root cause analysis

- [x] Verify root cause by inspecting `locks.c`:
  - `lock_acquire_generic()` aborts when `xPortCanYield()` is false and the lock is recursive (stdio/vprintf)
- [x] Verify we call driver/log/stdio while holding `portENTER_CRITICAL(&s_state_mux)`:
  - `tester_state_init()` calls `apply_config_locked()` under lock
  - `safe_reset_pin()` calls `gpio_reset_pin()` which may log internally
  - `print_status_locked()` calls `printf()` under lock

## Milestone C — Fix

- [x] Refactor `signal_state.cpp` to never call:
  - `gpio_reset_pin()` / `gpio_config()` / `gpio_set_direction()` / etc
  - `ESP_LOG*` / `printf`
  while holding the `s_state_mux` critical section.
- [x] Rebuild `0016`
- [ ] Flash and verify boot proceeds past `tester_state_init()` (hardware)
- [ ] Verify `status` output works from the manual REPL (no abort) (hardware)

## Milestone D — Documentation + bookkeeping

- [ ] Update `009` docs if needed (if behavior/contract changes)
- [ ] Update this ticket diary + changelog with commit hashes and validation notes

