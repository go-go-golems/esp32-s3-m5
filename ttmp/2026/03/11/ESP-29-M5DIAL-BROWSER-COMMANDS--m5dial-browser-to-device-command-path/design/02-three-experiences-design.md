---
title: "Three Dial Experiences: Design Specification"
description: "Narrative Radio, Descent Game, Memory Palace — visual design, command protocol, interaction mapping"
type: design
related_ticket: ESP-29-M5DIAL-BROWSER-COMMANDS
date: 2026-03-11
---

# Three Dial Experiences: Design Specification

Three modes that transform the M5Dial from a debug peripheral into a
physical artifact with atmosphere.  All three share a CRT green-on-black
aesthetic (matching lain-os.tsx) on both the 240x240 round screen and
the browser.  The browser and dial always show *complementary* views of
the same state — never redundant.

Common palette:

    bg          #0a0f0a
    text        #c8e6c0
    accent      #7fff7f
    warn        #ffff60
    danger      #ff6060
    critical    #ff3030
    dim         text at 20-40% opacity
    scanlines   repeating 2px transparent / 2px rgba(0,0,0,0.08)


Common firmware conventions:

  - All three modes reuse the existing websocket plumbing
    (device_hello, heartbeat, ui_command / ui_command_ack)
  - Modes are selected by a new `set_mode` command
  - Each mode defines its own `ui_command` subtypes
  - The dial's 240x240 GC9A01 draws with LovyanGFX
  - Encoder: 120 positions per full revolution
  - Button: short-press and long-press (700ms)
  - Touch: 4-direction swipe (left/right/up/down)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 MODE 1 — NARRATIVE RADIO
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Concept:  The dial is a radio tuner.  Twist to sweep frequencies.
Each frequency maps to an audio stream or spoken fragment in the
browser.  Some stations are clear, some are static, some hide
messages that reveal only if you dwell.


