#include "bookmark_store.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_log.h"

namespace ereader {

static const char* TAG = "bookmark_store";

bool BookmarkStore::Load()
{
    FILE* f = std::fopen("/spiffs/bookmarks.dat", "r");
    if (!f) {
        ESP_LOGW(TAG, "no bookmarks file");
        return false;
    }

    char line[128];
    // Version header
    if (!std::fgets(line, sizeof(line), f) ||
        std::strncmp(line, "ereader-bookmarks-v1", 20) != 0) {
        std::fclose(f);
        return false;
    }

    count_ = 0;
    while (count_ < kMaxBookmarks && std::fgets(line, sizeof(line), f)) {
        char* nl = std::strchr(line, '\n');
        if (nl) *nl = '\0';
        if (line[0] == '\0') continue;

        // Parse: filename|byte_offset|page_number
        char* tok = std::strtok(line, "|");
        if (!tok) continue;
        std::strncpy(bookmarks_[count_].filename, tok, sizeof(bookmarks_[0].filename) - 1);

        tok = std::strtok(nullptr, "|");
        if (tok) bookmarks_[count_].byte_offset = std::atol(tok);

        tok = std::strtok(nullptr, "|");
        if (tok) bookmarks_[count_].page_number = std::atol(tok);

        ++count_;
    }

    std::fclose(f);
    ESP_LOGI(TAG, "loaded %d bookmarks", count_);
    dirty_count_ = 0;
    return true;
}

bool BookmarkStore::Save()
{
    FILE* f = std::fopen("/spiffs/bookmarks.dat", "w");
    if (!f) {
        ESP_LOGE(TAG, "failed to write bookmarks");
        return false;
    }

    std::fprintf(f, "ereader-bookmarks-v1\n");
    for (int i = 0; i < count_; ++i) {
        std::fprintf(f, "%s|%ld|%ld\n",
                     bookmarks_[i].filename,
                     static_cast<long>(bookmarks_[i].byte_offset),
                     static_cast<long>(bookmarks_[i].page_number));
    }
    std::fclose(f);
    dirty_count_ = 0;
    ESP_LOGI(TAG, "saved %d bookmarks", count_);
    return true;
}

const Bookmark* BookmarkStore::GetBookmark(const char* filename) const
{
    for (int i = 0; i < count_; ++i) {
        if (std::strcmp(bookmarks_[i].filename, filename) == 0) {
            return &bookmarks_[i];
        }
    }
    return nullptr;
}

void BookmarkStore::UpdatePosition(const char* filename, int32_t byte_offset, int32_t page_number)
{
    // Find existing or create new
    Bookmark* bm = nullptr;
    for (int i = 0; i < count_; ++i) {
        if (std::strcmp(bookmarks_[i].filename, filename) == 0) {
            bm = &bookmarks_[i];
            break;
        }
    }
    if (!bm && count_ < kMaxBookmarks) {
        bm = &bookmarks_[count_++];
        std::strncpy(bm->filename, filename, sizeof(bm->filename) - 1);
    }
    if (!bm) return;

    bm->byte_offset = byte_offset;
    bm->page_number = page_number;
    ++dirty_count_;

    if (dirty_count_ >= kBookmarkFlushInterval) {
        Save();
    }
}

}  // namespace ereader
