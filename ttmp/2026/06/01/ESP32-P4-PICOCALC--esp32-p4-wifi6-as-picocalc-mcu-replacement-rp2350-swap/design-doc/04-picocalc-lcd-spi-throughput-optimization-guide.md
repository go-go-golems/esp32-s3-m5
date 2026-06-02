---
Title: PicoCalc LCD SPI throughput optimization guide
Ticket: ESP32-P4-PICOCALC
Status: active
Topics:
    - esp32-p4
    - picocalc
    - hardware
    - firmware-port
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.2/components/esp_driver_spi/src/gpspi/spi_master.c
      Note: ESP-IDF GPSPI speed ceiling and transaction-size checks used as evidence
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.2/components/hal/esp32p4/include/hal/spi_ll.h
      Note: ESP32-P4 DMA transaction bit-length ceiling used to choose 32 KiB chunks
    - Path: 0099-esp32-p4-picocalc-display-keyboard/README.md
      Note: |-
        Operator-facing command list and current benchmark summary
        Operator-facing notes for LCD speed and throughput benchmark behavior
        Operator documentation for lcd textqueued and lcd perf queued (commit e91b3e5)
    - Path: 0099-esp32-p4-picocalc-display-keyboard/main/app_main.c
      Note: |-
        Lean ESP32-P4 PicoCalc LCD/keyboard firmware; contains SPI clock, DMA buffer, benchmark, and console-command implementation
        LCD SPI clock source
        Queued LCD row-payload transfer and double-buffered pseudo-text benchmark (commit e91b3e5)
ExternalSources: []
Summary: Analysis and task plan for maximizing PicoCalc LCD throughput on the same-position Waveshare ESP32-P4 adapter
LastUpdated: 2026-06-01T19:50:00-04:00
WhatFor: Explain why the LCD clock is capped at 80 MHz, where display time is still being spent, and which optimizations to implement next
WhenToUse: Use when continuing display performance work, comparing benchmark runs, or deciding whether the physical adapter needs a new LCD routing
---



# PicoCalc LCD SPI throughput optimization guide

## Executive summary

The ESP32-P4 GPSPI master path in ESP-IDF v5.4.2 has a normal SCLK ceiling of 80 MHz. The earlier 20 MHz ceiling was not a hardware limit of the LCD wiring; it came from ESP32-P4's default SPI clock source, `SPI_CLK_SRC_DEFAULT`, which resolves to the 40 MHz XTAL source. Selecting `SPI_CLK_SRC_SPLL` allows ESP-IDF to accept and generate actual 80 MHz SCLK.

The first throughput optimization should not try to exceed 80 MHz. Instead, it should reduce per-transfer overhead. The lean `0099` firmware originally pushed a 320×320 RGB565 fill through 512-byte chunks. A full frame is 204,800 bytes, so that path required roughly 400 pixel transactions per fill. The first optimization changed the fill path to a reusable 32 KiB internal DMA-capable buffer and raised the SPI bus maximum transfer size to 32 KiB. That reduces a full-frame fill to roughly seven pixel transactions.

Observed benchmark at actual 80 MHz:

| Firmware state | Command | Result |
|---|---|---:|
| Before DMA chunk optimization | `lcd bench 5` | ~32 ms/fill |
| After 32 KiB DMA chunk optimization | `lcd bench 5` | 21 ms/fill |
| After 32 KiB DMA chunk optimization | `lcd bench 50` | 21 ms/fill |
| Before DMA chunk optimization | `lcd bars` | ~33 ms |
| After 32 KiB DMA chunk optimization | `lcd bars` | 26 ms |
| Pattern tests added | `lcd pattern checker` | 34 ms |
| Pattern tests added | `lcd pattern stripes` | 32 ms |
| Pattern tests added | `lcd pattern diagonal` | 33 ms |
| Dirty-rectangle benchmark added | `lcd rectbench 16 16 500` | 1170 rects/s |
| Dirty-rectangle benchmark added | `lcd rectbench 80 24 200` | 843 rects/s |
| Terminal-cell benchmark added | `lcd cellbench 8 16 1000` | 1206 cells/s |
| Terminal-row benchmark added | `lcd rowbench 16 200` | 546 rows/s |
| Scroll-style redraw benchmark added | `lcd scrollbench 16 20` | 27 scrolls/s, 546 row updates/s |
| Scroll-style redraw benchmark added | `lcd scrollbench 8 20` | 18 scrolls/s, 759 row updates/s |
| Row-batched pseudo-text benchmark added | `lcd textbench 8 16 20` | 21 screens/s, 17,112 cells/s |
| Row-batched pseudo-text benchmark added | `lcd textbench 8 8 20` | 20 screens/s, 32,653 cells/s |
| Row-batched pseudo-text draw added | `lcd text 8 16` | 46 ms/screen |
| Repeatable perf suite added | `lcd perf full` fill | 21 ms/fill, 9105 KiB/s |
| Repeatable perf suite added | `lcd perf full` pattern | 33 ms/frame, 6052 KiB/s |
| Repeatable perf suite added | `lcd perf full` text8x16 | 20 screens/s; render 477 ms, transfer 476 ms over 20 screens |
| Repeatable perf suite added | `lcd perf full` cell8x16 | 1207 updates/s |
| Repeatable perf suite added | `lcd perf full` row320x16 | 546 updates/s |
| Queued row transfer added | `lcd perf queued` text8x16-poll | 950 ms / 20 screens; render 461 ms, transfer 476 ms |
| Queued row transfer added | `lcd perf queued` text8x16-queued | 568 ms / 20 screens; render 463 ms, window 59 ms, wait 21 ms; 35 screens/s |

