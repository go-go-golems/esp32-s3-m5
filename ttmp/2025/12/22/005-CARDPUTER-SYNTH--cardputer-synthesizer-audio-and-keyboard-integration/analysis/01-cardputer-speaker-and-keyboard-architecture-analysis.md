---
Title: Cardputer Speaker and Keyboard Architecture Analysis
Ticket: 005-CARDPUTER-SYNTH
Status: active
Topics:
    - audio
    - speaker
    - keyboard
    - cardputer
    - esp32-s3
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp
      Note: Speaker/mic pin mapping and HAL init order (Cardputer-specific wiring)
    - Path: M5Cardputer-UserDemo/main/hal/hal_cardputer.h
      Note: HAL surface, speaker/mic/keyboard entrypoints
    - Path: M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.hpp
      Note: Speaker public API (tone/playRaw/playWav) and config struct
    - Path: M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.cpp
      Note: I2S setup + speaker task + mixer/resampler implementation
    - Path: M5Cardputer-UserDemo/main/apps/app_record/app_record.cpp
      Note: Real-world example toggling mic/speaker (mutual exclusion) + playRaw usage
    - Path: M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h
      Note: Keyboard pin mapping and 4x14 key map (labels + HID codes)
    - Path: M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp
      Note: Keyboard matrix scan algorithm (single-key and multi-key scans)
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: ESP-IDF keyboard scan implementation with debounce + USB-Serial-JTAG echo
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/Kconfig.projbuild
      Note: Kconfig for keyboard pins, scan period, debounce, settle delay, IN0/IN1 autodetect
ExternalSources: []
Summary: "How Cardputer audio output works in M5Cardputer-UserDemo (I2S speaker: tone/playRaw/playWav), how keyboard scanning works (3x7 GPIO matrix), and how tutorial 0007 improves it for ESP-IDF (debounce, autodetect, USB-Serial-JTAG output)."
LastUpdated: 2025-12-22T22:19:04.660366208-05:00
WhatFor: "Make it easy to build a Cardputer \"synth\": map keyboard events to playable tones/PCM on the built-in speaker, using known-good vendor pin mappings and APIs."
WhenToUse: "When you need to emit sound on Cardputer (beep/tone, play WAV, stream PCM) and/or when you need keyboard input (matrix scan, key mapping, debounce, modifiers)."
---

## Executive summary

Cardputer audio output in `M5Cardputer-UserDemo/` is handled by a self-contained `m5::Speaker_Class` implementation that drives an **I2S TX** peripheral in a background FreeRTOS task. The public API is already “synth-friendly”: you can play **tones** (`tone()`), **WAV blobs** (`playWav()`), or **raw PCM buffers** (`playRaw()`), and the speaker task will mix up to **8 virtual channels** and resample input buffers to the configured output sample rate.

Keyboard input in `M5Cardputer-UserDemo/` uses a classic **GPIO matrix scan**: **3 output pins** select one of 8 scan states and **7 pulled-up input pins** return a 7-bit “pressed mask”. The scan state + pressed bit maps to a physical key coordinate in a **4 rows × 14 columns** “picture coordinate system”. The `esp32-s3-m5/0007-cardputer-keyboard-serial/` tutorial re-implements this scanner in pure ESP-IDF C and adds practical improvements (debounce, optional IN0/IN1 autodetect, and serial output that works reliably over USB-Serial-JTAG).

If your immediate goal is “make sound when a key is pressed”, the shortest path is:

- Use the vendor keyboard’s `KEYBOARD::Keyboard` to get key coordinates + labels.
- Call `m5::Speaker_Class::tone(frequency, duration, channel, ...)` on press events (polyphony is easy via channels).
- If you later need better audio quality, use `playRaw(int16_t*, ...)` with alternating buffers and queueing.

## Quick recipes (copy/paste mental model)

These aren’t complete compilable examples (because your project structure may differ), but they’re the shortest “how do I make a sound / read a key?” recipes tied to the real APIs in this repo.

### Make a short beep (tone)

- **Speaker wiring reference**: `HAL::HalCardputer::_speaker_init()` (pins: `DOUT=GPIO42`, `BCK=GPIO41`, `WS=GPIO43`, `I2S_NUM_1`)
- **API**: `m5::Speaker_Class::tone()`

