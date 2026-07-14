---
Title: Similar PaperS3 EPD bug reports and independent driver controls
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - ereader
    - esp-idf
    - esp32s3
    - m5gfx
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/epd-painter-753c521da8aef59756df07c1a4eb88f1c64c8227/src/EPD_Painter_presets.h
      Note: |-
        Independent PaperS3-specific fast, normal, and high waveform tables
        Independent PaperS3-specific waveform control
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/m5gfx-lut-comparison/comparison.txt
      Note: |-
        Reproducible proof that M5GFX 0.2.15 and 0.2.25 use identical built-in EPD LUTs
        Proof that factory/current M5GFX built-in LUTs are identical
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/hardware/factory-v0.5/02-operator-observations.md
      Note: Official factory broad-black and dashboard visual disposition
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/web/17-m5gfx-issue-119-full-thread.md
      Note: |-
        Direct PaperS3 M5GFX versus EPDiy visual comparison
        Direct PaperS3 M5GFX-versus-EPDiy optical evidence
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/web/18-m5gfx-issue-152-full-thread.md
      Note: |-
        M5GFX maintainer discussion of excessive panel strain and unstable gradations
        Maintainer evidence about panel strain and unstable gradations
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/web/21-m5gfx-issue-166-panel-instability.md
      Note: PaperS3 panel/control-circuit instability hypotheses
ExternalSources:
    - https://github.com/m5stack/M5GFX/issues/119
    - https://github.com/m5stack/M5GFX/issues/152
    - https://github.com/m5stack/M5GFX/issues/160
    - https://github.com/m5stack/M5GFX/issues/166
    - https://github.com/Xinyuan-LilyGO/LilyGo-EPD47/issues/93
    - https://github.com/tonywestonuk/EPD_Painter
    - https://www.reddit.com/r/eink/comments/1jxgkm4/new_only_for_epdiy_fastepd_geeks_i_created_an/
Summary: Targeted search for reports matching weak full-area black, progressive gray, ghosting, waveform regressions, analog-rail sensitivity, and independent PaperS3 drivers.
LastUpdated: 2026-07-14T16:15:00-04:00
WhatFor: Distinguish a known M5GFX waveform-family limitation from an ESP-IDF regression, a generic e-paper artifact, or a defective PaperS3 panel/control circuit.
WhenToUse: Use when selecting the next independent-driver control and before interpreting the factory V0.5 result as a hardware verdict.
---


# Similar PaperS3 EPD bug reports and independent driver controls

## Executive conclusion

There are credible reports of closely related PaperS3 behavior, but I did not find a public report with our exact controlled sentence: “Factory V0.5 full-screen QUALITY black is pale/gradient while its final text dashboard is crisp.” The closest direct evidence is stronger than generic e-paper folklore:

1. PaperS3 users have reported M5GFX regions becoming progressively gray while identical application logic under EPDiy remained cleaner.
2. M5GFX's maintainer has documented prior PaperS3 control that placed excessive strain on the panel, reverse gradation after release, and individual units whose nominally undamaged pixels drift toward gray.
3. The maintainer has explicitly listed M5GFX control, the PaperS3 EPD circuit, and the panel itself as unresolved possible causes of observed PaperS3 instability.
4. Another ED047TC1-class board has measured multi-volt gate-rail ripple and VCOM-dependent corruption. That is not proof about PaperS3, but it validates rail/VCOM probing as a discriminating experiment.
5. Independent PaperS3 drivers now exist with PaperS3-specific waveform tables, explicit DC-balance handling, hard clears, and selectable black-depth timing.

Most importantly, the factory run was **not an independent waveform control**. Factory V0.5 uses M5GFX 0.2.15, while Cells C/D use M5GFX 0.2.25. Direct source extraction shows that `lut_quality`, `lut_text`, `lut_fast`, `lut_fastest`, and `lut_eraser` are byte-for-byte identical between those releases. Seeing similar full-black behavior in factory firmware therefore confirms reproducibility across application/toolchain versions, but it does not clear the M5GFX waveform family.

The next high-information experiment is an independent driver on the same board—not another M5GFX release.

## Our new factory observation

The operator reported that factory V0.5 appeared to have the same kind of issue during the whole-screen black stage. The final dashboard was decently crisp, especially for text.

That combination lowers the probability of a completely failed panel, gross pixel-bus mapping error, or display that cannot make black at all. It remains compatible with:

