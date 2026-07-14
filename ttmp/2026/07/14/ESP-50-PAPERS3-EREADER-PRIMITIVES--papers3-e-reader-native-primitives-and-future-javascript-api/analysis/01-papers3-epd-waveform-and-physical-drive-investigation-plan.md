---
Title: PaperS3 EPD waveform and physical-drive investigation plan
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - ereader
    - esp-idf
    - esp32s3
    - m5gfx
    - microquickjs
    - architecture
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://0106-papers3-epd-qualification/.component-matrix/current/M5GFX/src/lgfx/v1/platforms/esp32/Bus_EPD.cpp
      Note: Pinned PaperS3 direct-drive scan timing and power sequencing
    - Path: repo://0106-papers3-epd-qualification/.component-matrix/current/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
      Note: Pinned generic M5GFX waveform and transition state machine
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/hardware/ED047TC1-datasheet.pdf
      Note: Primary panel electrical, VCOM, waveform, temperature, and optical specification
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/hardware/PaperS3-schematic-V1.0.pdf
      Note: Primary PaperS3 source/gate rail and fixed VCOM circuit
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/hardware/epdiy_ED047TC1.h
      Note: ED047TC1-specific origin-to-target reference waveform
ExternalSources:
    - https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/517/C139_ED047TC1_datasheet.pdf
    - https://github.com/vroland/epdiy/wiki/How-pixels-are-driven-in-a-parallel-epaper-with-epdiy
    - https://github.com/vroland/epdiy/wiki/Waveform-timings-for-epdiy
    - https://epdiy.readthedocs.io/en/latest/filegen.html
    - https://www.frontiersin.org/journals/physics/articles/10.3389/fphy.2021.723106/full
Summary: Evidence-based assessment and experimental plan for the PaperS3 ED047TC1 panel's washed-out black, ghosting, generic M5GFX waveforms, fixed high-voltage rails, and VCOM calibration.
LastUpdated: 2026-07-14T15:20:23.423184679-04:00
WhatFor: Replace mode-name trial and error with controlled waveform, transition-history, optical, timing, and high-voltage measurements before selecting a reader refresh policy.
WhenToUse: Use before changing M5GFX LUTs, modifying PaperS3 analog hardware, accepting a production display stack, or attributing visual artifacts to ESP-IDF.
---


# PaperS3 EPD waveform and physical-drive investigation plan

## Executive assessment

We were digging partly in the dark. The first harness was useful for memory safety, geometry, and broad visual triage, but `quality`, `text`, `fast`, and `fastest` are software labels—not physical specifications. Testing them without decoding the pulse sequences, controlling the starting optical state, measuring area and temperature, or checking the panel rails cannot identify the mechanism behind washed-out black.

The live evidence now narrows the problem substantially:

1. The same almost-white full-screen TEXT black occurs under ESP-IDF 5.3.4 and 5.4.2 with identical M5GFX/M5Unified revisions. ESP-IDF is therefore unlikely to be the visual root cause.
2. M5GFX 0.2.25 autodetects the correct 960×540 PaperS3 and drives all boundaries without corruption. Geometry/data transport is functioning.
3. FASTEST produces the darkest partial black but leaves texture. FAST is uniform gray. TEXT is light/textured. QUALITY is lightest and has gradients.
4. A TEXT checkerboard looked deep, while full-screen TEXT black after TEXT white was almost white. Output depends on transition pattern, area, and history—not only target pixel value.
5. PaperS3 uses a directly driven ED047TC1 active-matrix electrophoretic panel. M5GFX supplies generic built-in target-level LUTs; the PaperS3 board setup overrides line padding and geometry but does not install an ED047TC1-specific waveform.
6. The official ED047TC1 datasheet guarantees electrical/optical behavior only with the controller and waveform supplied by E Ink. It explicitly states that VCOM must match the assigned panel value within ±0.1 V.
7. The PaperS3 schematic uses fixed discrete high-voltage generation and a fixed VCOM divider. There is no software-controlled VCOM DAC.

The leading hypothesis is therefore **waveform mismatch or an oversimplified transition model**, with **analog rail/VCOM accuracy or full-area load droop** as a serious co-hypothesis. A defective or unusually calibrated panel remains possible until the official factory firmware is tested on this same unit.

Do not tune arbitrary pulse counts or board resistors yet. Electrophoretic waveforms must manage activation, reference-state preparation, temperature, and DC balance; undisciplined unipolar driving can increase charge trapping, ghosting, or long-term damage.

## What the panel physically does

