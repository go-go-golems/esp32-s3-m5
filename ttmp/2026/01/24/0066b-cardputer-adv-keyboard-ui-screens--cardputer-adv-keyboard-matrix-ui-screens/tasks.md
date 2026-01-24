# Tasks

## TODO


- [x] Import UI mock sources (/tmp -> ticket sources/)
- [x] Write keyboard matrix setup analysis doc
- [x] Write per-screen UI design doc
- [x] Add cardputer_kb to 0066 build and implement input_keyboard module
- [x] Implement UI state machine + overlays in 0066 sim_ui
- [x] Implement on-device Presets + JS Lab (examples-only)
- [x] Expand 0066 web UI into multi-screen layout
- [x] Add ticket scripts: build/flash + smoke instructions
- [ ] Build/flash/validate on /dev/ttyACM0
- [x] Fix 0066 keyboard: use Cardputer-ADV TCA8418 instead of matrix scanner
- [x] Fix web UI parse error in /assets/app.js; serve /favicon.ico
- [x] Fix boot crash: avoid I2C driver_ng conflict (use legacy i2c driver for ADV keyboard)
- [x] Web UI: stop polling from clobbering in-progress edits; add trailing-slash aliases for /api/js/* and /ws
- [ ] Reflash/verify web endpoints: /api/js/eval and /ws no longer 404
