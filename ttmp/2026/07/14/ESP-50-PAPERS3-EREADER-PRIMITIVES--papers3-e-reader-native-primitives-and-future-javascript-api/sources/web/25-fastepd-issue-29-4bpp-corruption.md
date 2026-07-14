# bitbank2/FastEPD issue #29: 4‑bpp buffer calculation errors cause rendering artifacts and heap corruption on 960×540 displays

- URL: https://github.com/bitbank2/FastEPD/issues/29
- State: `closed`
- Created: 2026-04-07T05:09:43Z
- Updated: 2026-04-07T22:12:19Z
- Author: `jetsharklambo`

## Issue body

# FastEPD v1.3.0: 4‑bpp buffer calculation errors on 960×540 panels

## Summary

FastEPD v1.3.0 appears to have systematic buffer calculation errors in
4‑bit‑per‑pixel (4‑bpp) mode on 960×540 displays. In several places the code
uses a `/4` divisor (appropriate for 2‑bpp) where `/2` is required for 4‑bpp.
This leads to:

- A visible duplicate vertical strip on the right edge of the display.
- Buffer under‑allocation and out‑of‑bounds accesses.
- Heap corruption warnings and occasional instability, especially after Wi‑Fi
  activity.

I’ve identified and patched the affected locations in `src/FastEPD.inl` and
verified that the fixes remove both the visual artifact and the heap issues on
real hardware.

## Affected hardware

- LilyGo T5 4.7" S3 Pro (960×540, ESP32‑S3) — confirmed affected.
- M5Stack PaperS3 (960×540, ESP32‑S3) — very likely affected due to identical
  resolution and 4‑bpp usage.[web:2]

Any 960‑wide panel driven in 4‑bpp mode via FastEPD is likely to see the same
behavior.

## Visible symptom

When rendering full‑screen content on a 960×540 display in 4‑bpp mode, the
display shows a narrow vertical strip (roughly 5–10 pixels wide) along the
right edge that appears to repeat content from earlier in the framebuffer.


Switching to 1‑bpp mode with otherwise similar drawing code does not show this
artifact, which points directly at the packed 4‑bpp path.

## Heap / stability symptom

Under sustained 4‑bpp full‑screen updates with Wi‑Fi enabled, the ESP32 heap
checker occasionally reports corrupt tails in allocations, for example:

```text
CORRUPT HEAP: Bad tail at 0x3fcXXXXX. Expected 0xbaad5678 got 0xXXXXXXXX
```

In my tests this sometimes coincides with failures entering deep sleep after
Wi‑Fi operations. Running the same workload in 1‑bpp mode does not reproduce
these issues.

## Reproduction (simplified)

Hardware:

- LilyGo T5 4.7" S3 Pro (or similar 960×540 e‑paper panel).

Software:

- FastEPD v1.3.0 as published in the Arduino / PlatformIO ecosystem.

Sketch outline:

```cpp
#include <FastEPD.h>

FASTEPD epaper;

void setup() {
    epaper.initPanel(BB_PANEL_M5PAPERS3);       // or equivalent T5 S3 panel
    epaper.setMode(BB_MODE_4BPP);              // 4-bit grayscale
    epaper.fillScreen(0x0);                    // black
    // draw a gradient or other full-screen test pattern
    // ...
    epaper.fullUpdate(CLEAR_NONE, false);
}

void loop() {
    // optionally, repeat full-screen updates and Wi-Fi connect/disconnect
}
```

Expected result: clean image across the full 960‑pixel width.

Actual result: narrow repeated strip at the right edge; under repeated redraws
and Wi‑Fi cycles, occasional heap corruption messages.

## Root cause analysis (high‑level)

Looking at `bbepSetPanelSize` in `src/FastEPD.inl`:

```c
pState->width = pState->native_width = width;
pState->height = pState->native_height = height;

pState->pCurrent = (uint8_t *)heap_caps_aligned_alloc(
    16,
    (pState->width * pState->height) / 2,
    MALLOC_CAP_SPIRAM
); // current pixels

if (!pState->pCurrent) return BBEP_ERROR_NO_MEMORY;

if (pState->iPanelType == BB_PANEL_VIRTUAL) {
    return BBEP_SUCCESS;
}

pState->pPrevious = &pState->pCurrent[(width/4) * height]; // comment: "only 1-bpp mode"
pState->pTemp = (uint8_t *)heap_caps_aligned_alloc(
    16,
    (pState->width * pState->height) / 4,
    MALLOC_CAP_SPIRAM
);
```