The queued pseudo-text row path shows that overlapping row rendering with one in-flight row-payload DMA transaction can improve full pseudo-text redraw throughput substantially. The next improvements should verify the queued output visually, extend queued measurements to non-text workloads, and then focus on dirty rectangles and higher-level frame composition rather than higher SPI clocks.

## Problem statement and scope

The current target is the ClockworkPi PicoCalc LCD connected through the same-position RPico-to-Waveshare ESP32-P4-WIFI6 adapter. The current physical LCD mapping is:

```text
Pico GP10 / LCD SCK  -> ESP32-P4 GPIO3
Pico GP11 / LCD MOSI -> ESP32-P4 GPIO2
Pico GP13 / LCD CS   -> ESP32-P4 GPIO7
Pico GP14 / LCD DC   -> ESP32-P4 GPIO24
Pico GP15 / LCD RST  -> ESP32-P4 GPIO25
```

The goal is to make the display path fast enough for an interactive PicoCalc firmware while preserving the same-position adapter as the current hardware truth. The scope of this guide is the 4-wire SPI RGB565 command/data path in `0099-esp32-p4-picocalc-display-keyboard`, not Wi-Fi, ESP-Hosted, SD card, or full application rendering.

## Current-state evidence

### Firmware clock and transfer configuration

The active firmware defines the display clock and transfer policy in `0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`:

- LCD pins and dimensions are defined near lines 41-49.
- `LCD_DEFAULT_SPI_HZ` is 80 MHz near line 54.
- `LCD_SPI_CLK_SRC` is `SPI_CLK_SRC_SPLL` near line 55.
- `LCD_SPI_MAX_TRANSFER_SZ` and `LCD_FILL_DMA_CHUNK_BYTES` are 32 KiB near lines 56-57.
- The SPI device uses `.clock_source = LCD_SPI_CLK_SRC`, `.clock_speed_hz = s_lcd_spi_hz`, and `.queue_size = 4` near lines 161-167.
- The fill path allocates an internal DMA-capable buffer with `heap_caps_malloc(..., MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL)` near lines 100-118.
- `lcd_fill_rect()` fills that DMA buffer in big-endian RGB565 byte order and transmits chunks up to the DMA buffer length near lines 284-314.
- `lcd bench` prints elapsed time, per-fill time, throughput, requested clock, actual clock, and DMA chunk size near lines 531-548.

### ESP-IDF clock ceiling

ESP-IDF v5.4.2 caps ESP32-P4 GPSPI master SCLK in `components/esp_driver_spi/src/gpspi/spi_master.c`:

```c
SPI_CHECK((dev_config->clock_speed_hz > 0) &&
          (dev_config->clock_speed_hz <= MIN(clock_source_hz / 2, (80 * 1000000))),
          "invalid sclk speed", ESP_ERR_INVALID_ARG);
```

This is line 432 in the local ESP-IDF checkout. It establishes two independent constraints:

1. requested SCLK must be at most half the selected clock source; and
2. requested SCLK must be at most 80 MHz.

ESP32-P4's supported SPI clock sources in `components/soc/esp32p4/include/soc/clk_tree_defs.h` are XTAL, RC_FAST, and SPLL. `SPI_CLK_SRC_DEFAULT` resolves to XTAL in this ESP-IDF version, so default mode uses 40 MHz as the source and rejects requests above 20 MHz. `SPI_CLK_SRC_SPLL` gives the SPI driver enough source frequency to generate 80 MHz while still respecting the driver's hard 80 MHz ceiling.

