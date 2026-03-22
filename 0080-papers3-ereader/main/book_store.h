#pragma once

#include <cstddef>
#include <cstdint>

namespace ereader {

static constexpr int kMaxBooks = 20;
static constexpr int kMaxFilename = 32;
static constexpr int kMaxTitle = 48;
static constexpr int kMaxAuthor = 32;

struct BookInfo {
    char filename[kMaxFilename]{};
    char title[kMaxTitle]{};
    char author[kMaxAuthor]{};
    int32_t file_size = 0;
    int32_t total_pages = 0;
};

class BookStore {
public:
    bool Mount();
    bool LoadIndex();
    bool SaveIndex();

    int BookCount() const { return count_; }
    const BookInfo& GetBook(int index) const { return books_[index]; }
    BookInfo& GetBookMut(int index) { return books_[index]; }

    // Read a chunk of a book file into buf. Returns bytes actually read.
    int ReadChunk(const char* filename, int32_t offset, int32_t length, char* buf);

    // Get total file size for a book file.
    int32_t FileSize(const char* filename);

    const char* LastStatus() const { return status_; }

private:
    BookInfo books_[kMaxBooks]{};
    int count_ = 0;
    bool mounted_ = false;
    char status_[64]{};
};

}  // namespace ereader
