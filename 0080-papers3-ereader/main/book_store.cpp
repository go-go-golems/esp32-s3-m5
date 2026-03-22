#include "book_store.h"

#include <cstdio>
#include <cstring>

#include "esp_spiffs.h"
#include "esp_log.h"

namespace ereader {

static const char* TAG = "book_store";

bool BookStore::Mount()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 4,
        .format_if_mount_failed = true,
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        std::snprintf(status_, sizeof(status_), "mount failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", status_);
        return false;
    }

    size_t total = 0, used = 0;
    esp_spiffs_info("storage", &total, &used);
    std::snprintf(status_, sizeof(status_), "mounted: %zu/%zu bytes used", used, total);
    ESP_LOGI(TAG, "%s", status_);
    mounted_ = true;
    return true;
}

bool BookStore::LoadIndex()
{
    if (!mounted_) return false;

    FILE* f = std::fopen("/spiffs/books.idx", "r");
    if (!f) {
        std::snprintf(status_, sizeof(status_), "no books.idx found");
        ESP_LOGW(TAG, "%s", status_);
        count_ = 0;
        return false;
    }

    char line[256];
    // Read version header
    if (!std::fgets(line, sizeof(line), f) || std::strncmp(line, "ereader-index-v1", 16) != 0) {
        std::snprintf(status_, sizeof(status_), "invalid index version");
        std::fclose(f);
        return false;
    }

    count_ = 0;
    while (count_ < kMaxBooks && std::fgets(line, sizeof(line), f)) {
        // Strip newline
        char* nl = std::strchr(line, '\n');
        if (nl) *nl = '\0';
        nl = std::strchr(line, '\r');
        if (nl) *nl = '\0';

        if (line[0] == '\0') continue;

        // Parse: filename|title|author|pages
        char* tok = std::strtok(line, "|");
        if (!tok) continue;
        std::strncpy(books_[count_].filename, tok, kMaxFilename - 1);

        tok = std::strtok(nullptr, "|");
        if (tok) std::strncpy(books_[count_].title, tok, kMaxTitle - 1);

        tok = std::strtok(nullptr, "|");
        if (tok) std::strncpy(books_[count_].author, tok, kMaxAuthor - 1);

        tok = std::strtok(nullptr, "|");
        if (tok) books_[count_].total_pages = std::atoi(tok);

        // Get file size
        books_[count_].file_size = FileSize(books_[count_].filename);

        ++count_;
    }

    std::fclose(f);
    std::snprintf(status_, sizeof(status_), "loaded %d books", count_);
    ESP_LOGI(TAG, "%s", status_);
    return true;
}

bool BookStore::SaveIndex()
{
    if (!mounted_) return false;

    FILE* f = std::fopen("/spiffs/books.idx", "w");
    if (!f) {
        std::snprintf(status_, sizeof(status_), "failed to write index");
        return false;
    }

    std::fprintf(f, "ereader-index-v1\n");
    for (int i = 0; i < count_; ++i) {
        std::fprintf(f, "%s|%s|%s|%d\n",
                     books_[i].filename, books_[i].title,
                     books_[i].author, static_cast<int>(books_[i].total_pages));
    }
    std::fclose(f);
    return true;
}

int BookStore::ReadChunk(const char* filename, int32_t offset, int32_t length, char* buf)
{
    if (!mounted_ || !filename || !buf || length <= 0) return 0;

    char path[64];
    std::snprintf(path, sizeof(path), "/spiffs/%s", filename);

    FILE* f = std::fopen(path, "r");
    if (!f) return 0;

    if (offset > 0) {
        std::fseek(f, offset, SEEK_SET);
    }

    int read = static_cast<int>(std::fread(buf, 1, static_cast<size_t>(length), f));
    std::fclose(f);
    return read;
}

int32_t BookStore::FileSize(const char* filename)
{
    if (!mounted_ || !filename) return 0;

    char path[64];
    std::snprintf(path, sizeof(path), "/spiffs/%s", filename);

    FILE* f = std::fopen(path, "r");
    if (!f) return 0;

    std::fseek(f, 0, SEEK_END);
    int32_t size = static_cast<int32_t>(std::ftell(f));
    std::fclose(f);
    return size;
}

}  // namespace ereader
