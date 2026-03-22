# Tasks

## TODO

- [x] Add tasks here

- [x] Phase 1: Create project skeleton - copy CMakeLists.txt, sdkconfig.defaults, partitions.csv from 0078, copy Gnosis engine source files, verify clean build
- [x] Phase 1: Add ext_text pointer to Node struct in gnosis_types.h, update DrawTextBlock in widget_renderer.cpp to use ext_text when present
- [x] Phase 1: Create EReaderApp skeleton with InitBoard, hardcoded page buffer, reading screen using ext_text, verify text renders on EPD
- [x] Phase 2: Implement BookStore - SPIFFS mount, books.idx parsing, ReadChunk file I/O, create spiffs_data/ with sample book
- [x] Phase 3: Implement Paginator - word-wrap algorithm, page offset table, FormatPageBuffer with newline insertion
- [x] Phase 3: Wire pagination into reading screen - NextPage/PreviousPage update page buffer and mark dirty, verify partial EPD refresh
- [x] Phase 4: Implement touch navigation - reading screen touch zones (left 25% prev, right 25% next), status bar updates (page number, progress bar)
- [ ] Phase 5: Build library screen - LIST widget with book entries, touch selection to open book, screen switching between library and reading
- [ ] Phase 6: Implement BookmarkStore - load/save bookmarks.dat, auto-save every 10 page turns, restore position on OpenBook
- [ ] Phase 7: Register esp_console commands - ereader list/open/page/info/fontsize/rebuild-index, coexist with gnosis commands
- [ ] Phase 7: Polish - font size switching (size 1 vs 2), periodic full EPD refresh, edge case testing (empty book, single page, long lines)