Key facts:

- For 4‑bpp, each byte holds 2 pixels, so a 960‑pixel line requires 480 bytes.
- `pCurrent` is allocated as `(width * height) / 2`, which matches
  `width/2 * height` for 4‑bpp.
- The 4‑bpp graphics code (for example `bbepFillScreen`) also uses a pitch of
  `(width + 1)/2` bytes per line, consistent with 4‑bpp.

However:

- `pPrevious` is positioned at an offset of `(width/4) * height` within the
  `pCurrent` allocation.
- Several 4‑bpp code paths index into `pCurrent` and `pTemp` using
  `native_width/4` for row offsets and copy sizes, as if the framebuffer were
  2‑bpp instead of 4‑bpp.

On a 960‑wide 4‑bpp panel this effectively halves the assumed bytes‑per‑line
when stepping through the framebuffer, which explains both:

- The right‑edge visual corruption (writes going past the intended end of each
  line into adjacent memory), and
- The heap corruption when those overruns hit the next allocation.

## Concrete calculation for 960×540, 4‑bpp

For 960×540 at 4‑bpp:

- Correct bytes per line: `960 / 2 = 480`.
- Correct framebuffer size: `480 * 540 = 259,200` bytes.

Using `width/4` in places that operate on 4‑bpp data treats each line as if it
were only 240 bytes wide. That is a 50% underestimation of the line length and
is consistent with both the artifact and the heap issues observed.

## Affected code patterns

In `src/FastEPD.inl` for v1.3.0 there are multiple patterns of the form:

- Offsets into `pCurrent` or `pTemp` using `native_width/4` when the surface
  is 4‑bpp.
- `memset` / `memcpy` and loop bounds using `width/4` bytes per line in 4‑bpp
  code paths.
- `pPrevious` being carved out of the `pCurrent` block at `(width/4) * height`.

In my local patched version, all of these are normalized so that:

- 4‑bpp code uses `width/2` as the line pitch for any operation that walks 4‑bpp
  pixel data.
- The previous buffer either has its own allocation, or is placed after the
  entire 4‑bpp plane (i.e. after `width/2 * height` bytes), so it no longer
  overlaps the active framebuffer region.

## Proposed direction for a fix

I see two straightforward options that preserve existing behavior for 1‑bpp and
2‑bpp:

1. **Separate previous buffer**

   - Allocate `pPrevious` independently with enough space for the 1‑bpp plane,
     e.g. `((width + 7)/8) * height` bytes.
   - Keep `pCurrent` as a pure 4‑bpp framebuffer of `(width * height) / 2`
     bytes.
   - Ensure all 4‑bpp paths that step through `pCurrent` use `width/2` for
     pitch.

2. **Single contiguous block, adjusted layout**

   - Allocate a single block of size:
     - `curSize = (width * height) / 2` (4‑bpp),
     - `prevSize = ((width + 7)/8) * height` (1‑bpp previous plane),
   - Set `pCurrent` to the start of the block and `pPrevious` to
     `pCurrent + curSize`.
   - As above, ensure 4‑bpp code uses `width/2` consistently for line pitch.

In both cases, 1‑bpp and 2‑bpp behavior is unchanged; the only change is to
make the 4‑bpp buffer math internally consistent and avoid overlapping the
previous buffer with the active 4‑bpp framebuffer.

In my fork, I applied the second option with a small set of `/4 → /2` changes
in the 4‑bpp code paths and the correct offset for `pPrevious`. The resulting
build:

- No longer shows the right‑edge artifact on 960×540.
- Runs 72+ hours of continuous 4‑bpp full‑screen updates with Wi‑Fi and deep
  sleep cycles without heap corruption or crashes.

## Reference implementation

A working implementation of these patches is available here:

- https://github.com/jetsharklambo/TRMNL-T5S3-firmware

Relevant files:

- `lib/FastEPD/src/FastEPD.inl` — patched 4‑bpp buffer logic.
- `lib/FastEPD/PATCHES.md` — notes on each change and its rationale.

I’m happy to extract this into a focused PR against FastEPD with a minimal,
readable diff.

## Request

Would you be open to a pull request that:

