# Tasks

## TODO

- [x] Phase A (MVP): implement **mock “GIFs”** (built-in animations like colored squares) so we can focus on **button + serial console control** first
- [x] Define serial console protocol (commands: `list`, `play <id|name>`, `stop`, `next`, `info`, optional `brightness`) using `esp_console`
- [x] Implement button press → `next` animation (GPIO configurable; debounce; playback task owns display)
- [x] Implement new tutorial chapter (likely `esp32-s3-m5/0013-atoms3r-gif-console/`): display bring-up + serial console + mock animation playback loop
- [ ] Validate on hardware: flash firmware, verify `list/play/stop/next` works and mock animations switch smoothly
- [ ] Phase B (later): real GIF playback + flash-bundled assets (see below)
- [x] Ticket bookkeeping: relate key files, update diary/changelog, commit docs

- [x] Document AssetPool injection details (include mmap + field lengths + consumer length gotchas)
- [x] Decide button GPIO default + make configurable via Kconfig; implement button->next animation event
- [x] Expand esp_console section with concrete command registration + transport config guidance

### Phase B (later): real GIF assets

- [ ] Decide playback strategy: decode GIF on-device vs pre-converted frame pack (storage vs CPU tradeoff)
- [ ] Choose asset storage mechanism (custom `assetpool/animpack` partition vs SPIFFS/FATFS) and size budget
- [ ] Define animation pack directory format (names+offsets) building on AssetPool pattern (if not using FS)
- [ ] Build host pipeline (GIF -> normalized frames -> packed asset) and document exact commands/tools
- [ ] Integrate chosen GIF decoder and wire it into the playback loop (after Phase A is stable)
