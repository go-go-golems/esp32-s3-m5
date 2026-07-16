// Passive buzzer on GPIO21 via LEDC (ESP-53 P1). Owner-task-only: the
// bindings and the console handler both reach this through the owner.
// Facts from M5PaperS3-UserDemo/main/hal/hal.cpp:385 — LEDC timer 0,
// low-speed mode, 13-bit resolution, duty 4096 (50%); LEDC channel 0 is
// otherwise unused in this firmware.
#pragma once

#include <stdint.h>

#include "s3paper/status.h"

namespace pulp {

using BuzzStatusCode = s3paper::StatusCode;

// Starts a tone (lazy LEDC init on first call). duration_ms > 0 schedules
// an owner-tick stop; 0 sustains until BuzzerStop().
BuzzStatusCode BuzzerTone(int32_t freq_hz, int32_t duration_ms);

// 1 kHz, 60 ms convenience chirp.
BuzzStatusCode BuzzerBeep();

// Silences the output and cancels any pending melody.
void BuzzerStop();

// Parses "freq:ms,freq:ms,..." (max 16 notes, freq 0 = rest) and starts
// playback; the sequencer advances from BuzzerTick.
BuzzStatusCode BuzzerMelody(const char *spec);

// Owner-loop hook: stops due tones and advances the melody. Cadence is the
// owner tick (~20 ms with touch enabled) — fine for UI chimes.
void BuzzerTick(int64_t now_us);

// Fills the console snapshot (struct defined in app_events.h).
struct BuzzSnapshot;
void FillBuzzSnapshot(BuzzSnapshot *out);

}  // namespace pulp