## Dial Screen (240x240 round)

    ┌────────────────────────────────┐
    │          ╭──────────╮          │   <- outer: frequency arc
    │       ╭──┤  WIRED   ├──╮      │      (green arc showing
    │     ╭─┘  ╰──────────╯  └─╮    │       tuned position)
    │    ╱  ·  ·  ·  ·  ·  ·  · ╲   │
    │   │ ·                    · │   │   <- dot ring: all 120
    │   │·    ╔══════════════╗  ·│   │      stations as dots
    │   │·    ║  107.3 MHz   ║  ·│   │      (green=clear,
    │   │     ║  ░░STATIC░░  ║   │   │       dim=empty,
    │   │·    ║              ║  ·│   │       yellow=distorted,
    │   │·    ║   ─┐    ┌─  ║  ·│   │       red=hidden)
    │   │ ·   ║    └──┐ │   ║ · │   │
    │    ╲ ·  ╚══════════════╝· ╱    │   <- center box: current
    │     ╰─╮  ·  ·  ·  ·  ╭─╯      │      frequency + mini
    │       ╰──────────────╯         │      waveform (4 lines
    │                                │      of box-drawing wave)
    └────────────────────────────────┘

    Elements drawn:

    1. FREQUENCY ARC (outer ring)
       - 270-degree arc from 7-o'clock to 5-o'clock
       - Bright green sweep hand at current position
       - Tick marks every 10 positions (12 ticks)
       - Arc color: dim green (#7fff7f at 30%)

    2. STATION DOTS (inner ring, r=90px)
       - 120 dots evenly spaced
       - Color indicates station type at that position
       - Current position dot is larger (4px vs 2px)

    3. CENTER DISPLAY (80x50px text area)
       - Line 1: frequency label (e.g. "107.3 MHz")
       - Line 2: station name or "░░STATIC░░"
       - Line 3-4: mini waveform if signal present
       - Font: 8px monospace, green on black

    4. BAND LABEL (top center)
       - Current band: "AM" / "FM" / "WIRED"
       - Changes on swipe left/right

    5. LOCK INDICATOR (bottom center)
       - "◉ LOCKED" when station is locked (after press)
       - Dim when scanning


## Browser Screen

    ┌─────────────────────────────────────────────────────────┐
    │  NAVI_OS v7.02a    usr: lain    RADIO    LAYER:07       │
    │─────────────────────────────────────────────────────────│
    │                                                         │
    │  ╔═══════════════════════════════════════════════════╗  │
    │  ║  BAND: WIRED          FREQ: 107.3 MHz            ║  │
    │  ║  STATION: phantom_relay                           ║  │
    │  ║  STATUS: ◉ LOCKED                signal: ███░ 72% ║  │
    │  ╚═══════════════════════════════════════════════════╝  │
    │                                                         │
    │  waveform:                                              │
    │  ─┐    ┌─┐       ┌┐                                    │
    │    │  ┌┘ └┐   ┌┐ ││    ┌─                              │
    │    └──┘   └┐ ┌┘│ │└┐  ┌┘                               │
    │            └─┘ └─┘ └──┘                                 │
    │                                                         │
    │  spectral analysis:                                     │
    │  8kHz  █ . . █ ░ . . █ █ . ░ . . █ . ░ █ . . . █ █ .  │
    │  4kHz  . ░ █ . . █ . ░ . █ . █ ░ . █ . . ░ █ . ░ . █  │
    │  2kHz  ░ . . ░ █ . █ . ░ . . . █ ░ . █ ░ . . █ . . ░  │
    │  1kHz  . . ░ . . ░ . . . ░ . . . . ░ . . . ░ . . . .  │
    │   low  . . . . . . . ░ . . . . . . . . . ░ . . . . .  │
    │                                                         │
    │  ┌─────────────────────────────────────────────────┐    │
    │  │  embedded text found in noise floor:             │    │
    │  │                                                  │    │
    │  │  c l o s e  t h e  w o r l d                     │    │
    │  │  o p e n  t h e  n E X t                         │    │
    │  └─────────────────────────────────────────────────┘    │
    │                                                         │
    │  station log:                                           │
    │  03:31  tuned 104.1   empty                             │
    │  03:31  tuned 104.8   ░░STATIC░░                        │
    │  03:32  tuned 107.3   phantom_relay      ◉ locked       │
    │  03:33  dwell 4.2s    text revealed                     │
    │                                                         │
    │  ░ ▒ ░ ▒ ░ ░ ▓ ░ ▒ ░ ▒ ░ ░ ▒ ▓ ░ ▒ ░ ░ ▒ ░ ▒ ░ ░ ▒  │
    └─────────────────────────────────────────────────────────┘

    Elements:

    1. HEADER BAR — NAVI_OS style, glitch text on title
    2. STATUS BOX — current band, freq, station, lock, signal %
    3. WAVEFORM — animated box-drawing waveform (from lain-os)
    4. SPECTROGRAM — animated spectral bars (from lain-os)
    5. HIDDEN TEXT — revealed after dwell time, typewriter effect
    6. STATION LOG — recent tuning history, timestamped
    7. AMBIENT NOISE — bottom strip (from lain-os)
    8. SCANLINES + CRT OVERLAY — full-screen (from lain-os)

    Audio: Web Audio API plays per-station audio.
    - Clear stations: looping ambient pads
    - Static stations: filtered noise
    - Hidden stations: reversed speech fragments
    - Crossfade on tune (200ms)


## Interaction Mapping

    ┌──────────────┬────────────────────────────────────────┐
    │ Input        │ Action                                 │
    ├──────────────┼────────────────────────────────────────┤
    │ Twist CW     │ Tune up (increment frequency)         │
    │ Twist CCW    │ Tune down (decrement frequency)        │
    │ Short press  │ Lock/unlock current station            │
    │ Long press   │ "Scan" — auto-advance to next signal   │
    │ Swipe L/R    │ Switch band (AM → FM → WIRED → AM)     │
    │ Swipe U/D    │ Adjust signal gain (volume in browser) │
    └──────────────┴────────────────────────────────────────┘


## Command Protocol

### Browser → Dial (ui_command)

    set_mode
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "set_mode",                         │
    │   "request_id": 1001,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "value": 1 }                                   │
    │                                                  │
    │   value: 0=debug, 1=radio, 2=descent, 3=memory   │
    └──────────────────────────────────────────────────┘

    set_station_map
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "set_station_map",                  │
    │   "request_id": 1002,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "text": "0:empty,5:clear,12:hidden,30:static"} │
    │                                                  │
    │   Compact encoding: pos:type pairs.              │
    │   Types: empty, clear, static, hidden, distorted │
    │   Dial uses this to color the station dots.      │
    └──────────────────────────────────────────────────┘

    set_band
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "set_band",                         │
    │   "request_id": 1003,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "text": "WIRED" }                              │
    │                                                  │
    │   Sent when browser initiates band change.       │
    │   Normally the dial drives this via swipe,       │
    │   but browser can override.                      │
    └──────────────────────────────────────────────────┘

    show_reveal
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "show_reveal",                      │
    │   "request_id": 1004,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "text": "close the world" }                    │
    │                                                  │
    │   When a hidden message is revealed in browser   │
    │   after dwell, send the text to dial so it can   │
    │   flash it briefly in the center display.        │
    └──────────────────────────────────────────────────┘

