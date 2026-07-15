# Tutorial 0111 — PaperS3 EPD Density Gray Ladder

A bounded successor to 0110. It uses the same pinned direct driver and emits the same post-idle semantic markers, but paints packed 2-bit full-screen code values in exactly this sequence:

```text
00 HARD white → 55 gray-1 → AA gray-2 → FF black → 00 white
```

Each target has a four-second settled window; the final white has eight seconds. This is a fixed-aperture density experiment, not a spatial or absolute-color calibration.