- a waveform whose large-area endpoint is poor but whose small black-on-white features are acceptable;
- full-area rail/VCOM droop or ripple;
- an optical/perceptual difference between broad dark fields and sparse text;
- panel history or temperature sensitivity;
- a panel/control-circuit defect that is load dependent.

White and grayscale-bar dispositions remain unreported, so the factory control is not yet a complete optical characterization.

## Direct PaperS3 reports

### M5GFX issue 119: progressive gray and EPDiy comparison

The full thread is preserved in `sources/web/17-m5gfx-issue-119-full-thread.md`.

The most relevant report states that after almost an hour, areas not updated by the new M5GFX driver “gradually get greyer and greyer” (lines 513–521), while the author compared the identical clock application using EPDiy. The same thread records faint residual images after `clearDisplay()` (line 447) and several rounds of M5GFX control adjustments. A later build became approximately equivalent to EPDiy for that user's workload (line 657), so the thread demonstrates both a real driver-controlled optical difference and the possibility of improving it in software.

This is not the same as our immediate full-black endpoint. It is nevertheless direct evidence that PaperS3 optical state can diverge materially between M5GFX and EPDiy under otherwise similar content.

### M5GFX issue 152: excessive strain and unstable gradations

The full thread is preserved in `sources/web/18-m5gfx-issue-152-full-thread.md`.

The maintainer states that M5GFX 0.2.11 had a fatal control issue that placed excessive strain on the EPD and could influence the display for tens of minutes after power-off (line 177). The described failure includes gradation shifting in the opposite direction after release. The maintainer also reports one personally owned PaperS3 whose undamaged pixels have unstable gradations and tend toward gray with continued use (line 213).

This matters operationally: back-to-back waveform tests are not necessarily independent, and a short power cycle does not guarantee a neutral panel history. It also argues against aggressive pulse experimentation until DC balance and recovery are explicit.

### M5GFX issue 166: unresolved control, circuit, or panel cause

The full thread is preserved in `sources/web/21-m5gfx-issue-166-panel-instability.md`.

For a persistent PaperS3 line/gradation fault, the maintainer lists three unresolved possibilities (lines 67–72): M5GFX control causing overload, a PaperS3 control-circuit flaw, or a panel susceptible to damage. This report concerns a localized line rather than weak whole-screen black. We have not observed a fixed localized defect, so it is a risk signal rather than a match.

### M5GFX issue 160: an actual IDF 5.4 electrical-output regression

The full thread is preserved in `sources/web/20-m5gfx-issue-160-idf54-stripes.md`.

This report traced PaperS3 stripes under IDF 5.4+ to GPIO11/12 remaining open-drain after an ESP-IDF LCD-driver refactor. It establishes that IDF changes can alter real panel drive signals. It does not fit our current result:

- Cell C under IDF 5.3.4 and Cell D under 5.4.2 looked alike;
- neither showed the report's stripe signature;
- factory firmware exhibited similar whole-black weakness;
- current M5GFX includes later fixes.

Keep this report as a reminder to verify GPIO mode and logic timing, not as the leading explanation.

### M5GFX issue 157: M5GFX 0.2.15 pushSprite regression

The full thread is preserved in `sources/web/19-m5gfx-issue-157-pushsprite-regression.md`.

M5GFX 0.2.15 caused inverted/blank/unclearable output in a PaperS3 `Canvas::pushSprite` workload and was fixed in 0.2.16. Factory V0.5 pins 0.2.15, but its boot black/white test uses direct `fillScreen`, not the affected canvas path. Cells C/D use 0.2.25 and show the same broad-black concern. This issue does not explain the shared result, though it prevents treating every factory V0.5 drawing path as pristine.

## Analog corroboration from another ED047TC1-class board

The LilyGo T5 4.7 S3 is not electrically identical to PaperS3, so its fixes must not be copied blindly. Its issue 93 thread is still useful mechanism evidence:

- approximately 3.5 Vpp ripple was measured on +22 V during update;
- +15 V oscillation was found;
- VCOM adjustment materially reduced corruption;
- the authors observed untouched areas darkening while updated areas remained clear.

See `sources/web/24-lilygo-issue-93-rails-vcom-corruption.md`, especially lines 13–37. This supports measuring PaperS3 rails under full-area and tiled loads. It does **not** establish that PaperS3 has the same op-amp or regulator defect.