### Dial → Server (device events)

    encoder (existing)
    ┌──────────────────────────────────────────────────┐
    │ { "type": "encoder",                             │
    │   "device_id": "m5dial-b76a94",                  │
    │   "seq": 42,                                     │
    │   "pos": 73,                                     │
    │   "delta": 1 }                                   │
    │                                                  │
    │   Browser interprets pos as frequency index.     │
    │   Maps to station at that position.              │
    └──────────────────────────────────────────────────┘

    button (existing)
    ┌──────────────────────────────────────────────────┐
    │ { "type": "button",                              │
    │   "device_id": "m5dial-b76a94",                  │
    │   "kind": "short",                               │
    │   "pos": 73 }                                    │
    │                                                  │
    │   short → toggle lock at current frequency       │
    │   long  → start auto-scan                        │
    └──────────────────────────────────────────────────┘

    swipe (new event type)
    ┌──────────────────────────────────────────────────┐
    │ { "type": "swipe",                               │
    │   "device_id": "m5dial-b76a94",                  │
    │   "seq": 43,                                     │
    │   "direction": "left" }                          │
    │                                                  │
    │   left/right → cycle band                        │
    │   up/down    → browser adjusts gain              │
    └──────────────────────────────────────────────────┘

### Dial Firmware Interpretation

    On receiving set_mode value=1 (radio):
      - Switch display renderer to radio_draw()
      - Reset position to 0
      - Clear station map (all empty until set_station_map)

    On receiving set_station_map:
      - Parse "pos:type,..." string
      - Store in station_types[120] array
      - Redraw dot ring with new colors

    On encoder change:
      - Update frequency arc position
      - Look up station_types[pos] for center display
      - Send encoder event (server relays to browser)

    On receiving show_reveal:
      - Flash text in center display for 3 seconds
      - Dim waveform, bright text
      - Return to normal display after timeout


━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 MODE 2 — DESCENT GAME
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Concept:  Trace a signal through network hops.  Each twist of
the dial cranks you one hop deeper.  The round screen draws
concentric rings spiraling inward.  At certain depths the
display glitches.  The final prompt appears on the dial, not
the browser.  A tiny horror game where the physical device is
the interface to something unsettling.


