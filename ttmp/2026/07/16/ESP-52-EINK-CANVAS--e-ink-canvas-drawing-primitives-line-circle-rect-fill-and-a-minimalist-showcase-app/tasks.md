# Tasks

## TODO

- [x] Phase 0 - Orientation: 0114 builds, host suite green, guide + prior guides read <!-- t:jzqv -->
- [x] [P1.1] draw_ops.h: DrawOpKind Line/Circle + LinePayload/CirclePayload <!-- t:blub -->
- [x] [P1.2] FrameBuilder Line/Circle/Ring with clipped bbox, drop+count when fully clipped <!-- t:ols4 -->
- [x] [P1.3] Fake backend trace lines for Line/Circle (golden-stable format) <!-- t:0xhh -->
- [x] [P1.4] M5 backend rendering: drawLine (thick fallback), fillCircle/drawCircle rings, clip honored <!-- t:y57p -->
- [x] [P1.5] Host tests: bounds math, payload round-trip, trace golden; suite green <!-- t:ly2u -->
- [x] [P2.1] WidgetKind::Canvas + CanvasCmd store in WidgetArena (8x96), NewCanvas/CanvasAppend/CanvasClear <!-- t:akun -->
- [x] [P2.2] EmitNode Canvas case: frame-relative commands, PushClip(frame), arena-aware signature <!-- t:jc5h -->
- [x] [P2.3] Diff semantics: append bumps version -> canvas-frame damage; host tests + fuzz extension <!-- t:1jes -->
- [x] [P3.1] Stdlib: canvas() factory + line/disc/ring/box/paint/wipe methods; regen atoms + pulpjsc stubs <!-- t:fiwe -->
- [x] [P3.2] Bindings in js_widgets.cpp with kind checks + capacity TypeErrors <!-- t:ai12 -->
- [x] [P3.3] Console probes: canvas render probe + traced op-list probe; hardware transcript <!-- t:qp8y -->
- [ ] [P4.1] Ink app scene 1: analog clock, one blit per minute (transcript evidence) <!-- t:11qz -->
- [ ] [P4.2] Ink app scene 2: generative field, clean-full reveal <!-- t:61na -->
- [ ] [P4.3] Ink app scene 3: gray ladder rings; launcher entry + tap-rotate + swipe-down home <!-- t:3l3b -->
- [ ] [P5.1] Hardening: goldens re-pinned, 30-min clock soak (heap flat), diary + changelog, doctor clean <!-- t:a1fb -->