## Independent driver controls found

### EPD_Painter

`tonywestonuk/EPD_Painter` is a current, independent PaperS3 driver. A pinned source snapshot is preserved under `sources/code/epd-painter-753c521.../`.

Relevant properties:

- an explicit `EPD_M5PAPER_S3_PRESET`;
- PaperS3-specific fast, normal, and high lighter/darker waveform tables;
- direct LCD_CAM DMA rather than M5GFX's generic `Panel_EPD` state machine;
- `QUALITY_HIGH`, documented as the deepest-black setting;
- hard full-panel clear and explicit DC-balance operations;
- a waveform calibrator.

This is the best next A/B control because it changes both waveform representation and scan implementation while retaining the same physical board. Its claims are project documentation, not yet verified on our unit.

### FastEPD PaperS3 grayscale matrix

The preserved Reddit post in `sources/web/22-reddit-epd-grayscale-matrix.md` publishes a PaperS3-specific 16-level matrix with 25 actions per target level. It states that approximately five or six same-polarity pushes are normally needed to seat particles at an optical extreme. The matrix is another independent waveform family, but FastEPD issue 29 also documents a now-fixed 4-bpp backup-buffer overrun. Any use must pin a revision containing that fix.

### EPDiy on modern ESP-IDF

`sources/web/23-home-assistant-papers3-epdiy-idf55.md` records a PaperS3 EPDiy fork tested on ESP-IDF 5.5.1. This provides another route to the ED047TC1 origin-to-target waveform already collected in the ticket. The forum thread includes later integration errors and revisions, so “tested and working” should be reproduced locally rather than accepted at face value.

## Critical source comparison: factory and current M5GFX share LUTs

`scripts/08-compare-m5gfx-luts.py` downloads `Panel_EPD.cpp` from M5GFX tags 0.2.15 and 0.2.25, normalizes each built-in LUT initializer, and hashes it. All five hashes match:

```text
lut_quality=IDENTICAL
lut_text=IDENTICAL
lut_fast=IDENTICAL
lut_fastest=IDENTICAL
lut_eraser=IDENTICAL
```

The complete sources and hashes are preserved in `sources/code/m5gfx-lut-comparison/`.

This changes interpretation of the factory result. Factory firmware answered “does the official application/toolchain reproduce this on the same hardware?” with a tentative yes. It did **not** answer “does an independent known-good waveform reproduce this?” because the relevant pulse tables are unchanged.

## Revised hypothesis ranking

1. **M5GFX waveform/transition-family limitation for this panel or broad-area transition.** Raised by identical factory/current LUTs and independent-driver reports.
2. **Broad-area rail or VCOM behavior.** Still serious because sparse text is crisp while whole black is poor.
3. **Panel history, temperature, or prior overstress.** Supported by M5GFX maintainer observations; not yet controlled.
4. **Unit-specific panel/control-circuit defect.** Possible, but a completely failed panel is inconsistent with the crisp dashboard.
5. **ESP-IDF-specific GPIO/scan regression.** Lowered by matching C/D/factory appearance and absence of stripes.
6. **Framebuffer color, rotation, or geometry error.** Low given deterministic scenes, correct boundaries, and readable stock UI.

## Next experiment

Build a minimal, pinned EPD_Painter control that performs only:

1. hard clear to white;
2. fixed-temperature white hold;
3. HIGH-quality full black;
4. HIGH-quality full white;
5. 25%, 50%, 75%, and 100% black-area fixtures;
6. a realistic black-text-on-white reader page;
7. explicit DC-balanced cleanup.

Do not begin with animation or endurance. Record update timing and operator appearance. If EPD_Painter produces deep uniform full black, M5GFX waveform/transition handling becomes the leading cause. If both independent drivers fail only at high black area, probe VPOS/VNEG/VGH/VGL/VCOM under area load before editing waveforms. If both fail identically at all areas, panel/VCOM/hardware and temperature move higher.

## Safety and interpretation limits

- A different driver can leave a different physical state for tens of minutes; perform its documented cleanup and allow recovery before cross-comparison.
- Do not transplant LilyGo VCOM values or modifications onto PaperS3.
- Do not tune for maximum darkness without polarity accounting and a cleanup path.
- A crisp dashboard is encouraging for the reader workload, but it does not by itself qualify grayscale, page cleanup, or long-term panel safety.