## Dial Screen (240x240 round)

    EARLY (hops 1-5, clean):

    ┌────────────────────────────────────────┐
    │                                        │
    │        ╭╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╮            │
    │      ╭╌  ╭────────────╮  ╌╮           │
    │     ╌  ╭─┤            ├─╮  ╌          │   <- concentric rings
    │    ╌  ╭┤  ╭──────╮    │╮  ╌           │      green = traversed
    │    ╌  │┤  │ HOP5 │    ││  ╌           │      dim = unvisited
    │    ╌  │┤  │ clean │    ││  ╌           │
    │    ╌  ╰┤  ╰──────╯    │╯  ╌           │      center: current
    │     ╌  ╰─┤            ├─╯  ╌          │      hop label + status
    │      ╌╯  ╰────────────╯  ╌╯           │
    │        ╰╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╯            │
    │                                        │
    │           twist to descend              │
    └────────────────────────────────────────┘

    DEEP (hops 6-8, distorted):

    ┌────────────────────────────────────────┐
    │                                        │
    │        ╭╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╮            │
    │      ╭▓▓╭────────────╮▓▓╮             │
    │     ▓▓╭─┤░░░░░░░░░░░░├─╮▓▓           │   <- rings shift to
    │    ▓▓╭┤  ╭──────╮░░░░│╮▓▓            │      yellow/red
    │    ▓▓│┤  │ HOP7 │░░░░││▓▓            │      glitch characters
    │    ▓▓│┤  │ ░░░░ │░░░░││▓▓            │      replace clean text
    │    ▓▓╰┤  ╰──────╯░░░░│╯▓▓            │
    │     ▓▓╰─┤░░░░░░░░░░░░├─╯▓▓           │      random scanline
    │      ╰▓▓╰────────────╯▓▓╯             │      disruption every
    │        ╰╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╯            │      2-4 seconds
    │                                        │
    │           ??????????                    │
    └────────────────────────────────────────┘

    LAYER:08 (final, hop 10):

    ┌────────────────────────────────────────┐
    │                                        │
    │                                        │
    │                                        │
    │                                        │
    │         do you want to see?            │
    │                                        │
    │            [PRESS]                     │
    │                                        │
    │                                        │
    │                                        │
    │                                        │
    │                                        │
    │                                        │
    └────────────────────────────────────────┘

    POST-DESCENT:

    ┌────────────────────────────────────────┐
    │                                        │
    │                                        │
    │                                        │
    │                                        │
    │          hello, lain.                  │
    │                                        │
    │     let's all love lain.               │
    │                                        │
    │                                        │
    │                                        │
    │                                        │
    │                                        │
    │                                        │
    └────────────────────────────────────────┘

    Elements drawn:

    1. CONCENTRIC RINGS (up to 10)
       - Ring 1 (outermost, r=110): hop 1
       - Ring 10 (innermost, r=20): hop 10 / LAYER:08
       - Traversed rings: bright green (hops 1-5),
         yellow (hop 6), red (hops 7-9), white (hop 10)
       - Unvisited rings: dim (#c8e6c0 at 15%)
       - Rings are 8px wide with 2px gap

    2. CENTER LABEL (40x24px text area)
       - Current hop number + status text
       - Font: 8px monospace
       - Hops 7-8: random character replacement
         every 200ms (glitch effect)

    3. BOTTOM TEXT
       - "twist to descend" (early)
       - "??????????" (deep)
       - "[PRESS]" (Layer:08 prompt)
       - Empty (post-descent)

    4. GLITCH OVERLAY (hops 6+)
       - Random horizontal line displacement (2-4px)
       - Affects 1-3 scanlines at a time
       - Frequency increases with depth
       - At hop 9: full-screen flash every 3-5s


## Browser Screen

    ┌─────────────────────────────────────────────────────────┐
    │  NAVI_OS v7.02a    usr: lain    TRACE    LAYER:07       │
    │─────────────────────────────────────────────────────────│
    │                                                         │
    │  tracing source of unnamed.wav . . .                    │
    │                                                         │
    │  hop 1   providence      0ms     you                    │
    │  hop 2   boston_relay     3ms     clean                  │
    │  hop 3   atlantic_node   14ms    clean                  │
    │  hop 4   london_wire     22ms    clean                  │
    │  hop 5   berlin_gate     29ms    clean                  │
    │  hop 6   moscow_split    41ms    distortion             │
    │  hop 7   ??????????      ??ms    ░░░░░░░░               │
    │  hop 8   ??????????      --ms    ░░░░░░░░               │
    │                                                         │
    │  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  │
    │                                                         │
    │  the dial is descending.                                │
    │  you are at hop 8 of ?                                  │
    │                                                         │
    │  twist the dial to go deeper.                           │
    │                                                         │
    │  ┌──────────────────────────────────────────────────┐   │
    │  │   ╭╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╮                          │   │
    │  │ ╭▓▓╭────────────╮▓▓╮       DESCENT MAP          │   │
    │  │ ▓▓╭─┤ ████████ ├─╮▓▓     depth: 8/10           │   │
    │  │ ▓▓│  │  HOP 8  │  │▓▓     signal: degraded      │   │
    │  │ ▓▓╰─┤ ████████ ├─╯▓▓     integrity: 22%        │   │
    │  │ ╰▓▓╰────────────╯▓▓╯                            │   │
    │  │   ╰╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╯                          │   │
    │  └──────────────────────────────────────────────────┘   │
    │                                                         │
    │  ░ ▒ ░ ▒ ░ ░ ▓ ░ ▒ ░ ▒ ░ ░ ▒ ▓ ░ ▒ ░ ░ ▒ ░ ▒ ░ ░ ▒  │
    └─────────────────────────────────────────────────────────┘

    Elements:

    1. HEADER BAR — NAVI_OS with glitch text
    2. HOP TABLE — reveals row by row as dial twists
       - Color-coded: green → yellow → red → white
       - Each row fades in with typewriter effect
    3. DESCENT MAP — mirrors the dial's concentric rings
       (so the viewer sees what the dial holder sees)
    4. STATUS TEXT — narrative lines that change per hop
    5. GLITCH BAR — horizontal noise that intensifies
    6. LAYER:08 REVEAL — when dial reaches hop 10:
       - Browser screen flashes white then goes dark
       - Slow typewriter: "there is nothing here."
       - Then: "there is everything here."
       - Then: "hello, {user}."
       - Then: "let's all love lain."
    7. SCANLINES + CRT OVERLAY


## Interaction Mapping

    ┌──────────────┬────────────────────────────────────────┐
    │ Input        │ Action                                 │
    ├──────────────┼────────────────────────────────────────┤
    │ Twist CW     │ Descend one hop (advance ring)         │
    │              │ Only CW advances — no going back        │
    │ Twist CCW    │ Nothing (or subtle resistance text)     │
    │ Short press  │ At Layer:08 prompt: confirm "yes"       │
    │              │ At other hops: inspect current hop      │
    │ Long press   │ Abort trace — return to mode select     │
    │ Swipe any    │ No effect (you are committed)           │
    └──────────────┴────────────────────────────────────────┘

    Twist gating:  The dial must be turned N detents to
    advance each hop.  Early hops: 3 detents.  Deep hops:
    8+ detents (it gets harder to descend).  This creates
    physical effort that maps to narrative resistance.

      Hop 1-3:   3 detents per hop
      Hop 4-5:   5 detents per hop
      Hop 6-7:   8 detents per hop
      Hop 8-9:  12 detents per hop
      Hop 10:    1 press (not twist)


## Command Protocol

### Browser → Dial (ui_command)

    set_mode (value=2 for descent)

    start_descent
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "start_descent",                    │
    │   "request_id": 2001,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "text": "lain" }                               │
    │                                                  │
    │   text = username, displayed at the end.         │
    │   Resets hop counter to 0.                       │
    │   Configures detent thresholds.                  │
    └──────────────────────────────────────────────────┘

    set_hop_info
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "set_hop_info",                     │
    │   "request_id": 2002,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "value": 5,                                    │
    │   "text": "berlin_gate|clean" }                  │
    │                                                  │
    │   value = hop number                             │
    │   text = "label|status" for center display       │
    │   Sent as each hop is revealed so the dial       │
    │   can label its rings.                           │
    └──────────────────────────────────────────────────┘

    trigger_glitch
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "trigger_glitch",                   │
    │   "request_id": 2003,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "value": 3 }                                   │
    │                                                  │
    │   value = intensity (1-5)                        │
    │   Triggers a display glitch on the dial.         │
    │   Used for dramatic moments.                     │
    └──────────────────────────────────────────────────┘

    show_prompt
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "show_prompt",                      │
    │   "request_id": 2004,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "text": "do you want to see?" }                │
    │                                                  │
    │   Clears rings, shows text + [PRESS] prompt.     │
    │   Dial waits for button press.                   │
    └──────────────────────────────────────────────────┘

    show_final
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "show_final",                       │
    │   "request_id": 2005,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "text": "hello, lain.\nlet's all love lain." } │
    │                                                  │
    │   Final screen after button press.               │
    │   Displayed with slow fade-in.                   │
    └──────────────────────────────────────────────────┘

