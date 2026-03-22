---
Title: Diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/gnosis_types.h:Added ext_text pointer to Node struct"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/widget_renderer.cpp:Updated DrawTextBlock to use ext_text, configurable text size, 256-char line buffer"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/ereader_app.cpp:Main application: reading/library screens, touch zones, page buffer"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/ereader_console.cpp:Console REPL commands"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/book_store.cpp:SPIFFS mount, books.idx parsing, ReadChunk"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/main/paginator.cpp:Word-wrap pagination, page offset table, FormatText"
    - "/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0080-papers3-ereader/spiffs_data/sample.txt:Sample book text baked into SPIFFS"
ExternalSources: []
Summary: "Step-by-step implementation diary for the PaperS3 interactive e-reader firmware."
LastUpdated: 2026-03-22T11:16:21.114949361-04:00
WhatFor: "Track implementation progress, failures, and decisions for review and continuation."
WhenToUse: "When continuing implementation or reviewing what was done."
---

# Diary

## Goal

Build an interactive e-reader on the M5Paper S3 (960x540 EPD) using the Gnosis layout engine from firmware 0078. Track every implementation step, including failures and decisions, so the work can be continued or reviewed.

## Step 1: Project Skeleton and ext_text Extension

The first step covered three tasks at once: creating the project from 0078's engine files, extending the Node struct with an external text pointer, and building a working reading screen with hardcoded sample text.

The critical design decision here was how to handle large text in a system where nodes have a 64-byte inline text buffer. Adding a `const char* ext_text` field to Node (8 bytes) is the minimal change -- the renderer checks it first and falls back to `node->text` if null. This means every existing node type works unchanged, and only the TEXT_BLOCK used for book pages needs the external pointer.

### Prompt Context

**User prompt (verbatim):** "ok, create a detailed list of tasks in the ticket, were going to hand this off to our new intern. Then work on them one by one, committing as you go and keeping a diary (see skills)"

**Assistant interpretation:** Create docmgr tasks for the e-reader implementation, then execute them sequentially with commits and diary entries.

**Inferred user intent:** Produce a working e-reader firmware incrementally, with auditable progress.

**Commit (code):** 2f0a3eb — "feat(papers3): scaffold e-reader firmware with Gnosis engine and ext_text"

### What I did

- Created `0080-papers3-ereader/` with `.envrc` (sourcing esp-idf-5.3.4), `CMakeLists.txt`, `sdkconfig.defaults`, `partitions.csv`
- Copied 10 Gnosis engine files from 0078/main (types, layout, font, renderer, dirty tracker, builder)
- Added `const char* ext_text = nullptr` to `Node` struct in `gnosis_types.h`
- Modified `DrawTextBlock` in `widget_renderer.cpp`: reads from `ext_text` when non-null, supports `props[1]` as text size multiplier, increased line buffer from 64 to 256 chars
- Created `EReaderApp` class with `BuildReadingScreen()` (status bar + full-width text block + nav bar), `BuildLibraryScreen()` (stub), `HandleTouch()` with 25/50/25 zone split, `LoadCurrentPage()` pointing `ext_text` at a 4 KB page buffer
- Created `ereader_console.cpp` with `ereader list/open/page/info/refresh` commands
- Created `app_main.cpp` launching console on main task and UI on core 1
- Hardcoded `kSampleText` (~900 bytes of Snow Crash excerpt) as the initial page content
- Build produced 657 KB binary

### Why

Starting with a hardcoded page of text proves the entire rendering pipeline end-to-end: the ext_text pointer, the updated DrawTextBlock, the reading screen layout, the touch zone detection, and the dirty-rect refresh path. Once this displays correctly on the EPD, every subsequent step (SPIFFS, pagination, bookmarks) is a data-source change, not a rendering change.

### What worked

- `ext_text` addition was clean -- one field, one `if` check in the renderer
- Increasing the line buffer from 64 to 256 chars in DrawTextBlock accommodates the wider reading pane (149 chars/line at size 1)
- The builder helper pattern (`TextBlock(pool, "", 14, 32, 8, 896, 440)` + `node->ext_text = buffer`) composes naturally
- Build succeeded with IDF 5.3.4 toolchain (after forcing correct PATH)

### What didn't work