ED047TC1 is a 4.7-inch, 540×960 active-matrix TFT electrophoretic panel. Each pixel contains oppositely charged black and white particles suspended in a viscous dielectric fluid. The pixel electrode is driven relative to the front common electrode, VCOM:

- one polarity moves black particles toward the viewing surface (darken);
- the opposite polarity moves white particles toward the viewing surface (lighten);
- zero/no-op leaves the bistable optical state largely unchanged.

Particle motion is not a linear digital assignment. It depends on electric field, pulse duration, viscosity, prior particle distribution, trapped charge, temperature, and how fully particles were activated or reset. A requested “black” is therefore a temporal voltage program, not a stored color value.

A robust grayscale update commonly has stages resembling:

1. erase or neutralize the previous image;
2. activate particles with alternating fields;
3. drive to a known black or white reference state;
4. write the desired target from that reference;
5. preserve DC balance across the complete transition path.

This explains why the prior optical state matters and why a short fast pulse can look dark but textured: it can move a useful fraction of particles quickly without fully erasing spatial history or producing a uniform equilibrium distribution.

## PaperS3 electrical path, read directly from the schematic

The official V1.0 schematic was downloaded and inspected directly. The EPD is not behind an IT8951 or similar controller; ESP32-S3 parallel data and gate/source timing drive the panel connector.

### Source rails

- U8 is an MT9700 current-limited/load-switch stage from `SYS_MAIN` to `BST_VIN`, enabled by `BST_EN`.
- U9 is an MT3608 boost regulator.
- L4 is 2.2 µH.
- The MT3608 feedback divider is R35 = 120 kΩ and R36 = 5.1 kΩ.
- Assuming the normal MT3608 0.6 V feedback reference, the unloaded positive target is approximately:

  ```text
  VPOS ≈ 0.6 × (1 + 120 / 5.1) ≈ 14.7 V
  ```

- D8 (B5819WS), C41 (4.7 µF/25 V), and associated parts form/filter `EPD_VPOS`.
- C32, D9 (B5819WS), L5, and the negative output capacitor form/filter `EPD_VNEG` from the switch node.

This matches the datasheet's typical +15 V and −15 V source rails.

### Gate rails

- `EPD_VGH` is derived from VPOS through D5/D6 (1N4148WT), a BZT52C7V5S zener, capacitors, and 120 kΩ bleeders.
- `EPD_VGL` is derived from VNEG through D11/D12, a BZT52C9V1S zener, capacitors, and 120 kΩ bleeders.

These networks aim at the panel's approximately +22 V gate-high and −20 V gate-low requirements.

### VCOM

The VCOM circuit is unambiguous in the enlarged schematic:

```text
EPD_VNEG -- R37 5.6 kΩ --+-- VCOM
                          |
                         R38 1 kΩ
                          |
                         GND
```

C42 (1 µF/10 V) filters VCOM to ground. If VNEG is −15 V, the nominal divider output is:

```text
VCOM ≈ −15 × 1 / (5.6 + 1) ≈ −2.27 V
```

This is fixed in hardware. No digital potentiometer, DAC, or programmable EPD PMIC appears in the circuit. The ED047TC1 datasheet says VCOM is assigned per panel and recommends staying within that assigned value ±0.1 V. If this particular panel's label value differs materially from −2.27 V, the board cannot correct it in firmware without changing hardware.

There are no explicit TP-designated test points in the inspected EPD crop. Measurements would need to be made at suitable component pads or connector-adjacent nodes.

## What M5GFX actually drives

`Panel_EPD.cpp` defines each M5GFX mode as a sequence of per-gray-level actions. Its own comments define:

```text
0 = end
1 = drive toward black
2 = drive toward white
3 = no operation
```

Every LUT row is one complete panel frame. The horizontal LUT dimension is target grayscale 0–15; the vertical dimension is time. The driver also keeps an internal prior-level state and may insert a short generic eraser sequence before the target sequence.

Important implementation facts:

- QUALITY and TEXT use 4-bit grayscale framebuffer levels.
- FAST and FASTEST quantize writes to black/white with a 4×4 Bayer rule.
- FAST and FASTEST skip the eraser and directly request the new sequence.
- TEXT uses a special white-sensitive eraser condition.
- PaperS3's M5GFX board configuration sets geometry and `line_padding = 8`, but leaves all waveform pointers null. `Panel_EPD::init()` then installs the generic built-in LUTs.

The TEXT target-black sequence itself begins with five “toward white” frames before a small excess of “toward black” frames. That is not inherently wrong if it is part of a calibrated activation/reset sequence, but on this panel it finishes nearly white for a full white→black transition. The result is consistent with a sequence that is not calibrated to this panel/voltage/temperature combination.

