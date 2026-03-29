# Tasks

## Execution Plan

### 1. Project setup

- [x] Create new firmware project `0083-cardputer-adv-animation-ui`
- [x] Add `CMakeLists.txt`, `main/CMakeLists.txt`, `sdkconfig.defaults`, `partitions.csv`, and `build.sh`
- [x] Wire vendor ADV display components and `cardputer_kb` component dependencies
- [x] Confirm the project builds cleanly before feature work

### 2. Input layer

- [x] Reuse or adapt the ADV-capable semantic keyboard event layer from `0066`
- [x] Normalize left/right/up/down/back/tab/enter/text input for the new app
- [x] Add lightweight debug logging for backend detection and last semantic event

### 3. UI model and animation

- [x] Implement `UiState`, `ScrollModel`, and app modes
- [x] Implement target-based animated scrolling with easing and snap threshold
- [x] Add step-size policy for normal, Fn, Alt, and Ctrl navigation
- [x] Add optional autoplay / demo motion mode

### 4. Rendering

- [x] Implement full-screen `M5Canvas` rendering path
- [x] Draw title/status chrome
- [x] Draw minimap bars derived from scroll position
- [x] Draw scrollbar / indicator derived from the same normalized position
- [x] Draw synthetic content viewport that visibly moves as scroll animates
- [x] Add help / shortcuts overlay

### 5. Device workflow

- [x] Build and flash the firmware with tmux-based `idf.py flash monitor`
- [ ] Validate keyboard control on-device
- [ ] Validate animated scroll motion on-device
- [ ] Fix any hardware or rendering regressions found during device testing

### 6. Documentation and delivery

- [ ] Update the ticket design doc to reflect implementation choices if they differ from the initial plan
- [x] Keep the investigation diary updated with step-by-step implementation notes and commit hashes
- [x] Update changelog and related files as work lands
- [ ] Re-upload the updated ticket bundle to reMarkable after implementation work completes

## Completed Research

- [x] Import `retro_macos_line_minimap.html` into the ticket `imports/` folder
- [x] Analyze existing Cardputer and Cardputer ADV firmwares for UI, keyboard, and scroll control patterns
- [x] Write an intern-focused analysis/design/implementation guide for a new Cardputer ADV animation UI
- [x] Maintain the initial investigation diary for the design phase
- [x] Upload the initial research bundle to reMarkable