### Dial → Server (device events)

    hop_advanced (new event type)
    ┌──────────────────────────────────────────────────┐
    │ { "type": "hop_advanced",                        │
    │   "device_id": "m5dial-b76a94",                  │
    │   "seq": 44,                                     │
    │   "value": 6,                                    │
    │   "text": "moscow_split" }                       │
    │                                                  │
    │   Sent when detent threshold met for next hop.   │
    │   value = new hop number                         │
    │   Browser uses this to reveal the next table     │
    │   row and update the descent map.                │
    └──────────────────────────────────────────────────┘

    prompt_response (new event type)
    ┌──────────────────────────────────────────────────┐
    │ { "type": "prompt_response",                     │
    │   "device_id": "m5dial-b76a94",                  │
    │   "seq": 45,                                     │
    │   "text": "yes" }                                │
    │                                                  │
    │   Sent when user presses button at Layer:08.     │
    │   Browser triggers the final descent sequence.   │
    └──────────────────────────────────────────────────┘

    encoder (existing, still sent for progress tracking)

### Dial Firmware Interpretation

    On set_mode value=2 (descent):
      - Switch to descent_draw() renderer
      - Initialize 10 rings, all dim
      - Set hop=0, detent_count=0
      - Show "twist to descend" at bottom

    On encoder delta:
      - Accumulate abs(delta) into detent_count
        (only positive/CW deltas count)
      - When detent_count >= threshold[current_hop]:
        - Advance hop, reset detent_count
        - Light up next ring (color by hop depth)
        - Send hop_advanced event
      - If hop == 10: switch to prompt screen

    On set_hop_info:
      - Store label and status for ring N
      - Update center display if N == current hop

    On trigger_glitch:
      - Run glitch routine at given intensity
      - Displace random scanlines for 200ms
      - Flash random block characters

    On show_prompt:
      - Clear all rings
      - Display text centered
      - Show [PRESS] indicator
      - Wait for button press → send prompt_response

    On show_final:
      - Fade to black (200ms)
      - Typewriter text line by line (80ms/char)
      - Hold indefinitely until long-press exits


━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 MODE 3 — MEMORY PALACE
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Concept:  120 encoder positions = 120 memory slots.  Each holds
a fragment: quote, note, URL, code snippet.  The dial is a
physical memory wheel — a Rolodex from another dimension.
The Lain aesthetic frames it as "accessing memories stored in
the wired."