### ESP-IDF DMA transaction size ceiling

ESP32-P4's SPI low-level header sets:

```c
#define SPI_LL_DMA_MAX_BIT_LEN (1 << 18)
```

That is 262,144 bits, or 32,768 bytes. The SPI master checks transaction length against this ceiling for DMA-enabled buses. Therefore 32 KiB is a natural maximum chunk size for a single TX-only DMA transaction on this path.

## Throughput model

A full 320×320 RGB565 frame contains:

```text
320 * 320 * 2 = 204,800 bytes
```

At actual 80 MHz SPI, the raw wire rate is:

```text
80,000,000 bits/s / 8 = 10,000,000 bytes/s
204,800 bytes / 10,000,000 bytes/s = 20.48 ms
```

The optimized benchmark reports roughly 21 ms per full-screen fill. That is close to the raw SPI payload floor. The remaining time is mostly unavoidable protocol overhead plus a small amount of CPU work:

1. `CASET`, `RASET`, and `RAMWR` commands for each rectangle;
2. DC GPIO transitions between command and data phases;
3. SPI transaction setup and completion overhead;
4. DMA descriptor setup;
5. filling the software color buffer.

The earlier 32 ms benchmark was not limited by the 80 MHz SCLK. It was limited by transaction count. At 512-byte chunks, a full frame needed about 400 pixel transactions. At 32 KiB chunks, the same frame needs about seven pixel transactions.

## Implemented optimization: 32 KiB DMA fill chunks

The first optimization is deliberately conservative:

1. Keep SPI at the verified actual 80 MHz clock.
2. Keep the existing panel initialization sequence.
3. Keep the existing command/data protocol.
4. Keep polling transactions for now.
5. Replace tiny stack fill chunks with a reusable internal DMA buffer.
6. Raise `spi_bus_config_t.max_transfer_sz` to the ESP32-P4 DMA transaction ceiling.

Pseudocode:

```c
ensure_lcd_initialized();
set_window(x, y, x + w - 1, y + h - 1);
ensure_dma_buffer(32768);
fill_dma_buffer_with_rgb565_color(color);
bytes_remaining = w * h * 2;
while (bytes_remaining > 0) {
    chunk = min(bytes_remaining, dma_buffer_len);
    polling_transmit(dma_buffer, chunk);
    bytes_remaining -= chunk;
}
```

This is suitable for full-screen solid fills and color-bar smoke tests. It is also useful as a baseline primitive for terminal clear, UI background fill, and dirty-rectangle clearing.

## Benchmark protocol

Use the CH343 UART monitor session for interactive tests. Confirm the port is single-owner before flashing or starting a monitor.

```bash
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00
lsof "$PORT" /dev/ttyACM1 2>/dev/null || true
```

In the firmware console:

```text
lcd init
lcd speed
lcd bench 5
lcd bench 50
lcd bars
lcd pattern checker
lcd pattern stripes
lcd pattern diagonal
lcd rectbench 16 16 500
lcd rectbench 80 24 200
lcd cellbench 8 16 1000
lcd rowbench 16 200
lcd scrollbench 16 20
lcd scrollbench 8 20
lcd textbench 8 16 20
lcd textbench 8 8 20
lcd text 8 16
lcd perf
lcd perf full
status
```

Current optimized result:

```text
lcd speed requested=80000000 actual_khz=80000
lcd bench loops=5 elapsed_ms=107 per_fill_ms=21 throughput_kib_s=9345 requested=80000000 actual_khz=80000 dma_chunk=32768
lcd bench loops=50 elapsed_ms=1071 per_fill_ms=21 throughput_kib_s=9337 requested=80000000 actual_khz=80000 dma_chunk=32768
lcd bars ok elapsed_ms=26
lcd pattern name=checker err=ESP_OK elapsed_ms=34 requested=80000000 actual_khz=80000 dma_chunk=32768
lcd pattern name=stripes err=ESP_OK elapsed_ms=32 requested=80000000 actual_khz=80000 dma_chunk=32768
lcd pattern name=diagonal err=ESP_OK elapsed_ms=33 requested=80000000 actual_khz=80000 dma_chunk=32768
lcd rectbench w=16 h=16 loops=500 elapsed_ms=427 rects_s=1170 payload_kib_s=585 requested=80000000 actual_khz=80000
lcd rectbench w=80 h=24 loops=200 elapsed_ms=237 rects_s=843 payload_kib_s=3164 requested=80000000 actual_khz=80000
lcd cellbench w=8 h=16 loops=1000 elapsed_ms=829 rects_s=1206 payload_kib_s=301 requested=80000000 actual_khz=80000
lcd rowbench h=16 loops=200 elapsed_ms=366 rows_s=546 payload_kib_s=5464 requested=80000000 actual_khz=80000
lcd scrollbench row_h=16 rows=20 loops=20 elapsed_ms=732 scrolls_s=27 row_updates_s=546 payload_kib_s=5464 requested=80000000 actual_khz=80000
lcd scrollbench row_h=8 rows=40 loops=20 elapsed_ms=1054 scrolls_s=18 row_updates_s=759 payload_kib_s=3795 requested=80000000 actual_khz=80000
lcd textbench cell_w=8 cell_h=16 cols=40 rows=20 loops=20 elapsed_ms=935 screens_s=21 cells_s=17112 payload_kib_s=4278 requested=80000000 actual_khz=80000
lcd textbench cell_w=8 cell_h=8 cols=40 rows=40 loops=20 elapsed_ms=980 screens_s=20 cells_s=32653 payload_kib_s=4081 requested=80000000 actual_khz=80000
lcd text cell_w=8 cell_h=16 cols=40 rows=20 loops=1 elapsed_ms=46 screens_s=21 cells_s=17391 payload_kib_s=4347 requested=80000000 actual_khz=80000
lcd perf case=fill loops=20 elapsed_ms=439 per_ms=21 payload_kib_s=9105
lcd perf case=pattern loops=10 elapsed_ms=330 per_ms=33 payload_kib_s=6052
lcd perf case=text8x16 loops=20 elapsed_ms=955 render_ms=477 transfer_ms=476 screens_s=20 cells_s=16744 payload_kib_s=4186
lcd perf case=cell8x16 loops=2000 elapsed_ms=1656 updates_s=1207 payload_kib_s=301
lcd perf case=row320x16 loops=400 elapsed_ms=731 updates_s=546 payload_kib_s=5465
status ... lcd_actual_khz=80000 lcd_dma_chunk=32768 lcd_dma_buf=32768
```

## Visual feedback checklist

The firmware can prove that bytes were transmitted, but it cannot prove that the glass displayed them correctly. Every speed/throughput change needs a human visual pass.

After a benchmark run, leave the display on:

```text
lcd bars
```

Check for:

1. eight vertical bars in the expected color order: red, yellow, green, cyan, blue, magenta, white, black;
2. no random pixels;
3. no flicker;
4. no vertical tearing or unstable columns;
5. no swapped red/blue channels;
6. no partial-screen update artifacts;
7. no regression after repeated `lcd bench 50` runs.

If 80 MHz artifacts appear, retest at the quantized 60 MHz setting:

```text
lcd speed 75M
lcd bars
lcd bench 20
```

On this ESP-IDF/GPSPI path, requesting 75 MHz produced an actual 60 MHz clock. If 60 MHz is clean and 80 MHz is not, the physical adapter is signal-integrity-limited rather than driver-limited.

## Optimization task list

### Completed in this phase

- [x] Explain the ESP-IDF 80 MHz GPSPI ceiling and the earlier default-XTAL 20 MHz failure mode.
- [x] Select `SPI_CLK_SRC_SPLL` explicitly.
- [x] Default the LCD to requested/actual 80 MHz.
- [x] Report actual SPI frequency via `spi_device_get_actual_freq()`.
- [x] Increase bus maximum transfer size to 32 KiB.
- [x] Replace 512-byte stack fill chunks with a reusable 32 KiB internal DMA-capable buffer.
- [x] Add benchmark throughput reporting (`throughput_kib_s`, `dma_chunk`).
- [x] Build, flash, and benchmark the optimized firmware.

### Completed measurement baseline

- [x] Ask the operator to visually inspect the current `lcd bars` output at actual 80 MHz.
- [x] Add a patterned test command that alternates checkerboard, diagonal lines, and text-like stripe patterns. Solid fills are good throughput tests but weak signal-integrity tests.
- [x] Add a dirty-rectangle benchmark command (`lcd rectbench`) for small UI updates such as cursor, character cells, and status bars.
- [x] Add terminal-cell, row, and scroll-specific benchmark commands.
- [x] Add a row-batched pseudo-text benchmark path for glyph-like RGB565 pixels.
- [x] Add a repeatable `lcd perf` / `lcd perf full` suite with comparable metrics and text render-vs-transfer split timing.
- [x] Add queued row-payload SPI transfers with `spi_device_queue_trans()` and `spi_device_get_trans_result()`.
- [x] Add double-buffered pseudo-text row rendering so the CPU can render buffer B while SPI transfers buffer A.
- [x] Add `lcd perf queued` to compare polling and queued text redraws in the same firmware build.