- Leaves 1‑bpp and 2‑bpp behavior unchanged.
- Corrects the 4‑bpp buffer layout and pitch usage for 960×540 panels.
- Has been tested on real T5 S3 Pro hardware (and is applicable to M5Stack
  PaperS3 as well)?

If you prefer a particular layout for the previous buffer (single block vs
separate allocation), I’m happy to adjust the patch to match your design.

## Comments

### Comment 1: bitbank2 at 2026-04-07T15:25:05Z

Permalink: https://github.com/bitbank2/FastEPD/issues/29#issuecomment-4200144479

This problem is unrelated to memory allocation size or incorrect pitch calculation. The previous buffer is only for doing differential updates in 1 and 2-bit mode; 4-bit mode shouldn't access this pointer. Can you share your test sketch in its entirety? I don't see anywhere in the code which improperly writes to that pointer in 4-bit mode. Are you calling bbepBackupPlane()? I need to add a check in that function to make sure it's not called in 4-bit mode.

### Comment 2: jetsharklambo at 2026-04-07T18:40:46Z

Permalink: https://github.com/bitbank2/FastEPD/issues/29#issuecomment-4201390569

Thanks for looking into this!

To answer your first question, in my code I’m in 4‑bpp mode and calling clearWhite(true). That ends up calling backupPlane(), which calls bbepBackupPlane(), so we are hitting bbepBackupPlane() in 4‑bpp via clearWhite().

Here's exactly what I call:

```cpp
// 1. Hardware initialization
bbep.initPanel(BB_PANEL_EPDIY_V7);
bbep.setPanelSize(960, 540, BB_PANEL_FLAG_NONE);
bbep.setCustomMatrix(u8M5Matrix, sizeof(u8M5Matrix));

// 2. Set 4-bpp mode
bbep.setMode(BB_MODE_4BPP);

// 3. Pre-clear display (THIS CALLS bbepBackupPlane!)
bbep.clearWhite(true);  // <-- Problem is here

// 4. Render PNG via callback
png.decode(NULL, 0);
// Inside callback: bbep.drawPixelFast(x, y, pixel);

// 5. Update display
bbep.fullUpdate(CLEAR_NONE, false);
```

I'm now convinced the issue is `clearWhite()` calls `backupPlane()` which calls `bbepBackupPlane()`, even in 4-bpp mode. Forget all the divisor shenanigans I thought it was earlier.

One the 960x540 screens, the math would look like this:

```cpp
void bbepBackupPlane(FASTEPDSTATE *pState) {
    int iSize = (pState->native_width/2) * pState->native_height;
    if (!pState->pPrevious || !pState->pCurrent) return;
    memcpy(pState->pPrevious, pState->pCurrent, iSize);
}
```

For 960×540 in 4‑bpp mode:

- iSize = (960/2) * 540 = 259,200 bytes to copy.
- pCurrent is allocated as 259,200 bytes.
- pPrevious points inside pCurrent at offset (960/4) * 540 = 129,600 bytes.
- That means only 129,600 bytes remain from pPrevious to the end of pCurrent.
- Copying 259,200 bytes from that offset overruns the end of pCurrent by 129,600 bytes, which explains the heap corruption and visual artifacts.


Your suggestion to add a check in `bbepBackupPlane()` would fix it:

```cpp
void bbepBackupPlane(FASTEPDSTATE *pState) {
    // Guard against 4-bpp mode
    if (pState->mode == BB_MODE_4BPP) {
        return;  // Previous buffer not used in 4-bpp
    }

    int iSize = (pState->native_width/2) * pState->native_height;
    if (!pState->pPrevious || !pState->pCurrent) return;
    memcpy(pState->pPrevious, pState->pCurrent, iSize);
}
```

That would prevent `clearWhite()`/`clearBlack()` from corrupting the buffer when called in 4-bpp mode.

Happy to test this change on my hardware if you want more evidence. Hoping to push the firmware to the main repo once this looks good!



### Comment 3: bitbank2 at 2026-04-07T18:51:15Z

Permalink: https://github.com/bitbank2/FastEPD/issues/29#issuecomment-4201450606

Thanks for finding the root cause. I just pushed the fix; please let me know if it resolves the issue and then you can close this.

### Comment 4: jetsharklambo at 2026-04-07T22:12:19Z

Permalink: https://github.com/bitbank2/FastEPD/issues/29#issuecomment-4202507958

Can confirm the fix is working on the t5s3 hardware! Thanks for the prompt resolution here. Gonna close this out and get this firmware out.
