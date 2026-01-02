# Tasks

## TODO

- [x] Create new ESP-IDF project `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator`
- [x] Implement raw matrix scanner (pins + alt IN0/IN1 autodetect + multi-key support)
- [x] Implement on-device calibration UX (prompt -> capture -> confirm -> next)
- [x] Define and implement “decoder config” output format (JSON + C header snippet)
- [x] Add verbose serial logging (raw scan_state/in_mask, derived key positions, chord timing)
- [x] Add host-side capture helpers (optional): parse JSON output into a ready-to-commit header
- [x] Add build/flash/monitor playbook (reuse the 0022 `build.sh` style)

## Acceptance criteria

- [x] For any pressed key, the UI shows:
  - the currently-pressed matrix coordinates (`x,y`)
  - the “key number” convention (1..56) used in vendor code
  - raw scan_state + in_mask evidence
- [x] For Fn combos, the UI captures the *set of physical keys pressed* (including Fn) and stores it as a “combo binding”.
- [x] Firmware can guide the user through calibrating at least:
  - `Up/Down/Left/Right` (Fn-combos)
  - `Enter`, `Tab`, `Esc`/Back, `Space`
- [x] Firmware prints a single “config blob” to serial that can be pasted into another project to decode keys reliably.

- [x] Extract keyboard matrix scan + legend into reusable ESP-IDF component (cardputer_kb)
