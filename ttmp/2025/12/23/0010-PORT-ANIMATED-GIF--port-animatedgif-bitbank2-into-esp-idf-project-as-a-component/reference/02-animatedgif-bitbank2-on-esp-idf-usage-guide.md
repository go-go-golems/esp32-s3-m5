---
Title: 'AnimatedGIF (bitbank2) on ESP-IDF: usage guide'
Ticket: 0010-PORT-ANIMATED-GIF
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - animation
    - gif
    - m5gfx
    - assets
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T18:40:21.755909426-05:00
WhatFor: ""
WhenToUse: ""
---

# AnimatedGIF (bitbank2) on ESP-IDF: usage guide

## Goal

This document is a practical, copy/paste-friendly guide for using the vendored **AnimatedGIF (bitbank2)** library inside an **ESP-IDF** application. It explains:

- how the component is packaged and what to include in your project
- what the key APIs return (and the common “gotchas”)
- how to feed GIF bytes from memory (embedded arrays or flash-mapped partitions)
- how to implement `GIFDraw` efficiently for an RGB565 framebuffer (e.g. M5GFX `M5Canvas`)
- how to handle timing/pacing and avoid transfer overlap when presenting frames

The intent is to let a new contributor integrate AnimatedGIF into a fresh ESP-IDF app without re-deriving the porting decisions from ticket history.

## Context

AnimatedGIF is Arduino-first but its core is portable C/C++. In this repo we vendor it as an ESP-IDF component and use it in a "truth harness" project (`esp32-s3-m5/0014-atoms3r-animatedgif-single`) to validate decode → draw → present on AtomS3R using an RGB565 canvas.

Two constraints drive most design choices:

- **Memory model**: we want to decode from *memory-backed GIF bytes* (eventually from flash-mapped partitions via `esp_partition_mmap`), not from a filesystem.
- **Presentation correctness**: we must not modify a DMA-presented framebuffer while the transfer is still in flight (`waitDMA()` style guardrail).

The first constraint is practical: flash-mapped data is cheap and fast on ESP32, and avoids the overhead of a filesystem for small assets. The second constraint is critical for animation quality — if you render into a buffer while it's still being transmitted via DMA, you'll see tearing or "flutter" artifacts. Understanding these two constraints helps you avoid common pitfalls when integrating AnimatedGIF into your project.

## Quick Reference

This section is meant to be “paste into your project” material: key API contracts, the minimum setup, and the most common failure modes.

### Where the component lives (in this repo)

AnimatedGIF is vendored as a normal ESP-IDF component inside the harness project:

- `esp32-s3-m5/0014-atoms3r-animatedgif-single/components/animatedgif/`

The upstream revision is pinned (see `components/animatedgif/README.md`) and the upstream license is preserved as `components/animatedgif/LICENSE` (Apache-2.0).

This vendoring approach means we control the exact revision we compile against, which is critical for embedded systems where "latest" can introduce breaking changes or resource regressions. The component directory includes only the essential source files from upstream (`AnimatedGIF.h`, `AnimatedGIF.cpp`, `gif.inl`, plus `GIFPlayer.h` for optional file-based playback) along with a minimal `CMakeLists.txt` that declares ESP-IDF dependencies like `esp_timer` and `freertos`.

### Add the component to an ESP-IDF project

You have two common options for bringing this component into your own ESP-IDF application, depending on whether you're building a standalone project or working in a monorepo.

1) **Copy/vendor the component into your project**:

- `components/animatedgif/...` (CMakeLists + `src/*.cpp`/`gif.inl`)

2) **Reference the component via `EXTRA_COMPONENT_DIRS`** (monorepo-style):

In your project `CMakeLists.txt`:

```cmake
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/path/to/components/animatedgif"
)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(my_app)
```

In your app component `CMakeLists.txt`:

```cmake
idf_component_register(
  SRCS "main.cpp"
  REQUIRES animatedgif
)
```

### Include + instantiate

```cpp
#include "AnimatedGIF.h"

static AnimatedGIF gif;
```

### Core API contracts (don't guess these)

Understanding AnimatedGIF's return semantics is critical because they don't follow a single consistent pattern. The library evolved from Arduino/desktop origins where "1 means success" is common, but it also uses negative error codes and `GIF_SUCCESS == 0` in its error enum. This mix can cause confusion if you assume a uniform error-handling model. The most reliable approach is to treat `open()` and `playFrame()` separately, using their documented return contracts and always checking `getLastError()` when validation is needed.

AnimatedGIF uses a mix of boolean-like returns and error codes. The two most important contracts:

- **`open(...)` returns 1 on success, 0 on failure** (it is *not* a `GIF_*` error code)
- **`playFrame(...)`**:
  - returns `>0` when frames remain
  - returns `0` at end-of-file (EOF)
  - returns `<0` on error
  - may return `0` **even if it successfully decoded a frame**; check `getLastError()` and present at least once when it is `GIF_SUCCESS`

