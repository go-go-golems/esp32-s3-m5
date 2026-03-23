#include "paginator.h"

#include <algorithm>
#include <cstring>

namespace ereader {

void Paginator::Init(int chars_per_line, int lines_per_page)
{
    chars_per_line_ = chars_per_line;
    lines_per_page_ = lines_per_page;
    Reset();
}

void Paginator::Reset()
{
    page_offsets_[0] = 0;
    pages_computed_ = 1;
}

int Paginator::PaginateOnePage(const char* text, int text_len)
{
    int line_count = 0;
    int col = 0;
    int i = 0;

    while (i < text_len && line_count < lines_per_page_) {
        // Paragraph break: double newline
        if (text[i] == '\n') {
            if (i + 1 < text_len && text[i + 1] == '\n') {
                // Paragraph break
                line_count++;
                col = 0;
                i += 2;
                if (line_count < lines_per_page_) {
                    line_count++;
                }
                continue;
            } else {
                // Single newline: treat as space
                i++;
                if (col > 0 && col < chars_per_line_) {
                    col++;  // space
                }
                continue;
            }
        }

        // Skip leading spaces at start of line
        if (text[i] == ' ' && col == 0) {
            i++;
            continue;
        }

        // Find next word
        int word_start = i;
        while (i < text_len && text[i] != ' ' && text[i] != '\n') {
            i++;
        }
        int word_len = i - word_start;

        if (word_len == 0) {
            i++;
            continue;
        }

        // Does the word fit on the current line?
        if (col > 0 && col + 1 + word_len > chars_per_line_) {
            // Wrap to next line
            line_count++;
            col = 0;
            if (line_count >= lines_per_page_) {
                // Page full, rewind to word start
                i = word_start;
                break;
            }
        }

        if (col == 0 && word_len > chars_per_line_) {
            // Word longer than a line: force-break
            int fits = chars_per_line_;
            i = word_start + fits;
            line_count++;
            col = 0;
        } else {
            // Word fits
            if (col > 0) col++;  // space before word
            col += word_len;

            // Skip trailing space
            if (i < text_len && text[i] == ' ') {
                i++;
            }
        }
    }

    return i;
}

void Paginator::EnsurePage(BookStore& store, const char* filename, int page_number)
{
    if (page_number < pages_computed_) return;

    static char read_buf[kReadAheadSize];  // static to avoid 8KB stack alloc

    while (pages_computed_ <= page_number && pages_computed_ < kMaxPageOffsets) {
        int32_t offset = page_offsets_[pages_computed_ - 1];
        int bytes_read = store.ReadChunk(filename, offset, kReadAheadSize - 1, read_buf);
        if (bytes_read <= 0) break;  // EOF
        read_buf[bytes_read] = '\0';

        int page_len = PaginateOnePage(read_buf, bytes_read);
        if (page_len <= 0) break;  // Can't make progress

        page_offsets_[pages_computed_] = offset + page_len;
        pages_computed_++;

        // If we read less than the buffer, we've hit EOF
        if (bytes_read < kReadAheadSize - 1 && page_len >= bytes_read) {
            break;
        }
    }
}

int Paginator::FormatText(const char* raw, int raw_len, char* out, int out_size)
{
    int oi = 0;  // output index
    int col = 0;
    int i = 0;

    while (i < raw_len && oi < out_size - 1) {
        // Paragraph break
        if (raw[i] == '\n') {
            if (i + 1 < raw_len && raw[i + 1] == '\n') {
                if (oi < out_size - 2) {
                    out[oi++] = '\n';
                    out[oi++] = '\n';
                }
                col = 0;
                i += 2;
                continue;
            } else {
                // Single newline -> space
                i++;
                if (col > 0 && col < chars_per_line_) {
                    col++;
                }
                continue;
            }
        }

        // Skip leading spaces
        if (raw[i] == ' ' && col == 0) {
            i++;
            continue;
        }

        // Find word
        int word_start = i;
        while (i < raw_len && raw[i] != ' ' && raw[i] != '\n') {
            i++;
        }
        int word_len = i - word_start;
        if (word_len == 0) { i++; continue; }

        // Wrap?
        if (col > 0 && col + 1 + word_len > chars_per_line_) {
            if (oi < out_size - 1) out[oi++] = '\n';
            col = 0;
        }

        // Space before word (if not at start of line)
        if (col > 0 && oi < out_size - 1) {
            out[oi++] = ' ';
            col++;
        }

        // Copy word
        int copy_len = std::min(word_len, out_size - 1 - oi);
        std::memcpy(out + oi, raw + word_start, copy_len);
        oi += copy_len;
        col += copy_len;

        // Skip trailing space
        if (i < raw_len && raw[i] == ' ') {
            i++;
        }
    }

    out[oi] = '\0';
    return oi;
}

int Paginator::GetPageText(BookStore& store, const char* filename, int page_number,
                           char* buf, int buf_size)
{
    EnsurePage(store, filename, page_number + 1);

    if (page_number + 1 >= pages_computed_) {
        // Might be last page
        EnsurePage(store, filename, page_number + 1);
    }

    int32_t start = page_offsets_[page_number];
    int32_t end;
    if (page_number + 1 < pages_computed_) {
        end = page_offsets_[page_number + 1];
    } else {
        // Last page: read whatever remains
        end = start + buf_size;
    }

    int32_t length = std::min(end - start, static_cast<int32_t>(buf_size - 1));
    static char raw[kReadAheadSize];  // static to avoid 8KB stack alloc
    int bytes_read = store.ReadChunk(filename, start, length, raw);
    if (bytes_read <= 0) {
        buf[0] = '\0';
        return 0;
    }
    raw[bytes_read] = '\0';

    return FormatText(raw, bytes_read, buf, buf_size);
}

}  // namespace ereader
