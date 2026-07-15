# Printalyzer v1.1.0 analysis source map

This is a focused source snapshot for the Printalyzer physically connected during ESP-50. The device reported:

```text
GS V,"Printalyzer Densitometer","v1.1.0"
GS B,"2023-06-13 17:41","g7101373",8E155935
GS UID,323147103439323344002900
```

GitHub resolves `g7101373` to official upstream commit `710137374fa4131693c6dea65670c7759479e6a5`, titled “Bump app version to v1.1.0.” This snapshot uses that commit rather than assuming current `master` matches the installed instrument.

## Why these files are present

| Path | Analysis use |
|---|---|
| `software/firmware/src/cdc_handler.*` | CDC connection state, command grammar, responses, remote/raw operations, and DTR-dependent host connection |
| `software/firmware/src/densitometer.*` | Normal calibrated reflection/transmission density formulas and output publication |
| `software/firmware/src/sensor.*` | Target-read sequence, gain selection, integration, saturation, basic-count normalization, and slope correction |
| `software/firmware/src/tsl2591.*` | Sensor gain/time enums, saturation thresholds, and constants |
| `software/firmware/src/state_display.*` | Detect-switch gate for normal measurements and target-light timeout behavior |
| `software/firmware/src/state_remote.*` | Remote-mode entry/exit cleanup and light/sensor shutdown |
| `software/firmware/src/state_main_menu.*` | User-visible Target Light settings and diagnostic controls |
| `software/firmware/src/keypad.*` | Physical detect-switch GPIO interpretation |
| `software/firmware/src/settings.*` | Persisted light, gain, slope, and target calibration representations |
| `software/firmware/src/task_sensor.*` | Continuous sensor task and event cadence |
| `software/desktop/src/` | Vendor-side serial parsing, float decoding, and remote-control usage |
| `docs/` | Protocol, assembly, schematic plots, and operating context at the installed commit |
| `hardware/main-board/` | Detect switch, sensor, USB, and controller board source/plots |
| `hardware/trans-led-board/` | Transmission-light board source/plots |
| `enclosure/` | 2D geometry and enclosure notes relevant to a PaperS3 measurement fixture |

## Deliberate exclusions

This is not a complete build checkout. It excludes:

- `.git` history and metadata;
- vendored TinyUSB, U8g2, STM32 HAL, FreeRTOS, and other third-party trees;
- desktop deployment binaries;
- complete 3D CAD;
- compiler/toolchain installation and generated build products.

Those are unnecessary for the current protocol, timing, calibration, and measurement-geometry analysis. There is no plan to build or flash custom Printalyzer firmware. If that goal changes, create a separate exact checkout and build-reproduction task rather than treating this analysis snapshot as buildable.

## Relationship to the current-upstream snapshot

The sibling `../printalyzer-protocol-f91c91ecc60bb1f435b8dacfc9929f45315f3912/` captures the current protocol/manual and selected current implementation files. Use this `7101373...` tree for claims about the connected v1.1.0 instrument; use the `f91c91e...` tree only to identify later upstream behavior or documentation changes.
