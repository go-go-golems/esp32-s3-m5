# Tasks

## TODO

- [x] Add tasks here (placeholder)

- [x] Write setup/architecture analysis doc for typewriter app (ground truth links + implementation checklist)
- [x] Create new chapter project: esp32-s3-m5/0012-cardputer-typewriter (M5GFX autodetect + keyboard scan)
- [x] Implement typewriter UI: keypress -> text buffer -> redraw on canvas; scrolling + backspace + enter
- [x] Validate: idf.py build + flash via /dev/serial/by-id; record results
- [x] Ticket bookkeeping: relate key files, update diary/changelog, commit docs
- [x] Decide chapter folder name/number (0012) and copy scaffold patterns from 0011 (M5GFX EXTRA_COMPONENT_DIRS, full-screen M5Canvas)
- [x] Create esp32-s3-m5/0012-cardputer-typewriter skeleton: CMakeLists, main/CMakeLists, main/Kconfig.projbuild, sdkconfig.defaults, partitions.csv, dependencies.lock, README
- [x] Display bring-up: M5GFX autodetect + full-screen sprite/canvas; add a solid-color bring-up sequence (reuse 0011)
- [x] Keyboard input: port the 0007 matrix scan (GPIO config + scan loop + debounce + optional IN0/IN1 autodetect) into 0012 (rename configs to CONFIG_TUTORIAL_0012_*)
- [x] Keyboard mapping: map key positions to tokens (printable char, space, tab, enter, del/backspace, modifiers). Decide minimal handling for shift/caps/ctrl/alt
- [x] Typewriter buffer model: choose data structure (ring of lines) + cursor invariants; implement insert char, enter/newline, backspace (including join-with-previous-line)
- [x] Rendering MVP: redraw full screen on each edit event using canvas cursor APIs (setFont/setTextColor/setCursor/print/printf). Compute line height, max rows, and clamp/render last N lines
- [x] Scrolling behavior: when buffer exceeds visible rows, keep last N lines visible. Optional: implement textScroll/baseColor pattern like vendor text editor
- [x] Cursor UX: draw a visible cursor (e.g., invert block/underscore) and ensure it updates on each edit
- [x] Performance: only repaint on actual key events; add optional heartbeat/logging every N events; keep DMA wait option (CONFIG_TUTORIAL_0012_PRESENT_WAIT_DMA)
- [x] Validation: idf.py build; flash/monitor using stable by-id path + conservative baud (document in README). Verify typing/backspace/enter/scroll on device
- [x] Docs workflow rule for this ticket: after each implementation task, write a diary step (with commands + outcomes) and commit code separately from docs
- [x] Ticket bookkeeping at end: doc relate new 0012 files, update changelog with commit hashes, check tasks complete, and commit docs changes