### Decode loop (timing is owned by you)

AnimatedGIF provides frame decoding and palette management, but it intentionally leaves timing and presentation control in your hands. This design choice makes sense for embedded systems where you might want to pace frames according to available CPU, synchronize with a display refresh, or adjust playback speed based on user input. The library gives you a delay suggestion via the `playFrame()` output parameter, but you're free to ignore it or scale it. The only requirement is that you call `playFrame()` repeatedly to advance through the animation.

The typical pattern is:

```cpp
gif.begin(GIF_PALETTE_RGB565_LE);     // or GIF_PALETTE_RGB565_BE depending on your pipeline
int ok = gif.open(gif_bytes, gif_size, GIFDraw);
if (!ok) {
  int err = gif.getLastError();
  // log err
}

while (true) {
  int delay_ms = 0;
  int prc = gif.playFrame(false, &delay_ms, user_ctx);
  int last_err = gif.getLastError();

  if (prc > 0 || last_err == GIF_SUCCESS) {
    // present your framebuffer/canvas here
    if (delay_ms < 10) delay_ms = 10;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }

  if (prc == 0) {
    // restart animation
    gif.reset();
    vTaskDelay(pdMS_TO_TICKS(1));
  } else if (prc < 0) {
    // error path
    gif.reset();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
```

### `GIFDraw` (RAW mode) contract

The `GIFDraw` callback is where your application integrates with the decoder. AnimatedGIF decodes GIF data line-by-line and invokes this callback for each scanline (or segment, if the frame uses interlacing). The callback receives palette indices and a palette, giving you full control over how pixels are written into your framebuffer. This scanline-based approach is memory-efficient: the library doesn't need to allocate a full decoded frame, and you can perform per-line transformations (like scaling or color swapping) without an intermediate buffer. Understanding the `GIFDRAW` structure is essential because misinterpreting transparency or coordinate fields leads to visual artifacts.

In RAW mode (`GIF_DRAW_RAW`, default), the decoder calls:

- `pDraw->pPixels`: palettized indices for this scanline/segment
- `pDraw->pPalette`: RGB565 palette entries (endianness depends on `gif.begin(...)`)
- `pDraw->ucHasTransparency` / `pDraw->ucTransparent`: transparency info
- `pDraw->iX`, `pDraw->iY`, `pDraw->y`, `pDraw->iWidth`: where this line belongs

Your job is to write pixels into your framebuffer/canvas.

### Flash-mapped input (what you want eventually)

For production use, storing GIF assets directly in a flash partition is more practical than embedding them as C arrays in your firmware binary. ESP-IDF's partition mapping API (`esp_partition_mmap`) lets you treat flash memory as if it were normal RAM — you get a pointer and can pass it directly to AnimatedGIF's memory-backed `open()`. This approach avoids copying data into RAM and lets you ship multiple large GIFs without bloating your firmware image. The main consideration is that the mapping must remain active for the entire lifetime of playback; unmapping early will cause the decoder to read invalid data and likely crash or produce corrupted frames.

If you store GIF bytes in a custom partition, you can memory-map them and pass the pointer to `open()`:

```cpp
const esp_partition_t* part = esp_partition_find_first(...);
spi_flash_mmap_handle_t mmap_handle;
const void* mapped = nullptr;
ESP_ERROR_CHECK(esp_partition_mmap(part, 0, part->size, SPI_FLASH_MMAP_DATA, &mapped, &mmap_handle));

gif.open((uint8_t*)mapped, part->size, GIFDraw);
// keep mapping alive while playing
```

Real-world implication: the mapping must remain valid for the lifetime of playback (don’t `munmap` until you stop).

## Usage Examples

This section shows two concrete integration patterns: (1) a minimal "embedded array" harness, and (2) a "flash-mapped bytes" pattern. Both examples use the RAW mode (`GIF_DRAW_RAW`) because it gives you complete control over pixel placement and color conversion, which is essential when you need to apply transformations like scaling or byte-swapping. The examples focus on RGB565 output because that's the most common format for microcontroller LCD panels, and AnimatedGIF's palette-to-RGB565 path is already optimized for this use case.

### Example 1: Minimal memory-backed playback into an RGB565 canvas

This example mirrors the repo's AtomS3R harness shape: decode from a `const uint8_t[]` and draw into a 16-bit canvas buffer. The pattern is intentionally minimal to highlight the essential integration points: initialize the decoder with a palette type, open the GIF from memory, and implement a `GIFDraw` callback that translates palette indices into RGB565 pixels while respecting transparency.

Key idea: your present path must be safe — do not render into the same buffer while the previous DMA transfer is still active.