- First attempted to build with IDF 5.4.1 by mistake. User corrected: always use 5.3.4.
- IDF 5.3.4's `export.sh` picked up the 5.4.1 toolchain (xtensa-esp-elf 14.2) because both are installed. Had to prepend the 13.2 toolchain to PATH manually before sourcing export.sh. The build succeeded but resolved some components from 5.4.1 because `IDF_PATH` was overridden by the shell state. This should be cleaned up.

### What I learned

- The `.envrc` in each project specifies the IDF version. Always respect it.
- `IDF_PYTHON_ENV_PATH` must be unset when switching between IDF versions or the python env from the other version gets used.
- The Gnosis Node struct at ~140 bytes per node means the ext_text pointer (8 bytes) is a negligible overhead.

### What was tricky to build

The IDF toolchain version conflict. IDF 5.3.4 requires xtensa-esp-elf 13.2, but 5.4.1 installs 14.2 and its export.sh puts 14.2 first in PATH. The cmake `check_expected_tool_version` macro catches this and fails with a clear error. The workaround is to prepend the correct toolchain path before sourcing export.sh: `export PATH="~/.espressif/tools/xtensa-esp-elf/esp-13.2.0_20240530/xtensa-esp-elf/bin:$PATH"`.

### What warrants a second pair of eyes

- The IDF version situation: the build used 5.4.1 components despite intending 5.3.4. For a one-off demo this is fine, but if ABI compatibility matters it should be verified.
- The `ext_text` pointer is a raw `const char*` with no ownership semantics. The caller must ensure the buffer outlives the node. This is fine for our use (page buffer in the app class) but could be a footgun if someone stores a temporary.

### What should be done in the future

- Clean up the IDF environment setup so 5.3.4 export.sh reliably selects the 13.2 toolchain
- Consider running `install.sh` for 5.3.4 specifically to register its tool preferences

### Code review instructions

- Start at `gnosis_types.h:123` to see the `ext_text` addition
- Then `widget_renderer.cpp:186-210` for the updated DrawTextBlock
- Then `ereader_app.cpp:BuildReadingScreen()` to see how the reading screen is assembled
- Then `ereader_app.cpp:HandleTouch()` for the 25/50/25 touch zone logic
- To validate: `idf.py build` in the 0080 directory with IDF 5.3.4 sourced

### Technical details

**Node ext_text field:**
```cpp
// gnosis_types.h, line 123
const char* ext_text = nullptr;
```

**DrawTextBlock change:**
```cpp
// widget_renderer.cpp, line 188
const char* text = node->ext_text ? node->ext_text : node->text;
int text_size = node->props[1] > 0 ? node->props[1] : 1;
```

**Reading screen text area:**
- Position: (32, 8) within body (which starts at y=32)
- Size: 896 x 440 px
- At size 1: 149 chars/line, 31 lines/page

**Touch zones:**
```cpp
int x_pct = (tx - body.rect.x) * 100 / body.rect.w;
if (x_pct < 25) PreviousPage();
else if (x_pct >= 75) NextPage();
```

## Step 2: BookStore, Paginator, SPIFFS, and Library/Reading Wiring

This step replaced the hardcoded sample text with a real pipeline: SPIFFS-backed book storage, a word-wrap paginator with a page offset table, and a sample book baked into the SPIFFS image at build time. The library screen now lists books from the index, touch selection opens a book, and page turns read real pages from the filesystem.

The paginator is the most algorithmically interesting piece. It uses a two-pass word-wrap approach: first, `PaginateOnePage` scans forward through raw text to find where a page ends (respecting word boundaries and paragraph breaks). Then `FormatText` re-wraps that raw text into a buffer with `\n` characters at the wrap points, ready for the `DrawTextBlock` renderer. The page offset table (`page_offsets_[]`) is built incrementally as the user reads forward.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Continue implementing tasks from the plan -- BookStore, Paginator, SPIFFS data, library-to-reader wiring.

**Inferred user intent:** Get a functional e-reader that loads text from flash and paginates it.

**Commit (code):** eaf00a3 — "feat(ereader): add SPIFFS book storage, word-wrap paginator, and sample book"

### What I did