### Next task backlog — transfer-side optimization

- [ ] Ask the operator to visually confirm `lcd perf queued` / `lcd textqueued 8 16 20` output.
- [ ] Keep the current polling-transfer path as the baseline until queued transfer is measured and visually confirmed.
- [ ] Measure queued transfer impact separately for solid fills, generated patterns, row updates, pseudo-text rows, and mixed dirty regions.
- [ ] Decide whether queued DMA improves real workloads enough to justify the extra buffer-lifetime complexity.

### Next task backlog — renderer-side optimization

- [ ] Replace the pseudo-glyph generator with a real bitmap font renderer.
- [ ] Add a production line-buffer/blit path for real font glyphs and arbitrary RGB565 pixels.
- [ ] Add dirty-cell and dirty-row tracking for terminal-like text updates.
- [ ] Add renderer benchmarks for full-screen redraw, dirty row, dirty cell, cursor blink, and mixed edit workloads.
- [ ] Rerun `lcd perf full` after real font rendering and dirty tracking to compare against the pseudo-text baseline.

### Next task backlog — panel features and driver architecture

- [ ] Investigate ST7365P/ILI9488 vertical scroll commands.
- [ ] Add a hardware-scroll terminal benchmark if the panel scroll commands work.
- [ ] Compare the manual `spi_master` LCD path against ESP-IDF `esp_lcd_panel_io_spi`.
- [ ] Decide whether to migrate the low-level panel path to `esp_lcd_panel_io_spi`, or keep the manual command/data path for control and simplicity.

### Next task backlog — runtime architecture and stress testing

- [ ] Move display updates and perf loops from console command context into a dedicated display task.
- [ ] Define a display command queue API for app/UI code to submit fills, blits, text rows, and scroll operations.
- [ ] Add a long-running display stress test task with explicit watchdog/progress handling, separate from `esp_console`.
- [ ] Evaluate a PSRAM framebuffer or partial framebuffer for composition, while keeping active DMA buffers in internal DMA-capable memory.

### Next task backlog — hardware routing decisions

- [ ] If artifacts appear at 80 MHz, characterize stable speeds on the same-position GPIO-matrix wiring: 80 MHz, actual 60 MHz, 40 MHz, and 20 MHz.
- [ ] Evaluate final adapter routing tradeoff: same-position GPIO-matrix LCD pins versus cross-routed SPI2 IO-MUX pins GPIO28/GPIO29/GPIO30/GPIO31.

## Proposed next implementation phases

### Phase A: stabilize and validate the 32 KiB DMA baseline

This is the current phase. It should end only after operator visual feedback confirms that 80 MHz bars and repeated full-screen fills are clean.

Validation commands:

```text
lcd speed 80M
lcd bench 50
lcd bars
status
```

Acceptance criteria:

1. `lcd_actual_khz=80000`.
2. `lcd bench 50` remains around 21 ms/fill.
3. `lcd bars` remains visually correct.
4. No watchdog, heap, or SPI errors appear in the monitor.

### Phase B: add pattern tests

Solid fills can hide bit-order and signal-integrity problems. A better display test should include high-frequency pixel transitions.

Implemented command:

```text
lcd pattern checker
lcd pattern stripes
lcd pattern diagonal
lcd pattern all
```

Implementation sketch:

```c
for each row block:
    fill_dma_buffer_with_pattern(x, y, w, h);
    set_window(...);
    transmit_dma_buffer(...);
```

This requires a general pixel-buffer write path instead of the current solid-color fill path.

### Phase C: add dirty rectangle and terminal-cell benchmarks

PicoCalc workloads will usually update small regions, not full frames. A terminal-like firmware needs to know how fast it can update:

1. one character cell;
2. one text row;
3. a scrolling region;
4. a status bar;
5. a full clear.

Implemented command:

```text
lcd rectbench [w h loops]
```

Implemented terminal-specific benchmark commands:

```text
lcd cellbench [cell_w cell_h loops]
lcd rowbench [row_h loops]
lcd scrollbench [row_h loops]
```

These benchmarks should report updates per second and bytes per second.

