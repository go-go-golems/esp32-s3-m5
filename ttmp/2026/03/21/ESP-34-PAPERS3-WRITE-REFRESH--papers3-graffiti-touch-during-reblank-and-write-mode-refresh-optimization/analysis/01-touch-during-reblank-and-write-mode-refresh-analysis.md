---
Title: Touch during reblank and write-mode refresh analysis
Ticket: ESP-34-PAPERS3-WRITE-REFRESH
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5gfx
    - m5stack
    - ui
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp
      Note: Reference startWrite/endWrite semantics on EPD targets
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/panel/Panel_IT8951.cpp
      Note: Reference EPD mode mapping for fast and text updates
    - Path: 0077-papers3-alphabet-graffiti/main/alphabet_app.cpp
      Note: Inspect Run
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-21T22:21:46.582252233-04:00
WhatFor: ""
WhenToUse: ""
---

# Touch during reblank and write-mode refresh analysis

## Problem statement

The current `0077-papers3-alphabet-graffiti` firmware appears to lose responsiveness during a reblank or full-screen e-paper update after the UI redesign. The user-facing symptom is that touch does not appear to be handled during the reblank. At the same time, the current write path still schedules a full redraw after stroke completion, which is likely broader than necessary for normal letter entry.

This document records the diagnosis boundary only. No firmware changes are made here.

## Current main loop

The event loop is still:

```cpp
while (true) {
    M5.update();
    HandleTouch();
    ProcessPendingDisplayWork();
    M5.delay(kLoopDelayMs);
}
```

That means touch polling is still attempted every loop before display work. At a high level, the code does not intentionally disable touch while the display is updating.

## Current display gating

Inside `ProcessPendingDisplayWork()`, the first check is:

```cpp
if (M5.Display.displayBusy()) {
    return;
}
```

This means that while the EPD controller is still busy:

- queued canvas clears do not run
- queued stroke segments do not flush
- deferred full redraws do not run

Touch polling in the outer loop can still happen, but the visible UI may remain stale until the panel reports idle again. That is enough to make the device feel unresponsive even if internal state is still advancing.

## Current write-mode stroke completion behavior

After stroke completion, the app does this:

```cpp
void AlphabetApp::FinishStroke()
{
    drawing_ = false;
    AnalyzeStroke();
    if (mode_ == Mode::write) {
        TryAppendRecognizedGlyph();
    }
    QueueFullRender();
}
```

This is the strongest current efficiency problem. In `WRITE` mode, the app analyzes the stroke, may append one character to the text buffer, and then requests a full UI redraw anyway.

For normal letter input, the changed regions are much smaller:

- the canvas needs to clear or reset
- the text buffer bar needs to update
- perhaps one short status line changes

The entire screen does not.

## Why the symptom is plausible

Two effects can produce the behavior the user reported.

### Visual delay can look like touch loss

If the panel is still busy with a reblank or text-quality full refresh, touch may still be sampled but the screen does not acknowledge that input visually until later. To a user, that feels like dead touch.

### Write mode still escalates to a whole-screen event

Because `FinishStroke()` always ends in `QueueFullRender()`, routine letter entry still creates a full-screen redraw request. On an e-paper device, that increases the amount of time spent waiting on slow panel work and makes subsequent input feel delayed.

## Most promising optimization direction

The best next design change is to split write-mode rendering into separate dirty-region classes:

- live stroke ink in the canvas with `epd_fast`
- localized canvas clearing with `epd_fast`
- text-buffer bar redraw independent of the rest of the layout
- full-screen redraws only for mode switches, layout changes, or explicit maintenance refreshes

That change would make write-mode rendering proportional to what actually changed, instead of treating every completed stroke as a whole-screen event.

## Follow-up questions

- During `displayBusy()` windows, are touches truly missed, or only not reflected visually until later?
- Can `WRITE` mode clear and redraw only the canvas and text bar after recognition?
- Which parts of the current write-mode UI still genuinely require `epd_text` quality updates?
- Should full-screen refresh become a rare maintenance action instead of the default post-recognition path?

## Summary

The current code does not show a complete touch shutdown. It does show a strong coupling between:

- EPD busy windows
- deferred display work
- full redraw requests after every completed write stroke

That coupling is enough to produce the "touch is not handled during reblank" experience. The most promising fix is to stop treating routine letter entry as a full-screen redraw case and instead update only the small regions that actually changed.

