---
Title: 'Architecture: Cardputer synth + speaker (keyboard-driven audio)'
Ticket: 007-CARDPUTER-SYNTH
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - audio
    - speaker
    - i2s
    - keyboard
    - synth
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T23:13:40.015078029-05:00
WhatFor: ""
WhenToUse: ""
---

## Executive summary

This ticket documents the shortest, most reliable path to a **Cardputer keyboard-driven synthesizer**:

- Read key events from the built-in **3×7 GPIO matrix** (3 scan outputs × 7 pulled-up inputs)
- Generate PCM audio in a tight loop (simple oscillator voices + mixer)
- Output audio to the built-in speaker via **I2S TX** (Cardputer wiring: `DOUT=GPIO42`, `BCK=GPIO41`, `WS=GPIO43`, `I2S_NUM_1`)

The repo already contains “ground truth” implementations we should reuse rather than guessing:

- **Speaker HAL + I2S pins + port**: `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp` (`_speaker_init`)
- **Speaker engine (mixer/resampler + playRaw/tone APIs)**: `M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.*`
- **Keyboard scan (ESP-IDF)**: `esp32-s3-m5/0007-cardputer-keyboard-serial/`
- **Keyboard scan + display UI event loop (ESP-IDF)**: `esp32-s3-m5/0012-cardputer-typewriter/`

## Goals

- Provide an implementation-ready architecture for a Cardputer “synth” tutorial (keyboard → sound).
- Make hardware constraints explicit (I2S pins/port, mic vs speaker contention).
- Provide a pragmatic “MVP first” path (tone/beep or mono PCM) and clear next steps for higher fidelity.

## Non-goals

- High-quality DSP, filters, effects, or perfect tuning accuracy.
- Persistent presets, patch storage, or a full UI layout (that can be a follow-up ticket).

## Hardware ground truth (Cardputer audio + keyboard)

### Speaker (output)

Vendor HAL config (Cardputer):

- **Pins**: `pin_data_out=42`, `pin_bck=41`, `pin_ws=43`
- **I2S port**: `I2S_NUM_1`

See `HAL::HalCardputer::_speaker_init()` in `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`.

### Microphone (input) and “speaker vs mic” contention

Vendor HAL config (Cardputer mic) uses:

- **pin_ws=43** (same WS/LRCK as speaker)
- **I2S port**: `I2S_NUM_0`

Because WS is shared, the vendor recorder app explicitly **stops speaker before starting mic**, and vice-versa. Treat “mic and speaker active simultaneously” as **not supported unless proven otherwise**.

### Keyboard (input)

Keyboard is a classic matrix scan:

- **OUT pins (3)** select one of 8 scan states
- **IN pins (7)** are pulled up; pressed keys read low (invert to build a 7-bit pressed mask)
- Mapping produces a **4×14** “picture coordinate system” (x=0..13, y=0..3), with labels like `"a"`, `"enter"`, `"del"`, `"space"`, etc.

Known-good scan implementation exists in `esp32-s3-m5/0007-cardputer-keyboard-serial/` (with debounce + IN0/IN1 autodetect).

## Two viable implementation strategies

### Strategy A (fastest): reuse vendor `m5::Speaker_Class` as the audio engine

Pros:

- Already solved: I2S setup, buffer scheduling, mixer/resampler, 8 virtual channels
- Convenient APIs: `tone()`, `playRaw()` (8/16-bit, signed/unsigned), `playWav()`
- Good for an MVP “press key → beep” or “press key → oscillator voice”

Cons:

- Brings in vendor HAL code and its assumptions (task priorities, legacy I2S driver usage)
- Heavier dependency surface than a minimal tutorial

Recommended when:

- You want working audio quickly and are OK treating `Speaker_Class` as a “black box” that outputs samples.

### Strategy B (cleaner tutorial): implement a minimal ESP-IDF I2S TX loop

Pros:

- Small, explicit code surface (great for learning/tutorials)
- Total control over sample format, latency, and scheduling

Cons:

- You must implement: audio buffering, voice mixer, clipping, and output pacing

Recommended when:

- You want a tutorial that teaches the actual I2S + DSP basics and avoids pulling in vendor “kitchen sink” code.

## MVP synth architecture (works with either strategy)

### Data flow

- **Keyboard task** (or main loop):
  - scan at ~10ms (like `0007`/`0012`)
  - produce events: `KeyDown(token)`, `KeyUp(token)` (or edge-only for MVP)
- **Audio engine task**:
  - runs continuously
  - fills audio blocks of N frames (e.g., 256 frames)
  - mixes active voices into `int16_t` mono (or stereo if needed)
  - writes blocks to speaker output

### Voice model

Minimum viable:

- Oscillator per active key (polyphony up to N, e.g. 8)
- Each voice has:
  - `phase` (fixed-point)
  - `phase_inc` computed from frequency
  - `amp` (envelope can be “gate” only in MVP)

Frequency mapping:

- Pick a simple mapping:
  - map keyboard letters to a scale (e.g., row `asdf...` = chromatic)
  - or map to MIDI notes and compute \( f = 440 \cdot 2^{(m-69)/12} \)

### Scheduling + latency knobs

Key parameters to document in the eventual tutorial’s `Kconfig`:

- `SYNTH_SAMPLE_RATE` (e.g. 22050 or 44100)
- `SYNTH_FRAMES_PER_BLOCK` (e.g. 256)
- `SYNTH_POLYPHONY` (e.g. 8)
- keyboard scan/debounce settings (reuse `0012`/`0007`)

Rule of thumb:

- At 44100Hz, 256 frames ≈ 5.8ms, so input-to-audio latency can be kept low.

## Practical build plan for the next implementation phase

If we build this as a new tutorial chapter (likely `esp32-s3-m5/0013-cardputer-synth/`), the recommended order:

1. **Speaker bring-up**: play a beep/tone on boot (validates I2S pins + port).
2. **Keyboard bring-up**: reuse `0012` scan code; log events and show last token on screen (optional).
3. **Key → tone**: map token to frequency and call `tone()` (Strategy A) or start a voice (Strategy B).
4. **Key-up handling**: implement note-off (requires release detection; `0012` already tracks pressed state).
5. **Polyphony + clipping**: mix multiple voices safely.
6. **UX**: on-screen status (active notes, waveform, volume).

## Key references (start here)

- `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp` — speaker/mic pins + I2S port selection
- `M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.hpp` — public speaker API (`tone`, `playRaw`, volume)
- `M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.cpp` — I2S setup + speaker task loop
- `echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/005-CARDPUTER-SYNTH--cardputer-synthesizer-audio-and-keyboard-integration/analysis/01-cardputer-speaker-and-keyboard-architecture-analysis.md` — existing audio+keyboard ground truth write-up
- `esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp` — keyboard scan + event-driven redraw pattern (good template for synth UI)

