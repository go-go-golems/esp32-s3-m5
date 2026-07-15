# Tutorial 0110 — PaperS3 EPD Density Step Response

A bounded direct-driver experiment derived from the reviewed 0107 EPD_Painter control.

After boot it waits ten seconds for capture arming, then performs exactly one HIGH/two-stage sequence:

```text
HARD white cleanup → settle 4 s → full black → settle 4 s → full white → settle 8 s
```

It emits `EPD_DENSITY_STEP` markers only outside the direct-driver worker and after `waitIdle()`. PaperS3 output must be observed using the ticket's read-only reconnecting capture, never with pyserial or `idf.py monitor`. The physical run requires the separately preregistered ticket runner and explicit reset coordination.

Pinned environment and upstream driver provenance are inherited unchanged from `0107-papers3-epd-painter-control`: ESP-IDF 5.4.2, EPD_Painter `753c521da8aef59756df07c1a4eb88f1c64c8227`, M5PaperS3 preset, HIGH waveform tables, octal PSRAM, and USB Serial/JTAG output.