- Created `book_store.h/cpp`: SPIFFS mount at `/spiffs` with `storage` partition, `books.idx` parsing (version header + pipe-delimited fields), `ReadChunk()` for random-access file reads, `FileSize()`, `SaveIndex()`
- Created `paginator.h/cpp`: `PaginateOnePage()` word-wrap algorithm, `FormatText()` newline insertion, `GetPageText()` combining both, `EnsurePage()` for incremental page offset computation
- Created `spiffs_data/` with `books.idx`, `bookmarks.dat`, and `sample.txt` (~2.8 KB of text)
- Added `spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)` to CMakeLists.txt
- Rewrote `ereader_app.cpp`: `MountStorage()` on boot, `OpenBook()` initializes paginator and computes total pages, `LoadCurrentPage()` reads from SPIFFS via paginator, `BuildLibraryScreen()` populates LIST from index, library touch selection computes row index from touch Y
- `ComputeTotalPages()` paginates entire file on first open, caches result in `books.idx`

### Why

The hardcoded text proved the rendering pipeline. Now the data path is real: file -> paginator -> page buffer -> ext_text -> DrawTextBlock -> EPD. Every subsequent feature (bookmarks, font size, additional books) is a variation on this pipeline.

### What worked

- SPIFFS `spiffs_create_partition_image` bakes the book into the firmware image -- no manual upload needed
- The paginator's incremental page offset table means opening a book is instant (first page computed immediately) while total page count is computed once and cached
- `ReadChunk` with seek+read is efficient for random page access
- The `FormatText` / `PaginateOnePage` separation keeps concerns clean

### What didn't work

- `snprintf` with `"%d%%"` into a `char[8]` buffer triggered `-Werror=format-truncation` because the compiler considers `%d` might produce 11 characters. Fixed by increasing buffer to 16.
- Building with IDF 5.3.4 requires a clean subshell with `unset IDF_PYTHON_ENV_PATH` because the current terminal has 5.4.1's python env active. Used `bash -l -c 'unset IDF_PYTHON_ENV_PATH; export IDF_PATH=...; source ...; idf.py build'`.

### What I learned

- SPIFFS `format_if_mount_failed=true` is essential -- on first flash there's no filesystem yet
- The paginator must handle single `\n` (treat as space) differently from double `\n\n` (paragraph break) for prose text
- Pre-computing total pages by paginating the entire file is acceptable for ~3 KB files but would be slow for larger books; caching in the index is critical

### What was tricky to build

The word-wrap algorithm's edge cases: words longer than a line (force-break), paragraph breaks consuming two lines (blank line between paragraphs), trailing spaces, and EOF detection during incremental pagination. The key invariant is that `PaginateOnePage` must always make forward progress (return > 0) or the `EnsurePage` loop will hang. The `if (page_len <= 0) break` guard handles this.

### What warrants a second pair of eyes

- The paginator's word-wrap algorithm has not been tested with edge cases: very long words, files with only newlines, empty files, Unicode (which the 5x7 font doesn't support anyway)
- `ComputeTotalPages` loops up to `kMaxPageOffsets` (2048) times, each reading 8 KB from SPIFFS. For a 500 KB book that's ~60 iterations, which should be fast enough, but worth verifying

### What should be done in the future

- Task 9: BookmarkStore for persistent reading positions
- Task 10: Console commands for book management
- Task 11: Font size switching, periodic full refresh, edge case testing

### Code review instructions

- Start at `paginator.cpp:PaginateOnePage()` (line 22) -- this is the core algorithm
- Then `paginator.cpp:FormatText()` (line 99) for the output formatting
- Then `book_store.cpp:LoadIndex()` for the index parser
- Then `ereader_app.cpp:OpenBook()` and `LoadCurrentPage()` for the data flow
- To validate: build with `bash -l -c 'unset IDF_PYTHON_ENV_PATH; export IDF_PATH=~/esp/esp-idf-5.3.4; source $IDF_PATH/export.sh; cd 0080-papers3-ereader; idf.py build'`

### Technical details

**Paginator data flow:**
```
BookStore.ReadChunk(file, offset, 8KB) -> raw_buffer
    -> PaginateOnePage(raw_buffer) -> page_end_offset
    -> FormatText(raw_buffer[0..page_end]) -> formatted with \n
    -> page_buffer_ (4KB) -> text_node_->ext_text
    -> DrawTextBlock renders line by line
```

**Page offset table:**
```
page_offsets_[0] = 0          (start of file)
page_offsets_[1] = 847        (end of page 1 in bytes)
page_offsets_[2] = 1692       (end of page 2)
...
```

**Library touch row calculation:**
```cpp
int row = (ty - list_node->rect.y) / row_h;
int book_idx = row / 2;  // 2 list rows per book entry
```
