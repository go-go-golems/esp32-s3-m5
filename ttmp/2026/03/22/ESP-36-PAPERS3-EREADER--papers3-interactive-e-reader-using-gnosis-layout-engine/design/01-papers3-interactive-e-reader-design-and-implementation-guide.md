---
Title: PaperS3 Interactive E-Reader - Design and Implementation Guide
Ticket: ESP-36-PAPERS3-EREADER
Status: active
Topics:
    - papers3
    - display
    - esp-idf
    - esp32s3
    - e-paper
    - layout-engine
    - e-reader
    - touch
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/screens.cpp:BuildReader preset - the starting point for the reader UI"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/gnosis_types.h:Node, NodePool, TEXT_BLOCK, LIST data structures"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/widget_renderer.cpp:DrawTextBlock, DrawList rendering"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/node_builder.h:Builder helpers for constructing screen trees"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0078-papers3-gnosis-layout/main/gnosis_app.cpp:Main loop, touch handling, dirty refresh"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti/main/glyph_store.cpp:Reference SPIFFS mount/read/write pattern"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/:Target firmware directory"
ExternalSources: []
Summary: "Complete design and implementation guide for building an interactive e-reader on the M5Paper S3 using the Gnosis layout engine from 0078. Covers text storage, pagination, touch navigation, library management, bookmarks, and the rendering pipeline."
LastUpdated: 2026-03-22T11:06:01.05905239-04:00
WhatFor: "Intern onboarding: explains every subsystem needed to build the e-reader, from SPIFFS text storage through pagination math to touch-driven page turns."
WhenToUse: "When implementing or extending the PaperS3 e-reader firmware."
---

# PaperS3 Interactive E-Reader -- Design and Implementation Guide

## Table of Contents

