# Changelog

## 2026-03-21

- Initial workspace created
- Added ticket `ESP-33-PAPERS3-ALPHABET-GRAFFITI`
- Created new tutorial project `0077-papers3-alphabet-graffiti`
- Reused donor PaperS3 component wiring and copied the Protractor math module as a starting point
- Added a buildable placeholder UI that already exposes `TRAIN` and `WRITE` mode framing
- Recorded initial commit `99512ac` for the scaffolded app/task boundary
- Replaced the placeholder runtime with a real alphabet-training UI built around 36 persistent glyph templates
- Added `glyph_store.{h,cpp}` and a `storage` SPIFFS partition to persist trained templates at `/spiffs/glyph_templates.txt`
- Added page-based glyph selection, save/delete/reload actions, and recognition preview against all recorded templates
- Fixed two build integration issues during Task 2:
  - declared `M5Unified` as a `REQUIRES` dependency in `main/CMakeLists.txt`
  - normalized `std::clamp` inputs in `ChangePage()` to `int32_t`
- Verified the training/storage milestone with `idf.py build` against ESP-IDF `5.3.4`
- Recorded Task 2 commit `b04bd3e` for the persistent training milestone
- Implemented live `WRITE` mode with:
  - a writing buffer card
  - auto-append on strong recognition matches
  - `SPACE`, `BACKSPACE`, `CLEAR TEXT`, and `CLEAR STROKE` actions
- Reused the same Protractor recognition path for both training and live text entry instead of forking the gesture pipeline
- Verified the full two-mode app again with `idf.py build` against ESP-IDF `5.3.4`
- Recorded Task 3 commit `625dba3` for the live graffiti writing milestone
