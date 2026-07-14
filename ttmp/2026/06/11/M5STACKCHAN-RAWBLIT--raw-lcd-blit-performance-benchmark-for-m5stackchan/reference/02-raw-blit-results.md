# Raw Blit Benchmark Results

Generated from serial monitor logs under `/tmp/stackchan-rawblit-*-monitor.log`.

Best full-screen measured case: `full_320x240_chunk120` at 80 MHz requested, 36.21 FPS, 5.56 MB/s.

## Charts

![Full-screen throughput](images-m5stackchan-draw-performance/raw-blit-fullscreen-throughput.png)

![Partial region throughput](images-m5stackchan-draw-performance/raw-blit-partial-throughput.png)

![Chunk completion latency](images-m5stackchan-draw-performance/raw-blit-chunk-completion-latency.png)

## Summary table

| Clock | Case | FPS | MB/s | Chunk | Pattern | Avg complete us |
|---:|---|---:|---:|---:|---|---:|
| 40 MHz | `full_320x240_chunk120` | 25.00 | 3.84 | 120 | generated | 16707 |
| 40 MHz | `full_320x240_chunk20` | 16.30 | 2.50 | 20 | generated | 4521 |
| 40 MHz | `full_320x240_chunk40` | 16.66 | 2.56 | 40 | generated | 8884 |
| 40 MHz | `full_320x240_chunk80` | 22.18 | 3.40 | 80 | generated | 12849 |
| 40 MHz | `full_320x240_chunk80_solid` | 24.27 | 3.72 | 80 | solid | 12675 |
| 40 MHz | `half_320x120_chunk40` | 33.33 | 2.56 | 40 | generated | 8842 |
| 40 MHz | `quarter_160x120_chunk40` | 61.69 | 2.36 | 40 | generated | 4612 |
| 40 MHz | `tile_32x32_chunk32` | 266.08 | 0.54 | 32 | generated | 2788 |
| 40 MHz | `tile_80x60_chunk60` | 198.89 | 1.90 | 60 | generated | 4361 |
| 60 MHz | `full_320x240_chunk120` | 25.00 | 3.84 | 120 | generated | 16707 |
| 60 MHz | `full_320x240_chunk20` | 16.30 | 2.50 | 20 | generated | 4521 |
| 60 MHz | `full_320x240_chunk40` | 16.66 | 2.56 | 40 | generated | 8885 |
| 60 MHz | `full_320x240_chunk80` | 22.18 | 3.40 | 80 | generated | 12849 |
| 60 MHz | `full_320x240_chunk80_solid` | 24.88 | 3.82 | 80 | solid | 12655 |
| 60 MHz | `half_320x120_chunk40` | 33.33 | 2.56 | 40 | generated | 8843 |
| 60 MHz | `quarter_160x120_chunk40` | 61.69 | 2.36 | 40 | generated | 4612 |
| 60 MHz | `tile_32x32_chunk32` | 266.08 | 0.54 | 32 | generated | 2788 |
| 60 MHz | `tile_80x60_chunk60` | 198.89 | 1.90 | 60 | generated | 4361 |
| 80 MHz | `full_320x240_chunk120` | 36.21 | 5.56 | 120 | generated | 9718 |
| 80 MHz | `full_320x240_chunk20` | 24.99 | 3.83 | 20 | generated | 2733 |
| 80 MHz | `full_320x240_chunk40` | 31.91 | 4.90 | 40 | generated | 3873 |
| 80 MHz | `full_320x240_chunk80` | 33.33 | 5.12 | 80 | generated | 7472 |
| 80 MHz | `full_320x240_chunk80_solid` | 33.33 | 5.12 | 80 | solid | 8921 |
| 80 MHz | `half_320x120_chunk40` | 61.43 | 4.71 | 40 | generated | 3927 |
| 80 MHz | `quarter_160x120_chunk40` | 99.12 | 3.80 | 40 | generated | 2717 |
| 80 MHz | `tile_32x32_chunk32` | 268.88 | 0.55 | 32 | generated | 2699 |
| 80 MHz | `tile_80x60_chunk60` | 267.62 | 2.56 | 60 | generated | 2963 |

## Allocation failures

| Log | Case | Bytes per chunk | Internal heap free |
|---|---|---:|---:|
| `stackchan-rawblit-40-monitor.log` | `full_320x240_chunk240` | 153600 | 231383 |
| `stackchan-rawblit-60-monitor.log` | `full_320x240_chunk240` | 153600 | 231383 |
| `stackchan-rawblit-byteswap-80-monitor.log` | `full_320x240_chunk240` | 153600 | 231383 |