By contrast, an available ED047TC1 EPDiy waveform header contains transition-aware data indexed by both origin and target grayscale, multiple waveform modes, 15- or 30-phase sequences, and explicit phase timings. M5GFX's generic target-level LUT plus generic eraser is a materially simpler model. Porting or comparing the ED047TC1 waveform is a stronger next step than guessing new names for the four M5GFX modes.

## Ranked hypotheses

### H1 — Generic M5GFX LUT is not a suitable ED047TC1 waveform (high probability)

Evidence:

- PaperS3 does not install a panel-specific LUT.
- The datasheet requires the associated waveform and guarantees performance only with E Ink's controller/waveform.
- Different M5GFX modes produce dramatically different density and texture from the same requested black.
- IDF 5.3.4 and 5.4.2 behave the same.
- An ED047TC1-specific EPDiy waveform exists and is substantially richer.

### H2 — Full-area analog load causes VPOS/VNEG/VCOM droop or ripple (medium-high probability)

Evidence:

- The half-black checkerboard looked much darker than full black.
- A quarter-screen FASTEST column was dark, while full-screen QUALITY/TEXT were pale.
- QUALITY showed spatial gradients, a classic symptom worth correlating with rail droop or scan-position timing.
- The board uses a small MT3608 and discrete inverting/charge-pump stages rather than a monitored e-paper PMIC.

Counterpoint: the waveform sequences also differ, so area and mode are currently confounded.

### H3 — Fixed VCOM does not match this panel's assigned value (medium probability)

Evidence:

- Board VCOM is fixed around −2.27 V nominal.
- Datasheet requires panel-assigned VCOM ±0.1 V.
- A VCOM mismatch can shift black/white symmetry and increase residual images.

Required evidence: panel label value and measured VCOM under load.

### H4 — Temperature mismatch (medium-low until measured)

Electrophoretic mobility and viscosity are temperature-dependent. Current M5GFX LUT selection has no temperature range. The panel operates from 0–50 °C, but one room-temperature LUT may still be visibly wrong away from its calibration temperature.

### H5 — Damaged/aged panel or power component tolerance (unknown)

This cannot be dismissed until the factory firmware or a known-good ED047TC1 waveform is run on the same hardware. If factory output is also almost white, hardware/VCOM/panel condition moves up the ranking.

### H6 — ESP-IDF version, color constant, or geometry bug (low probability)

- C and D match visually.
- FASTEST can produce dark pixels, proving the data path can request a darkening polarity.
- Boundary/rotation tests pass without corruption.
- The same `0x000000` is represented as grayscale level 0 in the framebuffer path.

## Investigation program

## Stage 1 — Establish a known-good optical baseline

1. Run the official PaperS3 factory firmware on this exact board and photograph:
   - full white;
   - full black if exposed by the demo;
   - checkerboard;
   - grayscale ramp;
   - a realistic text page.
2. Run the exact upstream factory-demo control: ESP-IDF 5.3.3, M5GFX 0.2.15, M5Unified 0.2.10.
3. Run an ED047TC1-specific EPDiy waveform, preferably the current ESP32-S3/PaperS3 support path rather than an unrelated board profile.

Interpretation:

- Factory/EPDiy good, M5GFX generic bad → waveform/software root cause.
- All three bad → measure VCOM/rails and inspect panel condition.
- Old M5GFX good, current M5GFX bad → driver/LUT regression.

## Stage 2 — Quantify optics instead of relying only on adjectives

Use a fixed camera rig with manual exposure, white balance, focus, aperture, and illumination. Include a matte neutral reference in frame. Better still, use a colorimeter or spectrophotometer.

For each region record normalized reflectance. The datasheet specifies roughly:

- white reflectance: typical 35%;
- contrast ratio: typical 12, minimum 10;
- implied typical dark reflectance: about 35% / 12 ≈ 2.9%.

Even camera-relative measurements can determine whether “almost white” is 25%, 15%, or 5% reflectance and whether gradients correlate with scan direction.

## Stage 3 — Separate waveform, history, area, and load

Run a deterministic factorial corpus. Do not compare modes from uncontrolled prior images.

Variables:

- initial physical state: white, black, 50% checker, mid-gray;
- target: white or black;
- updated area: 1/16, 1/4, 1/2, full screen;
- shape: one contiguous block, tiled blocks, vertical stripes, horizontal stripes, checker;
- mode/waveform;
- repetitions of the same darken or lighten sequence;
- mode order;
- panel temperature.

High-value discriminating experiments:

1. **Repeated darkening dose:** white → black, then request black repeatedly. Progressive darkening means insufficient pulse dose/activation.
2. **Tiled versus simultaneous full black:** render 16 tiles sequentially versus one full-screen update. Dark tiles but pale simultaneous black strongly indicates rail/load droop.
3. **Equal-area spatial patterns:** compare 50% checker, horizontal stripes, vertical stripes, and one half-screen block. This reveals scan-direction and neighboring-pixel effects.
4. **Reverse mode order:** repeat the four-column comparison right-to-left to detect history/order bias.
5. **Power-cycle baseline:** repeat key transitions after a controlled power-down/reinit so driver state and physical state start together.
6. **Monochrome reader page:** use realistic small black glyphs on white, page-turn to a different page, then full cleanup. Solid black is diagnostic but not the primary e-reader workload.

## Stage 4 — Measure electrical rails safely

This panel uses rails up to approximately +22 V and −20 V. Only a person comfortable probing live switching converters and fine-pitch hardware should perform these measurements. Use a high-impedance DMM and appropriately rated oscilloscope probes; avoid slipping across neighboring pads or grounding a non-ground node through an earth-referenced probe.

Measure at accessible capacitor/resistor pads rather than assuming nonexistent test points:

- VPOS near C41: target approximately +15 V;
- VNEG at its output capacitor: target approximately −15 V;
- VGH near its output capacitor: target approximately +22 V;
- VGL near its output capacitor: target approximately −20 V;
- VCOM near C42/R37/R38: nominal board design approximately −2.27 V;
- `SYS_MAIN` and `BST_VIN` for supply sag.

For each rail capture:

- idle/power-on value;
- minimum/maximum during full white→black;
- ripple during full black;
- same measurements during checkerboard and sequential tiled black;
- startup and shutdown sequence.

The decisive comparison is not merely whether VPOS reads +15 V at idle; it is whether VPOS/VNEG and VCOM remain within tolerance throughout the highest-load scan frames.

Also inspect the panel/FPC label for its assigned VCOM. Do not alter R37/R38 unless a measured mismatch is established and the modification is reviewed.

## Stage 5 — Capture digital drive timing

Use a logic analyzer or oscilloscope on:

- XCL/CL (source shift clock);
- CKV and SPV (gate row timing);
- XLE (source latch);
- XOE/EPD_PWR (output enable/power control);
- representative D0–D7 data lines.

Verify:

- clocks and active polarities match the datasheet;
- exactly 540 gate rows and the expected 960 source pixels are emitted;
- line padding is harmless;
- frame rate is stable and near the waveform's intended rate;
- phase count and black/white/no-op codes match software;
- rail enable sequencing meets the datasheet requirements.

A software LUT only has physical meaning when the frame duration and source/gate timing are known.

## Stage 6 — Decode and compare waveforms offline

Add a host script that emits, for every M5GFX mode and target gray:

- ordered B/W/no-op sequence;
- number of frames;
- net signed drive count;
- maximum consecutive same-polarity run;
- sequence after the generic eraser for each origin gray;
- expected duration from measured frame timing.

Decode the ED047TC1 EPDiy header into the same conceptual traces, including origin→target transitions and phase timing. This will show exactly what information cannot fit in M5GFX's current target-only LUT representation.

## Stage 7 — Engineer only after measuring

Preferred order:

1. use a known ED047TC1 vendor/EPDiy waveform unchanged;
2. adapt M5GFX to accept a full origin×target transition waveform if needed;
3. only then tune a custom waveform with controlled optical and electrical measurements.

Any custom waveform should explicitly document:

- reference-state preparation;
- particle activation stage;
- write stage;
- origin/target transition matrix;
- temperature range;
- DC-balance argument over complete use cycles;
- cleanup cadence;
- measured reflectance, ghosting, latency, and rail behavior.

Do not “make black darker” by adding indefinite black pulses. Darker short-term output is not evidence of safe DC balance or stable long-term operation.

## Immediate next actions

1. Preserve Cell D's matching visual failure and boundary pass.
2. Install/build exact Cell A rather than substituting an IDF.
3. Acquire a factory-firmware baseline on the same board.
4. Add the controlled area/history fixture and offline LUT decoder.
5. Run the non-invasive optical tests before opening the enclosure.
6. If software baselines disagree or remain poor, inspect the panel VCOM label and measure rails under load.

## Current decision

Phase 0 remains **not qualified**. We should pause long text/mixed-update endurance soaks: they test stability but will not explain or repair the nearly-white black endpoint, and excessive unbalanced experiments could worsen panel history. The next milestone is causal waveform/analog diagnosis, not more undirected mode cycling.
