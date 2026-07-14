# Cell C operator observations — 2026-07-14

## Configuration

- Matrix cell: C
- ESP-IDF: 5.3.4
- M5GFX: 0.2.25 (`ad9b814264d4e2000e9f30070002310bbccaffc9`)
- M5Unified: 0.2.18 (`b1ffcc677014ed8bd01e5a1f240736ae654bfe12`)
- Board USB identity: `Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC`

## Operator-reported visual results

- Initial text scene: works and looks good.
- Full-black scene in `epd_quality`: not genuinely black; it appears as a strange washed-out gray.
- Grayscale scene in `epd_quality`: also looks washed out; the leftmost nominal-black bar is not black.
- Checkerboard in `epd_text`: visually fine with a pleasing deep black.
- Text scene: text is fine, but the gray swatches retain visible checker-pattern ghosting.
- Boundary corpus: automatic checks passed rotations 0–3, including all 1–16-pixel edge/corner updates and explicit full logical ranges. Visual disposition still requires operator confirmation.
- Display sleep/wake: automatic command passed in 2003 ms; visual disposition still requires operator confirmation.
- Four-column QUALITY/TEXT/FAST/FASTEST nominal-black comparison: automatic command and heap check passed.
  - QUALITY: lightest column, with strange gradients; visually poor.
  - TEXT: light, with visible texture.
  - FAST: gray rather than black, but very uniform.
  - FASTEST: deepest black, but with visible texture.
  - Overall operator disposition: none of the four partial-column results is great.
- Controlled full-screen TEXT transition (`text white` followed by `text black`): the resulting nominal black is very light—almost white—and retains slight ghosting; it is a strong visual failure for solid black.

## Initial interpretation

The same `0x000000` framebuffer color produces washed-out black under `epd_quality` but deep black under `epd_text`. This points to waveform/mode behavior rather than an RGB-to-grayscale or fill-color bug. The reader refresh policy should not assume that `epd_quality` is the best mode for high-contrast black-and-white content. A committed side-by-side waveform corpus is required before accepting a mode policy.
