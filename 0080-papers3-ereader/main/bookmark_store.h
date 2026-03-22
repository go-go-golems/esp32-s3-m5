#pragma once

#include <cstdint>

namespace ereader {

static constexpr int kMaxBookmarks = 20;
static constexpr int kBookmarkFlushInterval = 10;

struct Bookmark {
    char filename[32]{};
    int32_t byte_offset = 0;
    int32_t page_number = 0;
};

class BookmarkStore {
public:
    bool Load();
    bool Save();

    const Bookmark* GetBookmark(const char* filename) const;
    void UpdatePosition(const char* filename, int32_t byte_offset, int32_t page_number);

    int Count() const { return count_; }

private:
    Bookmark bookmarks_[kMaxBookmarks]{};
    int count_ = 0;
    int dirty_count_ = 0;
};

}  // namespace ereader