Pseudo-usage:

```cpp
auto spk = hal->Speaker();        // m5::Speaker_Class*
spk->setVolume(32);               // start quiet; 0 is silent, 255 is loud
spk->tone(440.0f, 200);           // A4 for 200ms
// optionally: while (spk->isPlaying()) { delay(1); }
```

### Use the keyboard (vendor HAL) and map to a label

- **API**: `KEYBOARD::Keyboard::getKey()` + `getKeyValue()`

Pseudo-usage:

```cpp
auto kb = hal->keyboard(); // KEYBOARD::Keyboard*
auto pos = kb->getKey();   // returns {x,y} or {-1,-1} if none
if (pos.x >= 0) {
  const char* label = kb->getKeyValue(pos).value_first; // e.g. "a", "enter", "opt"
  // ... handle it ...
}
```

### If you use the mic: treat mic and speaker as mutually exclusive

The vendor recorder app (`AppRecord`) toggles them explicitly:

```cpp
// record mode
speaker->end();
mic->begin();

// playback mode
mic->end();
speaker->begin();
speaker->playRaw(samples, n, 22050, /*stereo=*/false);
```

## Key files and symbols (starting points)

This section is meant to be “where do I look first?” before we get into details.

- **Speaker / audio output (vendor)**:
  - `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`
    - `HAL::HalCardputer::_speaker_init()` (**Cardputer speaker pin mapping + I2S port selection**)
    - `HAL::HalCardputer::SpeakerTest()` (**known-good tone/WAV playback usage**)
  - `M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.hpp`
    - `m5::speaker_config_t` (**pins, sample rate, DMA settings, task priority**)
    - `m5::Speaker_Class::tone()` / `playRaw()` / `playWav()` (**main public API**)
  - `M5Cardputer-UserDemo/main/hal/speaker/Speaker_Class.cpp`
    - `m5::Speaker_Class::begin()` / `end()` (**lifecycle**)
    - `m5::Speaker_Class::spk_task()` (**mixer + resampler + I2S write loop**)
    - `m5::Speaker_Class::_play_raw()` (**how play* APIs enqueue work**)

- **Microphone interaction (vendor)**:
  - `M5Cardputer-UserDemo/main/apps/app_record/app_record.cpp`
    - `_speaker->end()` / `_mic->begin()` and the inverse (**documents a real hardware constraint**)

- **Keyboard input (vendor)**:
  - `M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h`
    - `KEYBOARD::output_list` / `KEYBOARD::input_list` (**pin mapping**)
    - `KEYBOARD::_key_value_map[4][14]` (**string labels + HID keycodes**)
  - `M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp`
    - `KEYBOARD::Keyboard::getKey()` (**single key**)
    - `KEYBOARD::Keyboard::updateKeyList()` (**multi-key scan**)
    - `KEYBOARD::Keyboard::updateKeysState()` (**modifier handling + ASCII-ish output**) 

- **Keyboard input (tutorial)**:
  - `esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c`
    - `kb_scan_pressed()` (**scan loop, coordinate mapping, debounce integration**)
  - `esp32-s3-m5/0007-cardputer-keyboard-serial/main/Kconfig.projbuild`
    - `CONFIG_TUTORIAL_0007_*` (**pins, scan period, debounce, autodetect flags**)

## Speaker audio on Cardputer (how to “make sound”)

This section explains what the vendor HAL configures, why `tone()/playRaw()/playWav()` work, and what practical constraints matter for a synth.

### Hardware/pin mapping used by the vendor HAL

The vendor Cardputer HAL wires the speaker to I2S like this:

- **File**: `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`
- **Symbol**: `HAL::HalCardputer::_speaker_init()`

Key configuration (GPIO + I2S port):

- **I2S data out**: `GPIO42`
- **I2S BCLK**: `GPIO41`
- **I2S WS/LRCK**: `GPIO43`
- **I2S port**: `I2S_NUM_1`

This is the minimal “known-good” wiring to reuse for your own code (ESP-IDF or vendor-style HAL).

