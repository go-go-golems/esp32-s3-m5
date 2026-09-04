# Tasks

## TODO

- [x] Phase 0: ticket, sources import, design guide, diary, reMarkable upload <!-- t:ap0o -->
- [ ] Phase 1: create packages/pbui-handheld (pure shell reducer, line model, keymap, frame/dumpText) + browser harness; port the grace-period world verbatim; manual tutorials as screen-dump goldens <!-- t:txzl -->
- [ ] Phase 2: HandheldProduct contract (catalog, lines, slots, verbKeys), defineHandheldPresentation + handheldDiagnostics; demo presentation over the real kernel; workbench-core deck adapter; esbuild IIFE bundle; qjs golden run; size/speed report <!-- t:0l75 -->
- [ ] Phase 3: refusal on the doc line, help-kernel ? overlay and peek, PerformEnvelope transcript in the listener tile <!-- t:d8gc -->
- [ ] Phase 4: firmware 0104-esp32-p4-pbui-handheld skeleton + components/pbui_host (event queue, key/tick jobs, /key /dump console); replay goldens over the console <!-- t:ze5p -->
- [ ] Phase 5: components/pbui_rows renderer (8x16 font page with pbui glyphs, tone bars, caret/lit/chips, two DMA row buffers); measure caret <50 ms and accept lighting <100 ms <!-- t:iw26 -->
- [ ] Phase 6: quasimodes and timers on device (peek via pressed/released, blink, toast expiry, transport playback, tray strip, overview) <!-- t:qoc6 -->
- [ ] Phase 7: 6x10 font page and 53x32 geometry; goldens at both geometries <!-- t:ro3s -->
- [ ] Phase 8: real product - <app>/<process> from picoos_core and <file> from the SD card as a second HandheldProduct <!-- t:fdng -->
- [ ] Phase 9 (design only): keyboard grammar for the link kernel (multi-slot accept, wire a.out -> b.in) <!-- t:t2pc -->
- [ ] Open question: confirm board flash size (16 vs 32 MB) before the 0104 partition table <!-- t:atb8 -->
- [ ] Open question: enumerate ES2021+ usages in the kernel against the vendored QuickJS version <!-- t:6g6h -->
- [ ] Open question: rename tray 'drop' to 'untray' before habits form <!-- t:5na4 -->
- [ ] Native C++ path Phase 0: scaffold 0104 plus host CMake harness and key/dump smoke test <!-- t:m5c6 -->
