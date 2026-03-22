#pragma once

#include "book_store.h"
#include <cstdint>

namespace ereader {

static constexpr int kMaxPageOffsets = 2048;
static constexpr int kReadAheadSize = 8192;

class Paginator {
public:
    void Init(int chars_per_line, int lines_per_page);
    void Reset();

    // Ensure page offsets are computed up to (and including) page_number.
    void EnsurePage(BookStore& store, const char* filename, int page_number);

    // Load page text into buf, with word-wrap newlines inserted.
    // Returns number of bytes written.
    int GetPageText(BookStore& store, const char* filename, int page_number,
                    char* buf, int buf_size);

    int PagesComputed() const { return pages_computed_; }
    int CharsPerLine() const { return chars_per_line_; }
    int LinesPerPage() const { return lines_per_page_; }

private:
    // Paginate text starting at offset 0 in `text` (of `text_len` bytes).
    // Returns the byte offset within `text` where the page ends.
    int PaginateOnePage(const char* text, int text_len);

    // Word-wrap `raw` text into `out`, inserting \n at wrap points.
    int FormatText(const char* raw, int raw_len, char* out, int out_size);

    int32_t page_offsets_[kMaxPageOffsets]{};
    int pages_computed_ = 0;
    int chars_per_line_ = 80;
    int lines_per_page_ = 30;
};

}  // namespace ereader
