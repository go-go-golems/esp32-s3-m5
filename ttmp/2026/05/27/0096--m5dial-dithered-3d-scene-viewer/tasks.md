# Tasks

## TODO

- [ ] Add tasks here

- [x] Phase 1: Create project structure (CMakeLists.txt, sdkconfig.defaults, partitions.csv) and copy M5DialBoard driver from 0074
- [x] Phase 1: Implement framebuffer.h/c — 2-bit packed framebuffer with fb_set/fb_get/expand_scanline
- [ ] Phase 1: Static test pattern renderer — gradient or Bayer matrix, verify display pipeline end-to-end
- [x] Phase 2: Implement fixedpoint.h and trig_lut.h — fixed-point macros and sin/cos lookup table
- [ ] Phase 2: Implement renderer.h/c — vertex transform, projection, camera orbit
- [ ] Phase 2: Scanline triangle rasterizer with Z-buffer — render a single colored triangle
- [ ] Phase 3: Bayer 4×4 dithering + 4-color quantization integrated into rasterizer
- [x] Phase 3: Palette system — 5 palettes, button/console palette switching
- [ ] Phase 4: Scene terrain — 20×20 height grid, blue gradient, red sun circle
- [ ] Phase 4: Scene torus — torus knot geometry, red↔blue vertex colors, dual-axis rotation
- [ ] Phase 4: Scene ocean — 16×16 wave grid with per-frame sin displacement, sun reflection
- [ ] Phase 4: Scene planet — icosphere with noise displacement, moon orbit, ring
- [ ] Phase 4: Scene tunnel — 12 flying torus rings with 4 support bars
- [x] Phase 5: Encoder/button input — camera orbit, palette cycle, auto-rotate toggle
- [x] Phase 5: esp_console commands — scene, palette, rotate, contrast, aperture, fps, stats, wireframe, pause
- [x] Phase 5: Circular mask, contrast S-curve, performance tuning, FPS counter
