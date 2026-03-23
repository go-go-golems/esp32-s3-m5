---
title: PaperS3 E-Reader - Interactive Book Reader on E-Ink
aliases:
  - PaperS3 E-Reader
  - E-Ink Book Reader
  - SPIFFS E-Reader
tags:
  - project
  - esp32-s3
  - papers3
  - firmware
  - e-reader
  - e-ink
  - layout-engine
  - spiffs
  - touch
status: blocked
type: project
created: 2026-03-22
repo: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
firmware: 0080-papers3-ereader
blocked-by: ESP-37-EREADER-EPD-CRASH
---

# PaperS3 E-Reader -- Interactive Book Reader on E-Ink

This project builds an interactive plain-text e-reader on the M5Paper S3 (960x540 e-ink, capacitive touch, ESP32-S3) using the [[PROJ - Gnosis Layout Engine - PaperS3 UI Operating System|Gnosis layout engine]] from firmware 0078 as its UI foundation. It reads `.txt` books from the device's SPIFFS filesystem, paginates them with word wrapping, supports touch-driven page turns, maintains a library screen with all loaded books, and persists reading positions across power cycles.

The firmware is `0080-papers3-ereader`. It builds cleanly and the application logic is complete, but it is currently **blocked** by a null-pointer crash in the M5GFX EPD panel driver (ticket ESP-37-EREADER-EPD-CRASH) that prevents the display from rendering. The bug is in the display driver layer, not in the e-reader application code.

> [!summary]
> The e-reader has four modules on top of the Gnosis engine:
> - **BookStore**: SPIFFS mount, `books.idx` metadata index, random-access file reads
> - **Paginator**: word-wrap algorithm, incremental page offset table, formatted text output
> - **BookmarkStore**: persistent reading positions, auto-flush every 10 page turns
> - **EReaderApp**: two screens (library + reading), touch zones, page turns, console REPL
>
> The application compiles, loads books from flash, paginates them correctly, and handles touch input -- but crashes on the first screen draw because the EPD driver's framebuffer pointer is unexpectedly null.

## Why this project exists

The [[PROJ - Gnosis Layout Engine - PaperS3 UI Operating System|Gnosis layout engine]] proved that a tree-based UI with automatic dirty-rect tracking works well on e-ink. But its seven preset screens were static demos with hardcoded text. The e-reader is the first real application built on the engine: it has persistent state, user-driven navigation, filesystem I/O, and dynamic content that changes on every page turn. It tests whether the layout engine is genuinely useful as a foundation for interactive applications, not just demos.

The key engineering challenge was that the Gnosis `Node` struct stores text in a 64-byte inline buffer (`char text[64]`), which is fine for labels but far too small for a page of prose. The solution was adding a `const char* ext_text` pointer to the Node struct -- one 8-byte field that lets the `DrawTextBlock` renderer read from an external buffer instead of the inline text. The page buffer (4 KB) lives in the application class and is refilled from SPIFFS on each page turn.

## Architecture

```
EReaderApp
├── BookStore        (SPIFFS I/O, book index)
├── Paginator        (word-wrap, page offset table)
├── BookmarkStore    (position persistence)
└── Gnosis Engine    (layout, rendering, dirty tracking)
    └── M5GFX        (EPD driver, touch)
```

The application has two screens, both built as Gnosis node trees:

**Library screen**: status bar with book count, a LIST widget showing all books from the index (title, author, page count), touch to select a book.

**Reading screen**: status bar with book title and page number, a full-width TEXT_BLOCK using `ext_text` to render the current page, a nav bar with progress bar and library/bookmark buttons. Touch zones divide the body into left 25% (previous page), center 50% (no action), right 25% (next page).

## The ext_text extension to the Gnosis engine

This was the cleanest possible change to support large text. One field added to `Node`:

```cpp
struct Node {
    // ... existing 64-byte text buffer ...
    char text[64]{};
    const char* ext_text = nullptr;  // NEW: points to external buffer
};
```

And one line changed in the renderer:

```cpp
void DrawTextBlock(M5GFX& display, Node* node) {
    const char* text = node->ext_text ? node->ext_text : node->text;
    // ... rest unchanged
}
```

Every existing node type works unchanged. Only the TEXT_BLOCK used for book pages needs the external pointer. The page buffer is a `char[4096]` in the application class, refilled from SPIFFS on each page turn and pointed to by `text_node_->ext_text`.

## The word-wrap paginator

The paginator answers "what text fits on this page?" using a two-pass approach:

**Pass 1 -- `PaginateOnePage`**: scans forward through raw text from a given byte offset, counting lines and respecting word boundaries. Single newlines become spaces (soft wrap in the source file). Double newlines become paragraph breaks (blank line). Words longer than a line are force-broken. Returns the byte offset where the page ends.

**Pass 2 -- `FormatText`**: takes the raw text for one page and re-wraps it into a buffer with explicit `\n` characters at the wrap points, ready for the `DrawTextBlock` renderer which splits on newlines.

Page breaks are cached in an offset table:

```
page_offsets[0] = 0        // start of file
page_offsets[1] = 847      // end of page 1 in bytes
page_offsets[2] = 1692     // end of page 2
...
```

The table is built incrementally as the user reads forward. Going backward is free -- the offset is already cached. On first open, `ComputeTotalPages()` paginates the entire file to get the page count, which is then saved to `books.idx` so subsequent boots skip the scan.

## Book storage on SPIFFS

Books are plain UTF-8 `.txt` files stored on a 512 KB SPIFFS partition. The filesystem is baked into the firmware image at build time using `spiffs_create_partition_image()`.

```
/spiffs/
├── books.idx          # ereader-index-v1\nfilename|title|author|pages
├── bookmarks.dat      # ereader-bookmarks-v1\nfilename|offset|page
└── sample.txt         # plain text book (~2.8 KB sample)
```

`BookStore::ReadChunk(filename, offset, length, buffer)` does random-access reads into the book file using `fseek` + `fread`. This is how the paginator feeds text to the page buffer without loading the entire book into RAM.

## Bookmark persistence

`BookmarkStore` saves reading positions to `/spiffs/bookmarks.dat`. On every page turn, `UpdatePosition()` records the current page number. To avoid wearing out the flash, the store only flushes to disk every 10 page turns. On `OpenBook()`, the last saved position is restored.

## Console commands

The firmware runs an `esp_console` REPL on the USB Serial/JTAG port:

```
ereader> list
library (1 books):
  [0] The Deliverator - Neal Stephenson (4 pages, 2847 bytes) *

ereader> info
book: The Deliverator by Neal Stephenson
page: 1/4
file: sample.txt (2847 bytes)
nodes: 14/192

ereader> page 3
jumped to page 4
```

## The crash: ESP-37-EREADER-EPD-CRASH

The firmware boots, mounts SPIFFS, loads the book index, then crashes when it tries to draw the first screen. The crash is in `Panel_EPD::writeFillRectPreclipped` -- the EPD driver's internal framebuffer pointer (`_buf`) is null.

```
lgfx_epd_dbg: fillrect ... buf=0x0 ...
Guru Meditation Error: Core 1 panic'ed (LoadProhibited)
EXCVADDR: 0x00000000
```

This is puzzling because firmware 0078 (the Gnosis layout engine) uses the identical initialization pattern and works fine. Two fix attempts were made:

1. **Stack overflow fix** (commit `d0d557a`): the paginator had two 8 KB stack buffers that blew the task stack. Made them `static` and increased the stack to 16 KB. This fixed the stack overflow but revealed the real crash underneath.

2. **Move M5.begin to core 0** (commit `5b1e146`): split `Run()` into `Init()` on the main task and `RunLoop()` on core 1. Did not fix the crash -- the `_buf` is still null.

The investigation diary in ticket ESP-37 documents six ranked leads for the embedded engineer to pursue, starting with tracing the lazy buffer allocation path in `Panel_EPD::init()` / `post_init()`.

## Important files

| File | Purpose |
|------|---------|
| `0080-papers3-ereader/main/ereader_app.cpp` | Main application: screens, touch, page turns |
| `0080-papers3-ereader/main/book_store.cpp` | SPIFFS mount, index parsing, file reads |
| `0080-papers3-ereader/main/paginator.cpp` | Word-wrap algorithm, page offset table |
| `0080-papers3-ereader/main/bookmark_store.cpp` | Reading position persistence |
| `0080-papers3-ereader/main/gnosis_types.h` | Node struct with `ext_text` addition |
| `0080-papers3-ereader/main/widget_renderer.cpp` | Updated `DrawTextBlock` using `ext_text` |
| `0080-papers3-ereader/main/ereader_console.cpp` | Console REPL commands |
| `0080-papers3-ereader/spiffs_data/sample.txt` | Sample book text |

## Relationship to prior work

This firmware builds directly on:

- [[PROJ - Gnosis Layout Engine - PaperS3 UI Operating System]]: the tree-based layout engine, node pool, dirty-rect tracker, and widget renderer are copied from 0078 into 0080 with the `ext_text` extension
- [[PROJ - Glyph Protractor Algorithm - PaperS3 Handwriting Recognition]]: the SPIFFS mount pattern in `BookStore` is adapted from `GlyphStore` in 0077

## Commits

| Hash | Message |
|------|---------|
| `2f0a3eb` | feat(papers3): scaffold e-reader firmware with Gnosis engine and ext_text |
| `eaf00a3` | feat(ereader): add SPIFFS book storage, word-wrap paginator, and sample book |
| `8fc50b7` | feat(ereader): add bookmark persistence with auto-save every 10 page turns |
| `d0d557a` | fix(ereader): resolve stack overflow by making read buffers static |
| `5b1e146` | fix(ereader): init display on main task to avoid null framebuffer crash |

## What comes after the crash fix

Once the EPD framebuffer issue is resolved, the e-reader is functionally complete for a first demo. Follow-up work:

- Font size switching via console command (repaginate on change)
- Loading additional books via serial or larger SPIFFS image
- Edge case testing (empty files, very long words, Unicode)
- Periodic full EPD refresh to clear ghosting during reading