1. [What We Are Building](#1-what-we-are-building)
2. [What Already Exists](#2-what-already-exists)
3. [The Core Problem: Text Does Not Fit in a Node](#3-the-core-problem-text-does-not-fit-in-a-node)
4. [Architecture Overview](#4-architecture-overview)
5. [Book Storage on SPIFFS](#5-book-storage-on-spiffs)
6. [The Book Index](#6-the-book-index)
7. [Pagination Engine](#7-pagination-engine)
8. [Screen Layouts](#8-screen-layouts)
9. [Touch Interaction Model](#9-touch-interaction-model)
10. [Bookmark and Progress Persistence](#10-bookmark-and-progress-persistence)
11. [Console Commands](#11-console-commands)
12. [Extending the Gnosis Engine](#12-extending-the-gnosis-engine)
13. [Data Flow Walkthrough](#13-data-flow-walkthrough)
14. [File Structure and Build System](#14-file-structure-and-build-system)
15. [Implementation Plan](#15-implementation-plan)
16. [API Quick Reference](#16-api-quick-reference)

---

## 1. What We Are Building

An interactive e-reader application for the M5Paper S3 (960x540 e-ink, capacitive touch) that:

- Stores plain-text books on the device's SPIFFS filesystem (512 KB)
- Displays paginated text using the Gnosis layout engine from firmware 0078
- Supports touch-driven page forward/back by tapping the right/left halves of the screen
- Shows a library screen listing all available books with reading progress
- Maintains bookmarks and last-read position across power cycles
- Provides a status bar with book title, page number, and progress
- Allows loading new books and controlling the reader via `esp_console` commands over USB serial

The reader is not a PDF viewer, EPUB parser, or rich-text renderer. It reads plain UTF-8 text files and wraps them at word boundaries into pages that fit the display. This deliberate constraint keeps the implementation small, deterministic, and debuggable.

### Why build this?

The Gnosis layout engine (0078) proved that a tree-based UI with dirty-rect tracking works well on e-ink. But the presets in that firmware are static demos -- hardcoded text, no interaction beyond screen switching. The e-reader is the first *application* built on top of the engine: it has state (current book, current page, bookmarks), user input (page turns, library selection), and persistent data (book files, reading positions). It tests whether the layout engine is actually useful as a foundation for real applications.

---

## 2. What Already Exists

### The Gnosis layout engine (0078-papers3-gnosis-layout)

This firmware provides everything we need for the UI layer:

- **Node tree**: `Node` structs with types VBOX, HBOX, FIXED, LABEL, BAR, LIST, TEXT_BLOCK, SEP, etc.
- **Layout algorithm**: recursive VBOX/HBOX/FIXED flex layout computing positions for all nodes
- **Widget renderer**: draw functions for every node type, using a custom 5x7 bitmap font
- **Dirty-rect tracker**: collects changed regions, merges overlapping rects, issues partial EPD refreshes
- **Node pool**: static allocation of 192 nodes, no heap fragmentation
- **Builder helpers**: `VBox()`, `HBox()`, `Label()`, `TextBlock()`, `List()`, `Bar()`, `Sep()`, etc.
- **Console REPL**: `esp_console` with USB Serial/JTAG for runtime control
- **Touch handling**: polls `M5.Touch`, dispatches to nav bar icons

### The "reader" preset

The engine already includes a reader preset (`BuildReader()` in `screens.cpp`) that demonstrates the layout:

```
┌─────────────────────────────────────────────────────────────┐
│ GNOSIS//3.1                              PWR:EINK        .  │ bar (32px)
├──────────────┬──────────────────────────────────────────────┤
│ LIBRARY      │ READER                                       │
│ 7 VOL        │ SNOW CRASH              size 2               │
│              │ Neal Stephenson  CH 12                        │
│ Neuromancer  │ ─────────────────────────────                 │
│  Gibson  72% │ The Deliverator belongs                       │
│ Snow Crash * │ to an elite order, a                          │
│  Steph.. 45% │ hallowed sub-category.                        │
│ Dune         │ He is a pizza delivery                        │
│  Herbert 100%│ driver.                                       │
│ Solaris      │                                               │
│              │                                               │
│              │ ─────────────────────────────                 │
│              │ P.187/440     [======    ]      BM NT         │
├──────────────┴──────────────────────────────────────────────┤
│ ■  ●  ◇  △                      AUTO    PIXEL MONOSPACED    │ nav (32px)
└─────────────────────────────────────────────────────────────┘
```

The split is 300px sidebar, remainder for content. This is our starting point, but the current implementation has hardcoded text in a 64-byte `node->text` buffer. We need to replace that with a proper text flow system.

### SPIFFS filesystem (from 0077-papers3-alphabet-graffiti)

The `GlyphStore` class in 0077 shows the established pattern for SPIFFS:

```cpp
esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = "storage",
    .max_files = 4,
    .format_if_mount_failed = true,
};
esp_vfs_spiffs_register(&conf);
```

The partition table allocates 512 KB for SPIFFS. That is enough for roughly 500 KB of plain text -- about 5-10 short books or one novel. Files are accessed via standard POSIX `fopen`/`fread`/`fclose`.

---

## 3. The Core Problem: Text Does Not Fit in a Node

The Gnosis `Node` struct stores text inline:

```cpp
static constexpr std::size_t kMaxTextLen = 64;
struct Node {
    // ...
    char text[kMaxTextLen]{};
    // ...
};
```

The `DrawTextBlock` renderer reads from this 64-byte buffer, splitting on `\n` characters. This is fine for a label or a short message, but a page of readable text on a 960x540 display might contain 600-1000 characters (30-40 lines of ~25-30 characters each at size 1, or 15-20 lines of ~50 characters in the wider reading pane).

We cannot simply increase `kMaxTextLen` to 1024 -- every `Node` in the pool carries that buffer, and with 192 nodes that would be 192 KB wasted on nodes that don't use text. The layout engine was designed for compact labels, not flowing prose.

### The solution: external text buffer + custom renderer

Instead of storing text inside the node, we:

1. Keep a **page buffer** (a `char[2048]` or similar) in the application class, separate from the node pool
2. When building a reading screen, point a **custom reader widget node** at this external buffer
3. The renderer for this widget type reads from the external buffer, not from `node->text`
4. When the user turns pages, we refill the page buffer from the SPIFFS book file and mark the reader node dirty

This separates the concerns cleanly: the node tree handles layout and dirty tracking, but the text data lives outside the tree in application-managed memory.

There are two ways to implement this:

**Option A: Extend the Node struct with a `const char*` pointer**

Add a `const char* ext_text` field to Node. The `DrawTextBlock` renderer checks this pointer first, falling back to `node->text` if null. This is the minimal change -- one pointer per node (8 bytes on ESP32), and the renderer gains one `if` check.

```cpp
struct Node {
    // ... existing fields ...
    const char* ext_text = nullptr;  // External text for large content
};
```

**Option B: New READER_PAGE node type**

Add a dedicated `NodeType::READER_PAGE` that always uses external text and supports word wrapping, page numbering, and scroll state. This is cleaner but requires more code.

**Recommendation**: Option A for the initial implementation. It requires the smallest change to the engine and works immediately with the existing `TextBlock` builder. We can always add a dedicated type later if needed.

---

## 4. Architecture Overview

```
┌──────────────────────────────────────────────────────────┐
│                    EReaderApp                             │
│  Library state, current book, page position, bookmarks   │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │  BookStore    │  │  Paginator   │  │  BookmarkStore │  │
│  │  SPIFFS I/O   │  │  Word-wrap   │  │  Position     │  │
│  │  Book index   │  │  Page calc   │  │  persistence  │  │
│  └──────┬───────┘  └──────┬───────┘  └───────┬───────┘  │
│         │                 │                   │          │
├─────────┴─────────────────┴───────────────────┴──────────┤
│                  Gnosis Layout Engine                     │
│  Node tree, layout, dirty tracking, widget rendering     │
├──────────────────────────────────────────────────────────┤
│                  M5GFX / M5Unified                        │
│  EPD driver, touch input, system init                    │
├──────────────────────────────────────────────────────────┤
│                  ESP-IDF 5.x + SPIFFS                     │
└──────────────────────────────────────────────────────────┘
```

The application has three domain-specific modules on top of the Gnosis engine:

1. **BookStore**: manages the filesystem -- mounting SPIFFS, listing available books, reading chunks of text from book files, providing a book index (title, author, file size)
2. **Paginator**: takes a stream of text and a viewport size, computes where page breaks fall using word wrapping, provides "give me page N" as a substring operation
3. **BookmarkStore**: persists the user's last-read position for each book, so they can resume after power cycling

---

## 5. Book Storage on SPIFFS

### File layout

```
/spiffs/
├── books.idx              # Book index file (metadata)
├── book_001.txt           # Plain text book file
├── book_002.txt           # Another book
├── book_003.txt           # ...
└── bookmarks.dat          # Reading positions
```

### Book files

Plain UTF-8 text. No markup, no formatting, no chapters as separate files. Paragraphs are separated by blank lines (`\n\n`). The simplest possible format.

Why not structure chapters? Because SPIFFS has no directories and limited file count. With 512 KB and a 4-file limit on simultaneous opens, keeping each book as one file is the pragmatic choice. Chapter boundaries can be detected by scanning for double newlines or a simple marker like `---`.

### Book index file (`books.idx`)

A simple text file listing available books:

```
ereader-index-v1
book_001.txt|Snow Crash|Neal Stephenson|480
book_002.txt|Neuromancer|William Gibson|312
book_003.txt|Dune|Frank Herbert|650
```

Format: `filename|title|author|total_pages` (one line per book).

The `total_pages` field is pre-computed when a book is loaded (via console command) and cached here. Recomputing it on every boot would require reading the entire file, which is slow on SPIFFS.

### Why not compute the index at boot?

SPIFFS file enumeration is slow and returns filenames in arbitrary order. A pre-built index gives us titles, authors, and page counts without scanning. The index is rebuilt whenever a book is added or removed via console.

### BookStore pseudocode

```
class BookStore:
    Mount():
        esp_vfs_spiffs_register(base_path="/spiffs", partition="storage",
                                 max_files=4, format_if_mount_failed=true)
        report total/used bytes

    LoadIndex():
        open "/spiffs/books.idx"
        read version header "ereader-index-v1"
        for each remaining line:
            parse filename|title|author|pages
            append to books_[] array
        close file

    BookCount() -> int:
        return books_.size()

    GetBook(index) -> BookInfo:
        return books_[index]

    ReadChunk(filename, offset, length, buffer) -> bytes_read:
        open filename
        fseek to offset
        read up to length bytes into buffer
        close file
        return actual bytes read

    SaveIndex():
        write updated books_[] to "/spiffs/books.idx"
```

The key method is `ReadChunk`: it reads a portion of a book file into a buffer. This is how we feed text to the paginator without loading the entire book into RAM.

### Memory budget for text

The ESP32-S3 has 512 KB SRAM and 8 MB PSRAM. We can afford generous buffers:

| Buffer | Size | Location | Purpose |
|--------|------|----------|---------|
| Page buffer | 2 KB | SRAM | Current page text for rendering |
| Read-ahead buffer | 8 KB | PSRAM | Chunk of book file for pagination |
| Book index | ~1 KB | SRAM | Metadata for up to 20 books |
| Bookmark data | ~256 B | SRAM | Last position per book |

Total application memory: ~12 KB, well within budget.

---

## 6. The Book Index

### BookInfo structure

```cpp
struct BookInfo {
    char filename[32];    // e.g. "book_001.txt"
    char title[48];       // e.g. "Snow Crash"
    char author[32];      // e.g. "Neal Stephenson"
    int32_t file_size;    // Total file size in bytes
    int32_t total_pages;  // Pre-computed page count
};
```

### How page count is computed

When a book is loaded (via `ereader load <path>`), the system:

1. Copies the file to SPIFFS
2. Reads the entire file in 4 KB chunks
3. Runs the pagination algorithm (see next section) to count pages
4. Stores the page count in the index

This is a one-time cost per book, not per boot. After the index is written, subsequent boots just read the cached count.

---

## 7. Pagination Engine

Pagination is the heart of the e-reader. It answers two questions:

1. **Forward**: given the current byte offset in the file, what text fits on this page?
2. **Backward**: given the current byte offset, where does the *previous* page start?

### Display geometry

The reading pane occupies the right side of the split layout:

```
Total screen:        960 x 540
Status bar:           32px top
Nav bar:              32px bottom
Usable height:       476px

Split:               280px sidebar + 1px divider + 679px reading pane
Reading pane insets:  16px padding on each side
Text area:           647px wide x 440px tall (leaving room for title bar + footer)

At size 1 (6px per glyph, 8px line height):
    chars_per_line = 647 / 6 = 107 characters
    lines_per_page = 440 / 14 = 31 lines (with 14px line spacing)

At size 2 (12px per glyph, 20px line height):
    chars_per_line = 647 / 12 = 53 characters
    lines_per_page = 440 / 20 = 22 lines
```

For comfortable reading on e-ink, size 1 with 14px line spacing is dense but legible. Size 2 would be more comfortable but holds less text. The paginator should be parameterized so the user can switch.

### Word-wrap algorithm

The paginator wraps text at word boundaries, never splitting a word mid-character. It also respects paragraph breaks (double newlines).

```
function PAGINATE(text, max_chars_per_line, max_lines):
    line_count = 0
    col = 0
    page_start = 0
    page_end = 0
    i = 0

    while i < len(text) and line_count < max_lines:
        if text[i] == '\n':
            if i + 1 < len(text) and text[i+1] == '\n':
                // Paragraph break: skip both newlines, start new line
                line_count += 1  // blank line for paragraph gap
                col = 0
                i += 2
                if line_count < max_lines:
                    line_count += 1
                continue
            else:
                // Single newline: treat as space (soft wrap in source)
                text[i] = ' '

        if text[i] == ' ' and col == 0:
            // Skip leading spaces at start of line
            i += 1
            continue

        // Find next word boundary
        word_start = i
        while i < len(text) and text[i] != ' ' and text[i] != '\n':
            i += 1
        word_len = i - word_start

        if col + word_len > max_chars_per_line:
            if col == 0:
                // Word is longer than a line: force-break it
                page_end = word_start + max_chars_per_line
                col = 0
                line_count += 1
            else:
                // Wrap to next line
                line_count += 1
                col = 0
                i = word_start  // re-process this word on the next line
                continue

        // Word fits on current line
        col += word_len
        if text[i] == ' ':
            col += 1  // account for space
            i += 1
        page_end = i

    return page_end  // byte offset where this page ends
```

### Page offset table

Rather than re-paginating from the start every time, we build a table of page start offsets as the user reads forward:

```cpp
static constexpr int kMaxPages = 2048;

struct PaginationState {
    int32_t page_offsets[kMaxPages];  // byte offset where each page starts
    int32_t pages_computed;            // how many entries are valid
    int32_t chars_per_line;
    int32_t lines_per_page;
};
```

- `page_offsets[0]` = 0 (first page starts at byte 0)
- `page_offsets[1]` = result of paginating from offset 0
- `page_offsets[N]` = result of paginating from offset `page_offsets[N-1]`

When the user turns to page N:

1. If `page_offsets[N]` is already computed, use it directly.
2. If not, paginate forward from the last known offset until page N is reached, filling in the table as we go.

Going backward is free: `page_offsets[N-1]` is already in the table.

This table uses 8 KB for 2048 pages. A typical 500 KB book at ~500 bytes per page would need 1000 entries. The table is allocated in PSRAM.

### Paginator pseudocode

```
class Paginator:
    Init(chars_per_line, lines_per_page):
        state_.chars_per_line = chars_per_line
        state_.lines_per_page = lines_per_page
        state_.page_offsets[0] = 0
        state_.pages_computed = 1

    EnsurePage(book_store, filename, page_number):
        while state_.pages_computed <= page_number:
            offset = state_.page_offsets[state_.pages_computed - 1]
            chunk = book_store.ReadChunk(filename, offset, READ_AHEAD_SIZE)
            end = PAGINATE(chunk, chars_per_line, lines_per_page)
            state_.page_offsets[state_.pages_computed] = offset + end
            state_.pages_computed += 1

    GetPageText(book_store, filename, page_number, out_buffer, buf_size):
        EnsurePage(book_store, filename, page_number + 1)
        start = state_.page_offsets[page_number]
        end = state_.page_offsets[page_number + 1]
        length = min(end - start, buf_size - 1)
        book_store.ReadChunk(filename, start, length, out_buffer)
        out_buffer[length] = '\0'

    TotalPages():
        return state_.pages_computed - 1  // or from cached index
```

### Word-wrapped text for the renderer

The paginator produces a buffer of raw text for one page. Before handing it to the renderer, we need to insert newline characters at the wrap points so `DrawTextBlock` can render it line by line:

```
function FORMAT-PAGE(raw_text, chars_per_line) -> formatted_text:
    // Insert \n at word-wrap points
    out = ""
    col = 0
    i = 0
    while i < len(raw_text):
        if raw_text[i] == '\n':
            out += '\n'
            if i+1 < len(raw_text) and raw_text[i+1] == '\n':
                out += '\n'  // paragraph break
                i += 2
            else:
                i += 1
            col = 0
            continue

        // Find word
        word = next_word(raw_text, i)
        if col + len(word) > chars_per_line and col > 0:
            out += '\n'
            col = 0
        out += word
        col += len(word)
        i += len(word)

        // Space after word
        if i < len(raw_text) and raw_text[i] == ' ':
            if col < chars_per_line:
                out += ' '
                col += 1
            i += 1

    return out
```

This formatted text goes into the page buffer and is pointed to by `node->ext_text`. The renderer draws it as-is, with `\n` controlling line breaks.

---

## 8. Screen Layouts

The e-reader has two main screens, both built using the Gnosis layout engine.

### 8.1 Library Screen

Shown when the user opens the reader or taps the library icon.

```
┌─────────────────────────────────────────────────────────────┐
│ EREADER//1.0                           3 BOOKS        PWR .  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  LIBRARY                                                    │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  Snow Crash                                    45%  │    │
│  │  Neal Stephenson                         P.187/440  │    │
│  ├─────────────────────────────────────────────────────┤    │
│  │  Neuromancer                                   72%  │ *  │
│  │  William Gibson                          P.224/312  │    │
│  ├─────────────────────────────────────────────────────┤    │
│  │  Dune                                        100%  │    │
│  │  Frank Herbert                           P.650/650  │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                             │
│                         [  SELECT  ]                         │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│ ■  ●  ◇  △                      AUTO    PIXEL MONOSPACED    │
└─────────────────────────────────────────────────────────────┘
```

Node tree:

```
Screen
├── bar (HBOX h=32)
│   ├── LABEL "EREADER//1.0"
│   ├── SPACER
│   ├── LABEL "3 BOOKS" color=mid
│   ├── LABEL "PWR" color=mid
│   └── DOT
├── body (FIXED)
│   ├── LABEL "LIBRARY" @(16,16) size=2
│   └── LIST @(40,60) row_h=56 max_items=8
│       ├── row 0: [title] [progress%]
│       │          [author] [page/total]
│       ├── row 1: ...
│       └── row N: ...
└── nav (HBOX h=32)
    └── (standard nav bar)
```

The library list uses the existing LIST widget with two-column rows. Each book entry takes two list rows (title+progress on one line, author+page on the next), so a `row_h` of 56 gives 28px per sub-row, fitting both comfortably.

Alternatively, we can use a single row per book with a taller `row_h` and render the two lines within a single row by adding a second text offset. The simpler approach is to dedicate two list rows per book.

### 8.2 Reading Screen

Shown when a book is open.

```
┌─────────────────────────────────────────────────────────────┐
│ Snow Crash                      P.187/440   45%         .   │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│    The Deliverator belongs to an elite order, a hallowed    │
│    sub-category. He's got esprit up to here. Right now he   │
│    is preparing to deliver a pizza.                         │
│                                                             │
│    His car is a black 1994 Pontiac. A pizza delivery car.   │
│    There's a lot of cars that are a lot better. The         │
│    Deliverator drives the car that people like. He's got    │
│    the touch.                                               │
│                                                             │
│    The DELIVERATOR belongs to an elite order, a hallowed    │
│    subcategory. He's got esprit up to here. Right now,      │
│    he is preparing to deliver a pizza.                      │
│                                                             │
│    ...                                                      │
│                                                             │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│  LIBRARY     [================================        ]  BM │
└─────────────────────────────────────────────────────────────┘
```

This is a full-width reading layout (no sidebar split) to maximize text area:

```
Screen
├── bar (HBOX h=32)
│   ├── LABEL book_title (dynamic)
│   ├── SPACER
│   ├── LABEL "P.187/440" (dynamic)
│   ├── LABEL "45%" (dynamic)
│   └── DOT
├── body (FIXED)
│   └── TEXT_BLOCK @(32,16) w=896 h=440
│       ext_text -> page_buffer_  (external 2KB buffer)
└── nav (HBOX h=32)
    ├── LABEL "LIBRARY" (tap target)
    ├── SPACER
    ├── BAR progress (dynamic)
    ├── SPACER
    └── LABEL "BM" (bookmark button)
```

The text block fills nearly the entire body area (960 - 2*32 = 896 wide, 476 - 36 = 440 tall). At size 1 with 14px line spacing, this gives:

- 896 / 6 = **149 characters per line**
- 440 / 14 = **31 lines per page**
- ~4600 characters per page

That is substantial -- about 800-900 words per page.

### Switching between screens

The `EReaderApp` maintains an enum:

```cpp
enum class Screen { LIBRARY, READING };
```

Switching calls `pool_.Reset()`, builds the appropriate screen tree, runs layout, and does a full-quality EPD refresh (since the entire screen content changes).

---

## 9. Touch Interaction Model

### Reading screen touch zones

The reading screen divides the touch area into three zones:

```
┌─────────────────────────────────────────────────────────────┐
│                         status bar                          │
├──────────┬────────────────────────────────┬─────────────────┤
│          │                                │                 │
│   PREV   │           (ignore)             │      NEXT       │
│   PAGE   │                                │      PAGE       │
│  (25%)   │            (50%)               │     (25%)       │
│          │                                │                 │
├──────────┴────────────────────────────────┴─────────────────┤
│                          nav bar                            │
└─────────────────────────────────────────────────────────────┘
```

- **Left 25%**: previous page
- **Right 25%**: next page
- **Center 50%**: no action (avoids accidental page turns)
- **Nav bar**: "LIBRARY" label returns to library, "BM" saves bookmark

The 25/50/25 split is a common e-reader pattern. It is generous to the forward direction (right-handers naturally tap the right side) while making backward accessible without being too easy to trigger accidentally.

### Touch handling pseudocode

```
HandleTouch():
    if not touch_down and M5.Touch.getCount() > 0:
        touch_down = true
        tx, ty = M5.Touch.getDetail().x, .y

        if current_screen == READING:
            if body.rect.Contains(tx, ty):
                x_pct = (tx - body.rect.x) * 100 / body.rect.w
                if x_pct < 25:
                    PreviousPage()
                else if x_pct >= 75:
                    NextPage()
            else if nav.rect.Contains(tx, ty):
                // Check for LIBRARY or BM tap
                for each child in nav.children:
                    if child.rect.Contains(tx, ty):
                        if child is library_label: SwitchToLibrary()
                        if child is bookmark_label: SaveBookmark()

        else if current_screen == LIBRARY:
            if list_node.rect.Contains(tx, ty):
                // Determine which book was tapped
                row = (ty - list_node.rect.y) / row_h
                book_index = row / 2  // two rows per book
                if book_index < book_count:
                    OpenBook(book_index)

    else if M5.Touch.getCount() == 0:
        touch_down = false
```

### Page turn animation

E-ink does not support smooth animation, but we can make page turns feel responsive:

1. Mark the text node dirty
2. Refill the page buffer with the new page text
3. Update the page counter and progress bar labels
4. `ProcessDirtyRefresh()` redraws only the text area and status labels

The status bar labels (page number, percentage) and the progress bar each occupy small dirty rects that merge naturally with the text area if they're close enough, or refresh as separate small regions if distant. Either way, the total refresh is one or two partial EPD updates covering the text area and the status bar.

---

## 10. Bookmark and Progress Persistence

### Bookmark file (`/spiffs/bookmarks.dat`)

A simple text file:

```
ereader-bookmarks-v1
book_001.txt|18743|42
book_002.txt|54210|174
```

Format: `filename|byte_offset|page_number` per line.

On power-off/power-on, the app loads bookmarks and restores the last page. On every page turn, we update the in-memory bookmark but only flush to SPIFFS periodically (every 10 page turns) to avoid wearing out flash.

### BookmarkStore pseudocode

```
class BookmarkStore:
    struct Bookmark:
        char filename[32]
        int32_t byte_offset
        int32_t page_number

    bookmarks_[20]  // max 20 books
    count_ = 0
    dirty_count_ = 0  // page turns since last flush

    Load():
        open "/spiffs/bookmarks.dat"
        read version header
        parse each line into bookmarks_[]
        close

    Save():
        write version header + all bookmarks to file
        dirty_count_ = 0

    GetBookmark(filename) -> Bookmark*:
        linear search by filename

    UpdatePosition(filename, byte_offset, page_number):
        bm = GetBookmark(filename)
        if bm is null: add new entry
        bm.byte_offset = byte_offset
        bm.page_number = page_number
        dirty_count_ += 1
        if dirty_count_ >= 10: Save()
```

### Progress calculation

For the library screen, reading progress is:

```
progress_pct = current_page * 100 / total_pages
```

The progress bar on the reading screen uses the same value:

```
Bar(pool, total_pages, current_page, 3, ...)
```

---

## 11. Console Commands

The e-reader extends the Gnosis console with book management commands:

```
ereader> help
  ereader list              List loaded books
  ereader open <index>      Open a book by library index
  ereader page <number>     Jump to a specific page
  ereader goto <offset>     Jump to a byte offset
  ereader load <path>       Load a text file from host (via SPIFFS)
  ereader delete <index>    Remove a book
  ereader bookmark          Save bookmark at current position
  ereader info              Show current book info
  ereader fontsize <1|2>    Switch font size
  ereader rebuild-index     Recompute page counts for all books
```

### Loading books

The most practical way to get books onto the device is via SPIFFS at build time or via the `idf.py` tool. However, for convenience, the `ereader load` command could accept text piped over the serial console:

```
ereader load snow_crash.txt 245000
<245000 bytes of raw text follow>
```

The command reads the specified number of bytes from stdin, writes them to `/spiffs/snow_crash.txt`, paginates the file to compute the page count, and updates `books.idx`.

A simpler alternative: include books in the SPIFFS image at build time using `spiffsgen.py` and `idf.py`:

```bash
# In CMakeLists.txt:
spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)
```

With a `spiffs_data/` directory containing book files and the index.

### Console integration with Gnosis

The e-reader commands coexist with the existing Gnosis commands. Both `gnosis` and `ereader` command groups are registered. The prompt changes to `ereader> ` to reflect the application.

---

## 12. Extending the Gnosis Engine

The e-reader requires a few small additions to the Gnosis engine. These are minimal, targeted changes -- not a redesign.

### 12.1 External text pointer

Add to `Node` in `gnosis_types.h`:

```cpp
struct Node {
    // ... existing fields ...
    const char* ext_text = nullptr;  // +8 bytes per node
};
```

Modify `DrawTextBlock` in `widget_renderer.cpp`:

```cpp
void DrawTextBlock(M5GFX& display, Node* node)
{
    const char* text = node->ext_text ? node->ext_text : node->text;
    // ... rest unchanged ...
}
```

### 12.2 Configurable line height for TextBlock

The current `DrawTextBlock` uses `node->props[0]` for line height, which already works. No change needed.

### 12.3 Larger text support

For size 2 rendering in `DrawTextBlock`, we need to multiply the glyph dimensions. The current implementation calls `DrawBitmapText` with size 1 hardcoded. We should pass `node->props[1]` (or a new field) as the text size to `DrawBitmapText`:

```cpp
void DrawTextBlock(M5GFX& display, Node* node)
{
    const char* text = node->ext_text ? node->ext_text : node->text;
    int16_t line_h = node->props[0] > 0 ? node->props[0] : 14;
    int text_size = node->props[1] > 0 ? node->props[1] : 1;
    // ...
    DrawBitmapText(display, line_buf, node->rect.x, y, text_size, kColorFg);
    y += line_h;
}
```

### 12.4 Touch zone detection helper

Add a utility for percentage-based touch zone detection:

```cpp
int TouchZonePercent(const Rect& area, int16_t tx) {
    return (tx - area.x) * 100 / area.w;
}
```

### 12.5 List selection via touch

The current touch handler only checks nav bar icons. For the library screen, we need hit-testing on the LIST node. The existing `Rect::Contains()` method is sufficient; we just need to calculate the row index from the touch Y coordinate.

---

## 13. Data Flow Walkthrough

### Scenario: User opens app, selects a book, reads two pages

**Step 1: Boot**

```
app_main():
    ConsoleInit()           // Start USB REPL
    xTaskCreate(ereader_task)

ereader_task():
    app.InitBoard()         // M5.begin, rotation
    app.MountStorage()      // SPIFFS mount
    app.LoadBookIndex()     // Parse books.idx
    app.LoadBookmarks()     // Parse bookmarks.dat
    app.BuildLibraryScreen()
    app.LayoutScreen()
    app.FullRefresh()       // Show library on EPD
    // enter main loop
```

**Step 2: User taps "Snow Crash" in the library list**

```
HandleTouch():
    tx=450, ty=140 is inside list_node.rect
    row = (140 - list_y) / 56 = 1 -> book index 0
    OpenBook(0)

OpenBook(0):
    current_book_ = 0
    paginator_.Init(chars_per_line=149, lines_per_page=31)
    // Check for saved bookmark
    bm = bookmarks_.GetBookmark("book_001.txt")
    if bm: current_page_ = bm.page_number
    else: current_page_ = 0
    LoadCurrentPage()
    SwitchToReadingScreen()

LoadCurrentPage():
    paginator_.EnsurePage(book_store_, filename, current_page_ + 1)
    paginator_.GetPageText(book_store_, filename, current_page_,
                           page_buffer_, sizeof(page_buffer_))
    FormatPageBuffer()  // Insert word-wrap newlines
    text_node_->ext_text = page_buffer_
    MarkDirty(text_node_)
    UpdateStatusLabels()  // Page number, progress

SwitchToReadingScreen():
    pool_.Reset()
    BuildReadingScreen()
    LayoutScreen(screen_, 960, 540)
    FullRefresh()
```

**Step 3: User taps right side of screen (next page)**

```
HandleTouch():
    tx=800, ty=300 is inside body.rect
    x_pct = 800 * 100 / 960 = 83% -> NEXT zone

NextPage():
    if current_page_ + 1 >= total_pages: return
    current_page_ += 1
    LoadCurrentPage()
    // text_node marked dirty, status labels marked dirty
    // On next loop iteration:

ProcessDirtyRefresh():
    Collect dirty rects:
        - text_node rect: (32, 48, 896, 440)
        - page_label rect: (600, 4, 100, 24)
        - progress_bar rect: (200, 514, 400, 3)
    Merge: text + page_label likely merge (close vertically)
           progress_bar separate (far away)
    Refresh region 1: text area + status bar (~900x470)
    Refresh region 2: progress bar (~400x3)
    Total: 2 partial EPD refreshes using epd_text waveform
```

**Step 4: After 10 page turns, bookmark auto-saves**

```
bookmarks_.UpdatePosition("book_001.txt",
                           page_offsets_[current_page_],
                           current_page_)
// dirty_count reaches 10 -> flush to /spiffs/bookmarks.dat
```

---

## 14. File Structure and Build System

### Project directory

```
0080-papers3-ereader/
├── .envrc                       # source ~/esp/esp-idf-5.3.4/export.sh
├── CMakeLists.txt               # Project-level CMake
├── sdkconfig.defaults           # ESP32-S3, SPIRAM, USB console
├── partitions.csv               # nvs + factory 4M + storage 512K
├── spiffs_data/                 # Book files baked into SPIFFS image
│   ├── books.idx                # Pre-built book index
│   ├── book_001.txt             # Sample book
│   └── bookmarks.dat            # Empty initial bookmarks
├── main/
│   ├── CMakeLists.txt           # Component registration
│   ├── app_main.cpp             # Entry point
│   │
│   │   # ── Gnosis engine (copied from 0078, with ext_text addition) ──
│   ├── gnosis_types.h           # Node, NodePool, Screen (+ ext_text)
│   ├── layout_engine.h/cpp      # Layout algorithm (unchanged)
│   ├── bitmap_font.h/cpp        # 5x7 font (unchanged)
│   ├── widget_renderer.h/cpp    # Widget draw functions (TextBlock uses ext_text)
│   ├── dirty_tracker.h/cpp      # Dirty rect collection (unchanged)
│   ├── node_builder.h           # Builder helpers (unchanged)
│   │
│   │   # ── E-reader application ──
│   ├── ereader_app.h/cpp        # Main app class, screens, touch, loop
│   ├── book_store.h/cpp         # SPIFFS mount, book index, file I/O
│   ├── paginator.h/cpp          # Word-wrap pagination engine
│   ├── bookmark_store.h/cpp     # Reading position persistence
│   └── ereader_console.h/cpp    # esp_console commands for e-reader
```

### Build system

The root `CMakeLists.txt` is identical to 0078 except for the project name:

```cmake
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "../../M5PaperS3-UserDemo/components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(papers3_ereader)
```

The main `CMakeLists.txt` adds SPIFFS:

```cmake
idf_component_register(
    SRCS
        "app_main.cpp"
        "ereader_app.cpp"
        "book_store.cpp"
        "paginator.cpp"
        "bookmark_store.cpp"
        "ereader_console.cpp"
        "layout_engine.cpp"
        "widget_renderer.cpp"
        "dirty_tracker.cpp"
        "bitmap_font.cpp"
    INCLUDE_DIRS "."
    REQUIRES M5Unified console esp_driver_usb_serial_jtag
    PRIV_REQUIRES spiffs
)

# Bake SPIFFS image with sample books
spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)
```

### Copying the Gnosis engine

The engine files from 0078 are copied into the new project's `main/` directory, not shared as a component. This is intentional: it allows the e-reader to modify `gnosis_types.h` (adding `ext_text`) and `widget_renderer.cpp` without affecting the original 0078 firmware. Each firmware in this repo is self-contained.

---

## 15. Implementation Plan

### Phase 1: Skeleton (get something on screen)

1. Create project directory, copy `.envrc`, `CMakeLists.txt`, `partitions.csv`, `sdkconfig.defaults` from 0078
2. Copy Gnosis engine files from 0078/main into new project
3. Add `ext_text` field to `Node`, update `DrawTextBlock` to use it
4. Create `EReaderApp` class with `InitBoard()`, basic main loop
5. Hardcode a single page of text in a `char[2048]` buffer
6. Build a reading screen using `ext_text` pointing at that buffer
7. Verify it compiles, flashes, and shows text on the e-ink display

### Phase 2: Book storage

1. Create `spiffs_data/` with a sample `.txt` book file and `books.idx`
2. Implement `BookStore`: mount SPIFFS, load index, `ReadChunk()`
3. Wire `ReadChunk` to load the first 2 KB of the book into the page buffer
4. Display the first "page" (raw, no word-wrapping yet)

### Phase 3: Pagination

1. Implement `Paginator` with word-wrap algorithm
2. Implement `FormatPageBuffer()` to insert newlines at wrap points
3. Implement page offset table with forward pagination
4. Wire "next page" to: increment page, re-paginate, update buffer, mark dirty
5. Verify page turns with partial EPD refresh

### Phase 4: Touch navigation

1. Implement reading screen touch zones (left 25% = prev, right 25% = next)
2. Update status bar labels (page number, progress percentage) on page turn
3. Update progress bar on page turn
4. Verify dirty tracking correctly refreshes text + status without full refresh

### Phase 5: Library screen

1. Build library screen with LIST widget showing all books from index
2. Implement touch selection on the list
3. Implement `OpenBook()` with paginator initialization
4. Implement screen switching (library <-> reading)

### Phase 6: Bookmarks and persistence

1. Implement `BookmarkStore`: load, save, update
2. Save bookmark every 10 page turns
3. Restore last position on `OpenBook()`
4. Add "BM" nav button for manual bookmark save

### Phase 7: Console commands and polish

1. Register `ereader` console commands (list, open, page, load, info)
2. Add font size switching (size 1 vs size 2)
3. Periodic full EPD refresh every N partial refreshes
4. Test with multiple books, edge cases (empty book, single-page book, very long lines)

---

## 16. API Quick Reference

### BookStore

| Method | Description |
|--------|-------------|
| `Mount()` | Register SPIFFS at `/spiffs` with 512 KB `storage` partition |
| `LoadIndex()` | Parse `books.idx` into `BookInfo` array |
| `BookCount()` | Number of loaded books |
| `GetBook(i)` | Get `BookInfo` by index |
| `ReadChunk(file, offset, len, buf)` | Read bytes from a book file |
| `SaveIndex()` | Write updated index to SPIFFS |

### Paginator

| Method | Description |
|--------|-------------|
| `Init(cpl, lpp)` | Set chars-per-line and lines-per-page |
| `EnsurePage(store, file, n)` | Compute page offsets up to page `n` |
| `GetPageText(store, file, n, buf, sz)` | Load page `n` text into buffer |
| `FormatPage(raw, cpl, out, sz)` | Insert word-wrap newlines |
| `PageCount()` | Total pages (from cache or computed) |

### BookmarkStore

| Method | Description |
|--------|-------------|
| `Load()` | Read `/spiffs/bookmarks.dat` |
| `Save()` | Write all bookmarks to file |
| `GetBookmark(file)` | Find bookmark for a book file |
| `UpdatePosition(file, offset, page)` | Set/update bookmark, auto-flush every 10 |

### EReaderApp

| Method | Description |
|--------|-------------|
| `Run()` | Main entry: init, mount, build library, loop |
| `OpenBook(index)` | Load book, restore bookmark, switch to reading screen |
| `NextPage()` | Advance one page, update display |
| `PreviousPage()` | Go back one page |
| `SwitchToLibrary()` | Rebuild library screen, full refresh |
| `SwitchToReading()` | Rebuild reading screen, full refresh |
| `SaveBookmark()` | Persist current position |

### Touch Zones (Reading Screen)

| Zone | X range | Action |
|------|---------|--------|
| Left 25% | 0 -- 240 | Previous page |
| Center 50% | 240 -- 720 | No action |
| Right 25% | 720 -- 960 | Next page |
| Nav "LIBRARY" | hit test | Switch to library |
| Nav "BM" | hit test | Save bookmark |

### Display Geometry

| Parameter | Size 1 | Size 2 |
|-----------|--------|--------|
| Glyph width | 6 px | 12 px |
| Line height | 14 px | 20 px |
| Text area | 896 x 440 px | 896 x 440 px |
| Chars per line | 149 | 74 |
| Lines per page | 31 | 22 |
| Chars per page | ~4600 | ~1628 |
| Words per page | ~800 | ~280 |

### Key Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `kScreenW` | 960 | Display width (landscape) |
| `kScreenH` | 540 | Display height (landscape) |
| `kBarHeight` | 32 | Status bar height |
| `kNavHeight` | 32 | Navigation bar height |
| `kTextPadding` | 32 | Left/right padding in reading pane |
| `kPageBufferSize` | 2048 | Page text buffer (SRAM) |
| `kReadAheadSize` | 8192 | File read-ahead buffer (PSRAM) |
| `kMaxBooks` | 20 | Maximum books in library |
| `kMaxPages` | 2048 | Maximum pages in offset table |
| `kBookmarkFlushInterval` | 10 | Page turns between bookmark saves |

---

## Glossary

| Term | Definition |
|------|------------|
| **Page buffer** | A `char[2048]` holding the formatted text for the currently displayed page. Lives in app memory, pointed to by `node->ext_text`. |
| **Page offset table** | Array of byte offsets into the book file, one per page. `page_offsets[N]` is where page N starts. |
| **Word wrap** | Breaking text at word boundaries (spaces) to fit within a fixed column width. Never splits a word. |
| **Pagination** | Computing where page breaks fall in a continuous text, given a fixed viewport size. |
| **SPIFFS** | SPI Flash File System -- a flat filesystem for NOR flash, used on ESP32 for persistent storage. No directories. |
| **ext_text** | A `const char*` field added to `Node` that points to an external text buffer, allowing text larger than the 64-byte inline `node->text`. |
| **Dirty rect** | A screen rectangle that needs redrawing because its content changed. |
| **EPD partial refresh** | Updating only a sub-region of the e-ink display, much faster than a full refresh. |
| **Read-ahead buffer** | A larger buffer (8 KB) loaded from the book file, from which multiple pages can be paginated without re-reading the file. |
