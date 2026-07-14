# PaperS3 M5GFX runtime trace control

This numbered ESP-IDF project instruments the PaperS3 `Panel_EPD` state machine without printing or allocating while panel rails are active. It exists to explain which M5GFX update path, eraser state, frame count, and power interval actually execute for a controlled display transaction.

It is **not** the exact FactoryTest V0.5 binary. It uses the same M5GFX waveform bytes as 0.2.15 but builds pinned M5GFX 0.2.25 and M5Unified 0.2.18 under ESP-IDF 5.4.2 so trace-off and trace-timing variants can be compared reproducibly.

## Safety state

- `config.clear_display=false` prevents an initialization clear.
- Boot initializes the display object and console but issues no draw/display transaction.
- The build and audit scripts never flash or open a serial port.
- Runtime trace output is available only through an explicit `epd trace dump` after `waitDisplay()`.

## Variants

- **off**: all M5GFX trace calls, arguments, counters, and ring storage compile out.
- **trace**: a 512-record × 48-byte fixed ring records operation, queue, update preparation, power, frame, and idle boundaries.

Neither variant counts drive codes inside the 540-row scan loop. Frame content is joined to the statically decoded LUT schedules offline.

## Reproduce

From the repository root:

```bash
ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/18-prepare-m5gfx-runtime-trace.sh
ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/19-build-m5gfx-runtime-trace-variants.sh
ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/20-audit-m5gfx-runtime-trace.py
```

Complete ESP-IDF output is redirected to timestamped ticket logs. Successful builds print only concise hashes and size deltas; failures print a filtered, line-truncated tail.

## Console trace commands

These are compiled for future gated hardware work:

```text
epd trace status
epd trace dump
epd trace reset
```

`dump` emits JSON Lines using schema `esp50.m5gfx-runtime-trace.v1`. Do not dump while a display operation is active; the command takes the display mutex and calls `waitDisplay()` before reading the ring.