### Phase D: evaluate queued DMA

The first queued experiment has been implemented for pseudo-text rows. It keeps LCD command/window setup on the existing polling path, then queues only the row pixel payload while DC is high. The next row is rendered into the other internal DMA-capable row buffer while the current row payload is in flight. Before the firmware changes the LCD window or toggles DC for the next row, it waits for the queued payload to complete with `spi_device_get_trans_result()`.

This preserves the important DC invariant: no GPIO DC transition may occur while an in-flight queued transaction still depends on DC being high for pixel data. It also avoids reusing a DMA buffer until the corresponding queued transaction has completed.

Measured result at actual 80 MHz:

```text
lcd perf case=text8x16-poll loops=20 elapsed_ms=950 render_ms=461 transfer_ms=476 screens_s=21 cells_s=16841 payload_kib_s=4210
lcd perf case=text8x16-queued loops=20 elapsed_ms=568 render_ms=463 window_ms=59 wait_ms=21 screens_s=35 cells_s=28152 payload_kib_s=7038
```

The result is promising because the queued path has nearly the same render time but much lower wall-clock time. The remaining measured wait time is small because most pixel transfer time overlaps with rendering the next row. This still needs operator visual confirmation because queued transfers can make display corruption harder to diagnose than the polling baseline.

For solid-color fills, reusing the same immutable DMA buffer across queued transactions is safe. For arbitrary pixel data, queuing needs at least double buffering:

```c
prepare(buffer_a);
set_window_for_a();
queue(buffer_a);
prepare(buffer_b);
wait(buffer_a);
set_window_for_b();
queue(buffer_b);
```

Risk: queueing can make visual bugs harder to diagnose. Keep the simple 32 KiB polling path as a baseline until the queued output is visually confirmed and more workloads are measured.

### Phase E: application-level optimization

Once the transport is close to the 80 MHz wire limit, the biggest gains come from sending fewer pixels:

1. maintain dirty rectangles;
2. coalesce adjacent dirty rectangles;
3. avoid full-screen redraws for cursor blink and keypress echo;
4. pre-render glyph rows into RGB565 line buffers;
5. use PSRAM for frame/state storage but internal DMA memory for active transmit buffers.

## Design decisions

### Keep 80 MHz as the normal maximum

The driver-enforced 80 MHz ceiling means normal ESP-IDF `spi_master` cannot go faster by configuration. A faster clock would require unsupported driver changes, different peripheral use, or a different display bus. That is not appropriate while the current 80 MHz path is not yet visually validated.

### Use internal DMA memory for active SPI transmit buffers

The firmware has 32 MB PSRAM, but internal DMA-capable memory is the safe default for GPSPI transmit buffers. PSRAM is appropriate for larger frame/state storage; internal DMA buffers are appropriate for active SPI transfers.

### Optimize transaction size before queueing

Large polling DMA transfers are easier to reason about than queued transactions and already reduce full-frame fill time to the theoretical 80 MHz payload floor. Queueing should be a second-stage optimization, not the first-stage baseline.

## Risks and open questions

1. **Visual correctness is not yet confirmed in this phase.** The monitor says `lcd bars ok`, but only the operator can confirm the glass is clean.
2. **GPIO-matrix signal integrity may still be the practical limiter.** ESP-IDF accepts 80 MHz, but same-position SCK/MOSI wiring through GPIO3/GPIO2 might or might not be visually robust.
3. **Solid-fill benchmarks are optimistic.** Real UI rendering will include many smaller rectangles and more command overhead.
4. **Queued DMA may not improve full-screen fills much.** The current result is already close to the raw payload limit.
5. **Future adapter routing remains a hardware tradeoff.** Function-optimized SPI2 IO-MUX routing could improve signal margin, but it breaks the simplicity of a same-position adapter.

## References

- Firmware: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/main/app_main.c`
- Operator README: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/README.md`
- ESP-IDF GPSPI master speed check: `/home/manuel/esp/esp-idf-5.4.2/components/esp_driver_spi/src/gpspi/spi_master.c`
- ESP32-P4 SPI low-level DMA limit: `/home/manuel/esp/esp-idf-5.4.2/components/hal/esp32p4/include/hal/spi_ll.h`
- Full pin map: `ttmp/2026/06/01/ESP32-P4-PICOCALC--esp32-p4-wifi6-as-picocalc-mcu-replacement-rp2350-swap/design-doc/03-full-rpico-socket-to-waveshare-esp32-p4-pin-map.md`