### Lifecycle: why “begin() is optional” (but control is still useful)

The speaker is driven by a background task that writes to I2S DMA. That task is started by `m5::Speaker_Class::begin()`.

Important practical detail: `m5::Speaker_Class::_play_raw()` calls `begin()` for you:

- If you call `tone()` / `playRaw()` / `playWav()` on a freshly constructed speaker object, the implementation will attempt to initialize I2S and spawn `spk_task()` automatically.
- That’s convenient, but it also means “start audio output” can happen implicitly.

For a synth that also uses the mic (or any other I2S user), explicit lifecycle management is safer:

- **Before recording**: `_speaker->end(); _mic->begin();`
- **Before playback**: `_mic->end(); _speaker->begin();`

The `M5Cardputer-UserDemo` recorder app makes this constraint explicit:

- **File**: `M5Cardputer-UserDemo/main/apps/app_record/app_record.cpp`
- **Notes**: it literally comments “microphone and speaker cannot be used at the same time” and toggles them around playback.

### Public APIs you actually need for a synth

The speaker API surface is intentionally small:

- **`setVolume(uint8_t master_volume)`**
  - `0` means **silent**.
  - The mixer uses a **quadratic curve**: the effective gain is proportional to \(master\_volume^2\), so small values are very quiet and loudness ramps up fast as you increase.

- **`tone(float frequency, uint32_t duration = UINT32_MAX, int channel = -1, bool stop_current_sound = true)`**
  - Intended for “beeps” but can be used as a synth oscillator.
  - Internally it repeats a short wavetable (`_default_tone_wav`, a 16-sample sine-ish shape) at the requested frequency.
  - Supports channels, so you can do simple polyphony by assigning different `channel` ids.

- **`playWav(const uint8_t* wav_data, size_t data_len = ~0u, ...)`**
  - Expects an in-memory WAV blob including header.
  - Parses the WAV header (PCM only), locates the `"data"` chunk, then enqueues playback via `_play_raw()`.

- **`playRaw(...)` overloads**
  - Supports 8-bit signed/unsigned and 16-bit signed PCM.
  - Great for a synth if you generate PCM in memory.
  - Very important constraint: the data pointer must remain valid while playing; the header warns you to use alternating buffers (or split buffers and queue halves).

### Known-good usage examples inside the vendor repo

- **Simple WAV playback**: `HAL::HalCardputer::SpeakerTest()` plays embedded WAV blobs (`boot_sound_*.h`) and waits on `isPlaying()`.
- **Record → play PCM**: `AppRecord::onRunning()` uses `playRaw(int16_t* ...)` to play recorded samples, and toggles mic/speaker around playback.

### “Synth-ready” patterns (recommended approaches)

#### Pattern A: easiest “keyboard → beep”

Use `tone()` on press events.

- **Pros**: minimal code; no buffer management; easy polyphony (channels).
- **Cons**: wavetable is tiny (16 samples), so the tone is “buzzy” and can alias at higher notes.

#### Pattern B: better sound via custom wavetable

`tone()` has an overload that accepts `raw_data` + `array_len`. You can provide a larger sine table (e.g., 256 samples) for cleaner sound.

- **Pros**: still “tone-based” and simple; better quality.
- **Cons**: still limited to “single-cycle table” synthesis unless you manage more yourself.

#### Pattern C: full PCM synth via `playRaw(int16_t*, ...)`

Generate PCM blocks (e.g., 256–1024 samples) into alternating buffers and queue them to a fixed channel.

- **Pros**: highest flexibility (envelopes, filters, FM, etc.).
- **Cons**: you need to manage timing, buffer lifetime, and queue fullness.

The implementation has a per-channel “current/next” slot; if you enqueue faster than the speaker task consumes, you’ll hit “no room” conditions (see `isPlaying(channel)` and the comments in the header about multiple buffers).

## Keyboard input on Cardputer (how scanning and mapping works)

This section explains how the keyboard matrix is scanned and how to interpret what you get back, with emphasis on differences between the vendor implementation and tutorial 0007.

### Pin mapping and electrical model

The Cardputer keyboard is a matrix scanned with:

- **3 outputs**: `GPIO8`, `GPIO9`, `GPIO11`
- **7 inputs (pulled up)**: `GPIO13`, `GPIO15`, `GPIO3`, `GPIO4`, `GPIO5`, `GPIO6`, `GPIO7`

This is codified in:

- `M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h`
  - `KEYBOARD::output_list`
  - `KEYBOARD::input_list`

Inputs are configured with pull-ups; a pressed key reads low, so the scan builds an inverted “pressed mask”.

### Scan algorithm (vendor)

At a high level (see `keyboard.cpp`):

- For each scan state `i` in `0..7`:
  - Write the 3-bit state to the output pins (`_set_output(output_list, i)`).
  - Read the 7 input pins into a mask (`_get_input(input_list)`).
  - For each set bit in the mask, map `(scan_state, bit)` → `(x,y)` and record it.

Coordinate mapping details:

- `x` is derived from the input bit position using `X_map_chart[]`, but the selected x-column depends on whether `scan_state > 3`.
- `y` is derived from `scan_state` and then flipped to match a printed “picture”: `y = (-y_base) + 3`.

Important behavioral differences inside the vendor API:

- `Keyboard::getKey()` stops at the **first detected scan state** and returns a single `(x,y)`.
- `Keyboard::updateKeyList()` collects **all pressed keys** across all scan states into `_key_list_buffer`.

### Key mapping: “picture coords” → labels + HID codes

`KEYBOARD::_key_value_map[4][14]` maps `(y,x)` to:

- `value_first` (unshifted label, e.g. `"a"`)
- `value_second` (shifted label, e.g. `"A"`)
- plus corresponding HID keycode numbers

Special keys are represented as strings like `"shift"`, `"fn"`, `"opt"`, `"enter"`, `"del"`, `"space"`.

This is why code like the recorder app works:

- It checks `getKeyValue(getKey()).value_first` against `"enter"` / `"opt"` to trigger actions.

### Tutorial 0007 improvements (ESP-IDF-friendly)

Tutorial 0007 (`esp32-s3-m5/0007-cardputer-keyboard-serial/`) reimplements the scan loop and adds:

- **Debounce guardrail**: per-key minimum time between emit events (`CONFIG_TUTORIAL_0007_DEBOUNCE_MS`).
- **Optional “settle” delay**: after changing output pins, before reading inputs (`CONFIG_TUTORIAL_0007_SCAN_SETTLE_US`).
- **IN0/IN1 autodetect**: some boards may route IN0/IN1 to `GPIO1/GPIO2` instead of `GPIO13/GPIO15`.
  - Controlled by `CONFIG_TUTORIAL_0007_AUTODETECT_IN01` and alt pin configs.
- **USB-Serial-JTAG compatible echo**: in addition to `stdout`, it writes bytes to the `usb_serial_jtag` driver when enabled.

If you’re writing your synth in “pure ESP-IDF C/C++” (not using the vendor C++ classes), the tutorial is the best minimal reference implementation to copy from.

## Integration notes for a Cardputer synth

This section turns the analysis into concrete architecture guidance.

### A practical “first synth” architecture

For a first iteration (fastest to working sound):

- **Keyboard task** (10ms scan):
  - Scan keys (vendor `updateKeyList()` or tutorial `kb_scan_pressed()`).
  - Convert key positions to note events (press/release).
  - Maintain a current “pressed keys” set (for chords).

- **Audio control** (event-driven):
  - On press: start a `tone(frequency, UINT32_MAX, channel=voiceId, stop_current_sound=true)` for that voice.
  - On release: `stop(channel)` for that voice.

This avoids streaming PCM while still giving you polyphony.

### When you outgrow `tone()`

If the `tone()` quality isn’t good enough:

- Keep the keyboard side the same.
- Replace the audio backend with `playRaw(int16_t*)` and synthesize PCM blocks.

The main design constraint becomes **buffer lifetime and queueing**: you must keep PCM buffers alive until the speaker task has consumed them, and you must enqueue at a steady rate so you don’t underflow (gaps) or overflow (no room).

## External references (useful docs)

- ESP-IDF I2S driver docs (concepts + APIs): `https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2s.html`
- ESP-IDF GPIO driver docs: `https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html`