## Dial Screen (240x240 round)

    BROWSING (slot occupied):

    ┌────────────────────────────────────────┐
    │            ╭──────────╮                │
    │         ╭──┤ MEMORY   ├──╮             │   <- title arc
    │       ╭─┘  ╰──────────╯  └─╮          │
    │     ╱▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓╲        │   <- occupancy arc
    │    │▓                         ▓│       │      (filled slots
    │    │  ┌─────────────────────┐  │       │       are bright,
    │    │  │ #042                │  │       │       empty are dim)
    │    │  │                     │  │       │
    │    │  │ close the world     │  │       │   <- center: slot
    │    │  │ open the nEXt       │  │       │      number + first
    │    │  │                     │  │       │      2-3 lines of
    │    │  │ ── quote ────────── │  │       │      content + type
    │    │  └─────────────────────┘  │       │      tag
    │    │▓                         ▓│       │
    │     ╲▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓╱        │
    │       ╰─╮                ╭─╯          │
    │         ╰────────────────╯             │
    │          ◄ 041          043 ►          │   <- neighbor hints
    └────────────────────────────────────────┘

    BROWSING (slot empty):

    ┌────────────────────────────────────────┐
    │            ╭──────────╮                │
    │         ╭──┤ MEMORY   ├──╮             │
    │       ╭─┘  ╰──────────╯  └─╮          │
    │     ╱╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╲        │
    │    │╌                         ╌│       │
    │    │  ┌─────────────────────┐  │       │
    │    │  │ #087                │  │       │
    │    │  │                     │  │       │
    │    │  │     · empty ·       │  │       │
    │    │  │                     │  │       │
    │    │  │   swipe ▲ to add    │  │       │
    │    │  │                     │  │       │
    │    │  └─────────────────────┘  │       │
    │    │╌                         ╌│       │
    │     ╲╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╱        │
    │       ╰─╮                ╭─╯          │
    │         ╰────────────────╯             │
    │          ◄ 086          088 ►          │
    └────────────────────────────────────────┘

    Elements drawn:

    1. TITLE ARC (top)
       - "MEMORY" label

    2. OCCUPANCY ARC (270-degree outer ring)
       - Each of 120 positions is a 2.25-degree segment
       - Filled slot: bright green
       - Empty slot: dim (#c8e6c0 at 10%)
       - Current slot: white, wider (4-degree)
       - Shows at a glance how full the palace is

    3. CENTER CARD (120x80px text area)
       - Line 1: slot number "#042"
       - Lines 2-4: content preview (truncated)
       - Line 5: type tag ("quote", "note", "url", "code")
       - Empty slots: "· empty ·" + "swipe ▲ to add"

    4. NEIGHBOR HINTS (bottom)
       - Show adjacent slot numbers
       - Dim, indicating twist direction


## Browser Screen

    ┌─────────────────────────────────────────────────────────┐
    │  NAVI_OS v7.02a    usr: lain    MEMORY    LAYER:07      │
    │─────────────────────────────────────────────────────────│
    │                                                         │
    │  ┌───────────────────────────────────────────────────┐  │
    │  │  SLOT #042 of 120          type: quote            │  │
    │  │  stored: 2026-03-11 03:42  origin: manual         │  │
    │  ├───────────────────────────────────────────────────┤  │
    │  │                                                   │  │
    │  │  close the world                                  │  │
    │  │  open the nEXt                                    │  │
    │  │                                                   │  │
    │  │  — Serial Experiments Lain, episode 01            │  │
    │  │                                                   │  │
    │  └───────────────────────────────────────────────────┘  │
    │                                                         │
    │  ┌───────────────────────────────────────────────────┐  │
    │  │  nearby:                                          │  │
    │  │                                                   │  │
    │  │  #040  the wired remembers you          quote     │  │
    │  │  #041  192.168.1.42/api/nodes           url       │  │
    │  │  #042  close the world / open the nEXt  quote  ◄  │  │
    │  │  #043  · empty ·                                  │  │
    │  │  #044  fn trace(h: &Hop) -> bool {      code      │  │
    │  │                                                   │  │
    │  └───────────────────────────────────────────────────┘  │
    │                                                         │
    │  ┌───────────────────────────────────────────────────┐  │
    │  │  ADD / EDIT                                       │  │
    │  │                                                   │  │
    │  │  content: [________________________________]      │  │
    │  │  type:    [quote ▼]                               │  │
    │  │                                                   │  │
    │  │           [save to #042]   [clear slot]           │  │
    │  └───────────────────────────────────────────────────┘  │
    │                                                         │
    │  palace stats:  42/120 occupied   35% full              │
    │  last added: #041 "192.168.1.42/api/nodes" 2min ago    │
    │                                                         │
    │  ░ ▒ ░ ▒ ░ ░ ▓ ░ ▒ ░ ▒ ░ ░ ▒ ▓ ░ ▒ ░ ░ ▒ ░ ▒ ░ ░ ▒  │
    └─────────────────────────────────────────────────────────┘

    Elements:

    1. HEADER BAR — NAVI_OS style
    2. CURRENT CARD — full content for selected slot
       - Rendered with syntax highlighting for code
       - Clickable links for urls
       - Typewriter reveal on slot change
    3. NEARBY LIST — ±2 slots context, current marked ◄
       - Scrolls as dial turns (smooth animation)
    4. ADD/EDIT FORM — text input + type selector
       - Save writes to current slot
       - Clear empties current slot
       - Swipe-up on dial also opens this (focused)
    5. PALACE STATS — occupancy count, last addition
    6. SCANLINES + CRT OVERLAY


## Interaction Mapping

    ┌──────────────┬────────────────────────────────────────┐
    │ Input        │ Action                                 │
    ├──────────────┼────────────────────────────────────────┤
    │ Twist CW     │ Next slot (increment position)         │
    │ Twist CCW    │ Previous slot (decrement position)     │
    │ Short press  │ "Open" — browser shows full card +     │
    │              │ renders content (follows URL, runs      │
    │              │ code highlight, etc.)                   │
    │ Long press   │ "Search" — browser opens search/filter │
    │              │ overlay, twist to scroll results        │
    │ Swipe up     │ Add — browser focuses the edit form    │
    │ Swipe down   │ Clear current slot (with confirm on    │
    │              │ browser: "purge slot #042? [y/n]")      │
    │ Swipe left   │ Jump back 10 slots                     │
    │ Swipe right  │ Jump forward 10 slots                  │
    └──────────────┴────────────────────────────────────────┘


## Command Protocol

### Browser → Dial (ui_command)

    set_mode (value=3 for memory)

    set_slot
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "set_slot",                         │
    │   "request_id": 3001,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "value": 42,                                   │
    │   "text": "close the world\nopen the nEXt|quote"}│
    │                                                  │
    │   value = slot number                            │
    │   text = "content|type"                          │
    │   Updates the dial's local copy of this slot.    │
    │   Dial stores preview (first 60 chars) + type.   │
    └──────────────────────────────────────────────────┘

    clear_slot
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "clear_slot",                       │
    │   "request_id": 3002,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "value": 42 }                                  │
    │                                                  │
    │   value = slot number to clear                   │
    │   Dial marks slot as empty.                      │
    └──────────────────────────────────────────────────┘

    set_occupancy_map
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "set_occupancy_map",                │
    │   "request_id": 3003,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "text": "0,5,12,30,42,44,100,..." }            │
    │                                                  │
    │   Comma-separated list of occupied slot numbers. │
    │   Sent on mode entry so dial can draw the        │
    │   occupancy arc without fetching all content.    │
    └──────────────────────────────────────────────────┘

    jump_to
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "jump_to",                          │
    │   "request_id": 3004,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "value": 87 }                                  │
    │                                                  │
    │   Browser search result selected — jump dial     │
    │   to slot 87.  Dial updates position + display.  │
    └──────────────────────────────────────────────────┘

    show_message (existing, reused for confirmations)
    ┌──────────────────────────────────────────────────┐
    │ { "type": "ui_command",                          │
    │   "command": "show_message",                     │
    │   "request_id": 3005,                            │
    │   "device_id": "m5dial-b76a94",                  │
    │   "text": "saved #042" }                         │
    │                                                  │
    │   Brief confirmation toast on dial (2 seconds).  │
    └──────────────────────────────────────────────────┘

