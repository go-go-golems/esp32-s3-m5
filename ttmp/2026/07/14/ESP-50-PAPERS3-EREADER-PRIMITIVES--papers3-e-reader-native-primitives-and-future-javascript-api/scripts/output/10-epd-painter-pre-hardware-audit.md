---
Title: EPD Painter Pre-Hardware Audit Output
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - m5gfx
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources:
    - https://github.com/tonywestonuk/EPD_Painter/commit/753c521da8aef59756df07c1a4eb88f1c64c8227
Summary: "Generated source audit and hardware-use gate for pinned EPD_Painter."
LastUpdated: 2026-07-14T20:10:17.189748+00:00
WhatFor: "Reproduce the source-level decision that blocks unmodified EPD_Painter from PaperS3 hardware execution."
WhenToUse: "Regenerate and review before changing the local hardening patch or flashing the independent control."
---

# EPD_Painter pre-hardware audit

Generated: 2026-07-14T20:10:17.189748+00:00

## Audit gate

**BLOCKED — create and review a narrow local hardening patch before build/flash**

Blockers: 5; review items: 2.

This audit evaluates commit `753c521da8aef59756df07c1a4eb88f1c64c8227`. It does not establish optical quality or panel safety. It determines whether the unmodified source is suitable for the first controlled hardware run.

## Pin comparison

| Signal | EPD_Painter | M5GFX 0.2.25 |
|---|---:|---:|
| `pin_pwr` | 46 | 46 |
| `pin_spv` | 17 | 17 |
| `pin_ckv` | 18 | 18 |
| `pin_sph` | 13 | 13 |
| `pin_oe` | 45 | 45 |
| `pin_le` | 15 | 15 |
| `pin_cl` | 16 | 16 |
| `data[0..7]` | `[6, 14, 7, 12, 9, 11, 8, 10]` | `[6, 14, 7, 12, 9, 11, 8, 10]` |

## Waveform action counts

EPD_Painter documents `0=float`, `1=whiten`, `2=darken`, and `3=both`. Counts below are descriptive only; equal counts do not prove equal physical dose.

| Table | Row | 0 float | 1 whiten | 2 darken | 3 both |
|---|---:|---:|---:|---:|---:|
| `fast_lighter` | 0 | 0 | 1 | 5 | 1 |
| `fast_lighter` | 1 | 0 | 0 | 5 | 2 |
| `fast_lighter` | 2 | 0 | 0 | 7 | 0 |
| `fast_darker` | 0 | 0 | 4 | 0 | 3 |
| `fast_darker` | 1 | 0 | 5 | 0 | 2 |
| `fast_darker` | 2 | 0 | 7 | 0 | 0 |
| `normal_lighter` | 0 | 0 | 4 | 8 | 1 |
| `normal_lighter` | 1 | 0 | 3 | 10 | 0 |
| `normal_lighter` | 2 | 0 | 0 | 13 | 0 |
| `normal_darker` | 0 | 0 | 8 | 4 | 1 |
| `normal_darker` | 1 | 0 | 8 | 2 | 3 |
| `normal_darker` | 2 | 0 | 13 | 0 | 0 |
| `high_lighter` | 0 | 0 | 5 | 7 | 1 |
| `high_lighter` | 1 | 0 | 2 | 8 | 3 |
| `high_lighter` | 2 | 0 | 0 | 13 | 0 |
| `high_darker` | 0 | 0 | 7 | 5 | 1 |
| `high_darker` | 1 | 0 | 9 | 3 | 1 |
| `high_darker` | 2 | 0 | 13 | 0 | 0 |

## Findings

### 1. [PASS] PaperS3 scan and power pins match M5GFX 0.2.25

**Evidence:** EPD_Painter preset line 54; M5GFX board block line 1948. Data=[6, 14, 7, 12, 9, 11, 8, 10]; controls={'pin_pwr': 46, 'pin_spv': 17, 'pin_ckv': 18, 'pin_sph': 13, 'pin_oe': 45, 'pin_le': 15, 'pin_cl': 16}.

**Consequence:** The independent driver targets the same physical signals as the qualified M5GFX path.

**Required action:** Keep these values pinned and assert them in the firmware build metadata.

### 2. [BLOCKER] GPIO pad-selection helper receives an IOMUX register address instead of a GPIO number