```cpp
typedef struct {
  uint16_t* fb565;
  int fb_w;
  int fb_h;
} Ctx;

static void GIFDraw(GIFDRAW* d) {
  Ctx* ctx = (Ctx*)d->pUser;
  int y = d->iY + d->y;
  if ((unsigned)y >= (unsigned)ctx->fb_h) return;

  uint16_t* row = &ctx->fb565[y * ctx->fb_w];
  const uint8_t* src = d->pPixels;
  const uint16_t* pal = d->pPalette;

  if (d->ucHasTransparency) {
    uint8_t t = d->ucTransparent;
    for (int i = 0; i < d->iWidth; i++) {
      uint8_t idx = src[i];
      if (idx != t) row[d->iX + i] = pal[idx];
    }
  } else {
    for (int i = 0; i < d->iWidth; i++) {
      row[d->iX + i] = pal[src[i]];
    }
  }
}
```

### Example 2: Flash-mapped partition input (recommended for "bundled assets")

This pattern matches the eventual GIF-console architecture: put GIF bytes into a known partition and `mmap` it. Flash-mapped input is the recommended production approach for embedded devices because it avoids consuming precious RAM for asset storage. ESP32's memory-mapped flash feature lets you access flash contents as if they were in addressable memory, which means AnimatedGIF's internal read operations become simple pointer dereferencing rather than filesystem calls or explicit SPI transactions. This approach is both fast and memory-efficient, making it ideal for shipping multiple animations in a single device image.

```cpp
const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "animpack");
if (!part) { /* log + fail */ }

spi_flash_mmap_handle_t h;
const void* p = nullptr;
ESP_ERROR_CHECK(esp_partition_mmap(part, 0, part->size, SPI_FLASH_MMAP_DATA, &p, &h));

AnimatedGIF gif;
gif.begin(GIF_PALETTE_RGB565_LE);
if (!gif.open((uint8_t*)p, part->size, GIFDraw)) {
  /* log gif.getLastError() */
}
```

## Troubleshooting (common gotchas)

These issues showed up repeatedly while porting and validating on hardware. Most of them stem from subtle mismatches between what the library expects and what your hardware/display pipeline provides. The good news is that once you understand the root causes, these problems are straightforward to fix. The most common issues fall into three categories: color/byte-order mismatches, API contract misunderstandings, and FreeRTOS task management in tight loops.

### Colors are "wrong" (greens look purple)

This is almost always a byte-order / pixel-format mismatch. RGB565 is a 16-bit format where red occupies the high 5 bits, green the middle 6 bits, and blue the low 5 bits, but the byte order can vary depending on whether your display hardware expects little-endian or big-endian values. Different SPI displays have different requirements, and getting this wrong produces the classic "color swap" symptom where reds and blues are inverted.

- Try switching `gif.begin(GIF_PALETTE_RGB565_LE)` ↔ `gif.begin(GIF_PALETTE_RGB565_BE)`.
- If you have a known-good RGB565 pipeline (e.g. M5GFX expects CPU-native `uint16_t`), ensure you're not feeding it MSB-first values.

For AtomS3R (GC9107), we use `GIF_PALETTE_RGB565_LE` and then apply `__builtin_bswap16()` in our `GIFDraw` callback because M5GFX's internal buffer expects CPU-native byte order but our SPI transfer ends up needing the swap. Test with a known image (like one with obvious green areas) to quickly validate your byte order.

### "open failed: rc=1"

That message is a harness bug. `open()` returns 1 on success. If you log a return value, treat it as boolean.

### Watchdog triggers / IDLE starves

If your playback loop can hit EOF rapidly (single-frame GIF, zero delay), you must still yield:

- Always `vTaskDelay(1)` on the EOF/reset path.

This issue appears when testing with static or single-frame GIFs during development. Without any delay, your playback task spins continuously decoding and resetting the same frame, which starves the FreeRTOS IDLE task. The IDLE task is responsible for feeding the task watchdog, so starvation triggers a watchdog reset. Even a 1-tick delay is sufficient to let the scheduler service other tasks and prevent this failure mode.

## Resources

These links provide deeper technical context for the concepts mentioned in this guide. The upstream AnimatedGIF repository is the authoritative source for API details and usage patterns, while the ESP-IDF documentation covers partition management and build system integration.

- Upstream library: `https://github.com/bitbank2/AnimatedGIF`
- ESP-IDF application structure: `https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/build-system.html`
- Flash partition mmap: `https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/storage/spi_flash.html#memory-mapping-api`

## Related

- Ticket `0010` starting map:
  - `analysis/02-starting-map-files-symbols-and-integration-points.md`
- Harness project (reference implementation):
  - `esp32-s3-m5/0014-atoms3r-animatedgif-single/`