### Dial → Server (device events)

    encoder (existing)
    ┌──────────────────────────────────────────────────┐
    │ { "type": "encoder",                             │
    │   "device_id": "m5dial-b76a94",                  │
    │   "seq": 50,                                     │
    │   "pos": 42,                                     │
    │   "delta": 1 }                                   │
    │                                                  │
    │   Browser maps pos to slot number.               │
    │   Updates current card and nearby list.           │
    └──────────────────────────────────────────────────┘

    button (existing)
    ┌──────────────────────────────────────────────────┐
    │ { "type": "button",                              │
    │   "device_id": "m5dial-b76a94",                  │
    │   "kind": "short",                               │
    │   "pos": 42 }                                    │
    │                                                  │
    │   short → open/expand current slot in browser    │
    │   long  → activate search mode                   │
    └──────────────────────────────────────────────────┘

    swipe (new event type)
    ┌──────────────────────────────────────────────────┐
    │ { "type": "swipe",                               │
    │   "device_id": "m5dial-b76a94",                  │
    │   "seq": 51,                                     │
    │   "direction": "up" }                            │
    │                                                  │
    │   up    → browser focuses add/edit form          │
    │   down  → browser shows delete confirmation      │
    │   left  → jump -10 slots                         │
    │   right → jump +10 slots                         │
    └──────────────────────────────────────────────────┘