**Evidence:** `EPD_Painter.cpp:290` calls `esp_rom_gpio_pad_select_gpio()` with `GPIO_PIN_MUX_REG[pin]`; the ESP-IDF API takes the GPIO number.

**Consequence:** The raw pinned driver must not be run unchanged. Pin mux configuration can address invalid GPIO indices or leave data pins misconfigured.

**Required action:** Patch both calls to pass `pin` and `_config.pin_cl`, then compile and inspect the generated path before hardware use.

### 3. [PASS] Direct-GPIO power-on ordering matches M5GFX

**Evidence:** `epd_painter_powerctl.h:75` raises OE, waits 100 µs, then raises PWR.

**Consequence:** The tested enable order is retained.

**Required action:** Preserve this sequence.

### 4. [REVIEW] Power-off order differs from M5GFX and does not lower SPV

**Evidence:** `epd_painter_powerctl.h:83` lowers OE before PWR. M5GFX lowers PWR, then OE, then SPV. `EPD_Painter::powerOff()` only delegates to the power driver.

**Consequence:** The alternative sequence may be safer, equivalent, or incorrect, but it is an uncontrolled difference in a physical experiment.

**Required action:** Use an explicit local safe-state function, document the chosen order, and lower LE/SPV/SPH around power-off.

### 5. [BLOCKER] Packed physical-state buffers are allocated without initialization

**Evidence:** Allocations begin near `EPD_Painter.cpp:362`; no zeroing occurs before the paint task starts.

**Consequence:** The first differential update can compare a target against indeterminate PSRAM and drive arbitrary transitions.

**Required action:** Zero fast, screen, paint, and bitmask buffers before task creation; begin every experiment with a documented hard clear.

### 6. [BLOCKER] Initialization does not validate every required allocation

**Evidence:** `EPD_Painter.cpp:404` omits `packed_paintbuffer` and `bitmask`.

**Consequence:** Allocation failure can become a null dereference in the background task.

**Required action:** Extend the guard and fail cleanly before creating semaphores or tasks.

### 7. [BLOCKER] `paint()` returns after buffer pickup, not after scan completion

**Evidence:** `EPD_Painter.cpp:491` waits only until the initial stage value changes.

**Consequence:** Optical timing, heap checks, cleanup ordering, and power-down cannot be bounded by the caller.

**Required action:** Add a public `waitIdle(timeout)` and require it after every experimental operation.

### 8. [REVIEW] Adafruit binding unconditionally enables three-stage convergence

**Evidence:** `EPD_Painter_Adafruit.h:68` sets `paintStage` to 3 per paint.

**Consequence:** One requested paint can execute multiple waveform scans, complicating pulse-dose comparison with M5GFX.

**Required action:** Expose the stage policy explicitly in the control firmware and report it with each result.

### 9. [BLOCKER] Adafruit framebuffer allocation is dereferenced without a null check

**Evidence:** `EPD_Painter_Adafruit.h:63` allocates, then immediately calls `memset`.

**Consequence:** A PSRAM allocation failure crashes before diagnostics.

**Required action:** Use a local hardened binding or raw driver wrapper with explicit allocation checks.

### 10. [PASS] HARD clear alternates 20 full-panel actions with equal aggregate polarity counts

**Evidence:** `EPD_Painter.cpp:829` uses phase counts 6, 2, 4, 8; alternating phases total 10 actions per polarity.

**Consequence:** The explicit clear is a suitable controlled starting and ending operation, subject to verifying code-to-voltage polarity and endpoint.

**Required action:** Use HARD clear sparingly and record its duration and final optical state.

### 11. [PASS] Automatic shutdown can be disabled for the experiment

**Evidence:** `EPD_Painter.h:195` exposes the control.

**Consequence:** The test can avoid reset-toggle, NVS, shutdown-image, and automatic unpaint behavior.

**Required action:** Call `setAutoShutdown(false)` before `begin()` and implement no system-power-off command in the first firmware.

## Approved next step

Create the firmware in a numbered repository directory, not in a temporary directory. Vendor or pin the exact EPD_Painter commit through a reproducible ticket script. Apply only the audited hardening changes: correct GPIO pad selection, initialize and validate all buffers, add bounded idle waiting, make stage count explicit, disable automatic shutdown, and drive control pins to a documented safe state. Build and inspect the binary before any flash.

The first hardware operation remains a HARD white cleanup followed by idle wait. No black waveform runs until boot diagnostics, buffer initialization, command gating, and cleanup completion are visible on the serial console.