### Dial Firmware Interpretation

    On set_mode value=3 (memory):
      - Switch to memory_draw() renderer
      - Initialize occupancy_map[120] = all empty
      - Wait for set_occupancy_map to populate arc
      - Show current slot at encoder position

    On set_slot:
      - Store preview text (first 60 chars) + type
        in slot_preview[value]
      - Mark slot as occupied in occupancy_map
      - If value == current position, redraw center card

    On clear_slot:
      - Mark slot as empty in occupancy_map
      - Clear slot_preview[value]
      - Redraw if visible

    On set_occupancy_map:
      - Parse comma-separated slot numbers
      - Set occupancy_map[n] = true for each
      - Redraw occupancy arc

    On jump_to:
      - Set encoder position to value
      - Redraw with new slot focused
      - Send encoder event so browser stays in sync

    On encoder change:
      - Update occupancy arc highlight
      - Load slot_preview[pos] into center card
      - Send encoder event to server

    On swipe:
      - left:  position = (position - 10 + 120) % 120
      - right: position = (position + 10) % 120
      - up/down: send swipe event, let browser handle
      - Redraw and send encoder event for new position


━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 MODE SWITCHING
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

A mode-select screen on the dial, entered via triple-press
(3 short presses within 1 second) from any mode:

    ┌────────────────────────────────────────┐
    │                                        │
    │              MODE SELECT               │
    │                                        │
    │        ╭─────╮                         │
    │       ╱  ◉    ╲    1: RADIO            │
    │      │  RADIO  │                       │
    │      │         │   2: DESCENT          │
    │       ╲       ╱                        │
    │        ╰─────╯     3: MEMORY           │
    │                                        │
    │           twist + press                 │
    │                                        │
    └────────────────────────────────────────┘

Twist to highlight mode (3 positions), press to select.
Browser shows matching mode-select screen with descriptions.

The browser can also send set_mode to force a mode change
(e.g. from a mode picker in the UI).


━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 SHARED PROTOCOL ADDITIONS
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

New event: swipe
─────────────────
Firmware already detects swipes (kSwipe in m5dial_board.cpp)
but does not send them over websocket.  Add:

    { "type": "swipe",
      "device_id": "<id>",
      "seq": <n>,
      "ts_ms": <time>,
      "direction": "left|right|up|down" }

New command: set_mode
─────────────────────
    { "type": "ui_command",
      "command": "set_mode",
      "request_id": <id>,
      "device_id": "<id>",
      "value": <0-3> }

    0 = debug (current behavior, status text)
    1 = radio
    2 = descent
    3 = memory

Firmware maintains a mode enum and dispatches to the
appropriate draw/input handler.  Mode survives reconnection
(stored in RAM, not NVS).

New command: show_message (already exists, reused across modes)

Triple-press detection:
  - Track last 3 button timestamps
  - If all within 1000ms → emit mode_select event
  - Firmware shows mode picker screen
  - Or browser can catch 3 rapid button events and
    send set_mode directly


━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 VISUAL COHERENCE: DIAL ↔ BROWSER
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Both screens must feel like the same system.  Rules:

    1. Same palette — all colors from the common palette
       above.  No bright whites, no saturated blues.

    2. Same typography spirit — monospace everywhere.
       Dial: built-in 8px font.  Browser: Courier New
       or Lucida Console (matching lain-os.tsx).

    3. Matching elements — when the dial draws an arc,
       the browser draws the same arc (at larger scale)
       in its "device mirror" panel.  The browser always
       contains a round viewport that mirrors what the
       dial shows, plus additional context around it.

    4. Shared animation timing — glitch events happen
       simultaneously on both screens (browser receives
       the same trigger_glitch the dial does).

    5. CRT aesthetic on both — browser has scanlines +
       CRT overlay (from lain-os.tsx).  Dial approximates
       this with slightly darkened alternating pixel rows
       in the framebuffer (optional, may be too subtle
       at 240px).

    6. No chrome — no Material UI, no rounded cards with
       shadows, no gradient buttons.  Everything is flat,
       monospaced, and slightly dim.  Interactive elements
       are [bracketed text] that brightens on hover.

    7. Sound design — browser plays ambient hum in all
       modes (very low volume).  Mode transitions have
       a brief static burst.  The dial is silent (no
       speaker) but the browser compensates.
