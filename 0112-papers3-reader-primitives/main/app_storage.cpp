#include "app_storage.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

#include "app_display.h"
#include "app_reader_book.h"

namespace reader {
namespace {

const char *kTag = "storage";

// PaperS3 microSD over SPI (pins per M5PaperS3-UserDemo hal.cpp).
constexpr gpio_num_t kPinMiso = GPIO_NUM_40;
constexpr gpio_num_t kPinMosi = GPIO_NUM_38;
constexpr gpio_num_t kPinSclk = GPIO_NUM_39;
constexpr gpio_num_t kPinCs = GPIO_NUM_47;
constexpr char kMountPoint[] = "/sdcard";
constexpr char kStateDir[] = "/sdcard/.s3paper";
constexpr char kPositionsPath[] = "/sdcard/.s3paper/positions.bin";
constexpr char kPositionsTmp[] = "/sdcard/.s3paper/positions.tmp";
constexpr char kPositionsBak[] = "/sdcard/.s3paper/positions.bak";

sdmmc_card_t *s_card = nullptr;
bool s_spi_bus_initialized = false;

BookEntry s_books[kMaxBooks];
uint32_t s_book_count = 0;

// Versioned, checksummed position file (design doc: never bare offsets).
constexpr uint32_t kPositionsMagic = 0x53335250;  // "S3RP"
constexpr uint32_t kPositionsVersion = 1;
constexpr uint32_t kMaxPositions = 32;

struct PositionRecord {
    s3paper::ContentHash content;
    uint64_t byte_offset;
    uint32_t context_hash;
};

struct PositionsFile {
    uint32_t magic;
    uint32_t version;
    uint32_t count;
    PositionRecord records[kMaxPositions];
    uint32_t crc;  // FNV over all preceding bytes
};

PositionsFile s_positions{};
uint32_t s_position_writes = 0;
uint32_t s_position_write_failures = 0;

// Bookmarks: same persistence pattern, separate file and generation.
constexpr uint32_t kBookmarksMagic = 0x53334D42;  // "S3MB"
constexpr uint32_t kBookmarksVersion = 1;
constexpr uint32_t kMaxBookmarks = 64;
constexpr char kBookmarksPath[] = "/sdcard/.s3paper/bookmarks.bin";
constexpr char kBookmarksTmp[] = "/sdcard/.s3paper/bookmarks.tmp";
constexpr char kBookmarksBak[] = "/sdcard/.s3paper/bookmarks.bak";

struct BookmarkRecord {
    s3paper::ContentHash content;
    uint64_t byte_offset;
    uint32_t context_hash;
};

struct BookmarksFile {
    uint32_t magic;
    uint32_t version;
    uint32_t count;
    BookmarkRecord records[kMaxBookmarks];
    uint32_t crc;
};

BookmarksFile s_bookmarks{};

// Coalesced-write state: dirty flags plus the age of the oldest unflushed
// change (design task sda9: don't hit the card on every page turn).
bool s_positions_dirty = false;
bool s_bookmarks_dirty = false;
int64_t s_dirty_since_us = 0;
constexpr int64_t kFlushAfterUs = 2'000'000;

uint32_t PositionsCrc(const PositionsFile &f) {
    return s3paper::Fnv1a(reinterpret_cast<const uint8_t *>(&f),
                          offsetof(PositionsFile, crc));
}

uint32_t BookmarksCrc(const BookmarksFile &f) {
    return s3paper::Fnv1a(reinterpret_cast<const uint8_t *>(&f),
                          offsetof(BookmarksFile, crc));
}

// Shared atomic write: tmp -> flush -> fsync -> bak swap -> rename.
bool AtomicWrite(const char *tmp, const char *bak, const char *path,
                 const void *data, size_t size) {
    mkdir(kStateDir, 0775);
    FILE *f = fopen(tmp, "wb");
    if (f == nullptr) {
        return false;
    }
    const bool wrote = fwrite(data, 1, size, f) == size;
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    if (!wrote) {
        return false;
    }
    unlink(bak);
    rename(path, bak);
    return rename(tmp, path) == 0;
}

void ScanDirectory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == nullptr) {
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr &&
           s_book_count < kMaxBooks) {
        const char *name = entry->d_name;
        const size_t name_len = strlen(name);
        if (name_len < 5 || name[0] == '.' ||
            strcasecmp(name + name_len - 4, ".txt") != 0) {
            continue;
        }
        BookEntry &book = s_books[s_book_count];
        const int written = snprintf(book.path, sizeof(book.path), "%s/%s",
                                     dir_path, name);
        if (written <= 0 ||
            static_cast<size_t>(written) >= sizeof(book.path)) {
            ESP_LOGW(kTag, "skipping over-long path: %s/%s", dir_path, name);
            continue;
        }
        struct stat st;
        if (stat(book.path, &st) != 0 || st.st_size <= 0) {
            continue;
        }
        book.size = static_cast<uint64_t>(st.st_size);
        // Title = filename without extension, truncated.
        snprintf(book.title, sizeof(book.title), "%.*s",
                 static_cast<int>(name_len - 4), name);
        // Identity: FNV over first 4 KiB + size (matches ContentSource).
        SdContentSource probe;
        if (probe.Open(book.path) != StatusCode::Ok) {
            continue;
        }
        const s3paper::Result<s3paper::ContentHash> hash = probe.Hash();
        probe.Close();
        if (!hash.ok()) {
            continue;
        }
        book.content_hash = hash.value;
        s_book_count++;
    }
    closedir(dir);
}

}  // namespace

StatusCode StorageMount() {
    if (s_card != nullptr) {
        return StatusCode::Ok;
    }
    // Deterministic SPI bus ownership: M5GFX must initialize first (its
    // PaperS3 preset also touches this bus; initializing it after the SD
    // mount de-initialized the live bus and crashed the system).
    const s3paper::Status m5 = EnsureM5Init();
    if (!m5.ok()) {
        ESP_LOGE(kTag, "M5 init before SD mount failed: %s",
                 s3paper::StatusCodeName(m5.code));
        return StatusCode::Busy;
    }
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    if (!s_spi_bus_initialized) {
        spi_bus_config_t bus_cfg = {};
        bus_cfg.mosi_io_num = kPinMosi;
        bus_cfg.miso_io_num = kPinMiso;
        bus_cfg.sclk_io_num = kPinSclk;
        bus_cfg.quadwp_io_num = -1;
        bus_cfg.quadhd_io_num = -1;
        bus_cfg.max_transfer_sz = 4000;
        const esp_err_t bus = spi_bus_initialize(
            static_cast<spi_host_device_t>(host.slot), &bus_cfg,
            SDSPI_DEFAULT_DMA);
        if (bus == ESP_ERR_INVALID_STATE) {
            ESP_LOGI(kTag, "SPI bus already initialized (M5GFX); reusing");
        } else if (bus != ESP_OK) {
            ESP_LOGE(kTag, "spi_bus_initialize failed: %s",
                     esp_err_to_name(bus));
            return StatusCode::Busy;
        }
        s_spi_bus_initialized = true;
    }
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = kPinCs;
    slot_config.host_id = static_cast<spi_host_device_t>(host.slot);
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;  // never format user media
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;
    const esp_err_t ret = esp_vfs_fat_sdspi_mount(
        kMountPoint, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGW(kTag, "mount failed: %s (card missing or unreadable)",
                 esp_err_to_name(ret));
        s_card = nullptr;
        return ret == ESP_FAIL ? StatusCode::CorruptData : StatusCode::Busy;
    }
    ESP_LOGI(kTag, "mounted %s: %llu MiB", kMountPoint,
             (static_cast<unsigned long long>(s_card->csd.capacity) *
              s_card->csd.sector_size) >>
                 20);
    const StatusCode loaded = PositionsLoad();
    if (loaded != StatusCode::Ok) {
        ESP_LOGW(kTag, "positions not loaded: %s (starting fresh)",
                 StatusCodeName(loaded));
    }
    const StatusCode marks = BookmarksLoad();
    if (marks != StatusCode::Ok) {
        ESP_LOGW(kTag, "bookmarks not loaded: %s (starting fresh)",
                 StatusCodeName(marks));
    }
    return StatusCode::Ok;
}

StatusCode StorageUnmount() {
    if (s_card == nullptr) {
        return StatusCode::InvalidArgument;
    }
    esp_vfs_fat_sdcard_unmount(kMountPoint, s_card);
    s_card = nullptr;
    s_book_count = 0;
    ESP_LOGI(kTag, "unmounted");
    return StatusCode::Ok;
}

bool StorageMounted() { return s_card != nullptr; }

void FillSdSnapshot(SdSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->mounted = StorageMounted() ? 1 : 0;
    if (s_card != nullptr) {
        out->capacity_mib = static_cast<uint32_t>(
            (static_cast<uint64_t>(s_card->csd.capacity) *
             s_card->csd.sector_size) >>
            20);
    }
    out->book_count = s_book_count;
    out->position_records = s_positions.count;
    out->position_writes = s_position_writes;
    out->position_write_failures = s_position_write_failures;
}

StatusCode LibraryScan(uint32_t *out_count) {
    if (!StorageMounted()) {
        return StatusCode::Busy;
    }
    s_book_count = 0;
    ScanDirectory("/sdcard/books");
    ScanDirectory("/sdcard");
    // Deterministic order: title, then path as tiebreaker.
    std::sort(s_books, s_books + s_book_count,
              [](const BookEntry &a, const BookEntry &b) {
                  const int t = strcasecmp(a.title, b.title);
                  return t != 0 ? t < 0 : strcmp(a.path, b.path) < 0;
              });
    if (out_count != nullptr) {
        *out_count = s_book_count;
    }
    ESP_LOGI(kTag, "library scan: %u book(s)",
             static_cast<unsigned>(s_book_count));
    return StatusCode::Ok;
}

uint32_t LibraryCount() { return s_book_count; }

const BookEntry *LibraryGet(uint32_t index) {
    return index < s_book_count ? &s_books[index] : nullptr;
}

void LibraryPrint() {
    printf("library: %u book(s)%s\n", static_cast<unsigned>(s_book_count),
           StorageMounted() ? "" : " (card not mounted)");
    for (uint32_t i = 0; i < s_book_count; ++i) {
        printf("  [%u] \"%s\" %llu bytes hash=%08x %s\n",
               static_cast<unsigned>(i), s_books[i].title,
               static_cast<unsigned long long>(s_books[i].size),
               static_cast<unsigned>(s_books[i].content_hash),
               s_books[i].path);
    }
}

StatusCode StorageWriteDemoBook() {
    if (!StorageMounted()) {
        return StatusCode::Busy;
    }
    mkdir("/sdcard/books", 0775);
    const char *path = "/sdcard/books/alice-demo.txt";
    struct stat st;
    if (stat(path, &st) == 0) {
        return StatusCode::Ok;  // never overwrite
    }
    FILE *f = fopen(path, "wb");
    if (f == nullptr) {
        return StatusCode::CorruptData;
    }
    const size_t len = sizeof(kEmbeddedBookText) - 1;
    const bool ok = fwrite(kEmbeddedBookText, 1, len, f) == len;
    fclose(f);
    ESP_LOGI(kTag, "demo book %s: %s", path, ok ? "written" : "FAILED");
    return ok ? StatusCode::Ok : StatusCode::CorruptData;
}

// ---- SdContentSource ----

StatusCode SdContentSource::Open(const char *path) {
    Close();
    FILE *f = fopen(path, "rb");
    if (f == nullptr) {
        return StatusCode::InvalidArgument;
    }
    struct stat st;
    if (fstat(fileno(f), &st) != 0) {
        fclose(f);
        return StatusCode::CorruptData;
    }
    file_ = f;
    size_ = static_cast<uint64_t>(st.st_size);
    hash_ = 0;
    return StatusCode::Ok;
}

void SdContentSource::Close() {
    if (file_ != nullptr) {
        fclose(static_cast<FILE *>(file_));
        file_ = nullptr;
    }
}

s3paper::Result<uint64_t> SdContentSource::Size() {
    if (file_ == nullptr) {
        return s3paper::Result<uint64_t>::Err(StatusCode::Busy);
    }
    return s3paper::Result<uint64_t>::Ok(size_);
}

s3paper::Result<uint32_t> SdContentSource::ReadAt(uint64_t offset,
                                                  uint8_t *buf,
                                                  uint32_t len) {
    if (file_ == nullptr || buf == nullptr) {
        return s3paper::Result<uint32_t>::Err(StatusCode::InvalidArgument);
    }
    if (offset >= size_) {
        return s3paper::Result<uint32_t>::Ok(0);
    }
    FILE *f = static_cast<FILE *>(file_);
    if (fseek(f, static_cast<long>(offset), SEEK_SET) != 0) {
        return s3paper::Result<uint32_t>::Err(StatusCode::CorruptData);
    }
    const size_t n = fread(buf, 1, len, f);
    if (n == 0 && ferror(f)) {
        clearerr(f);
        return s3paper::Result<uint32_t>::Err(StatusCode::CorruptData);
    }
    return s3paper::Result<uint32_t>::Ok(static_cast<uint32_t>(n));
}

s3paper::Result<s3paper::ContentHash> SdContentSource::Hash() {
    if (file_ == nullptr) {
        return s3paper::Result<s3paper::ContentHash>::Err(StatusCode::Busy);
    }
    if (hash_ != 0) {
        return s3paper::Result<s3paper::ContentHash>::Ok(hash_);
    }
    uint8_t head[4096];
    const s3paper::Result<uint32_t> n = ReadAt(0, head, sizeof(head));
    if (!n.ok()) {
        return s3paper::Result<s3paper::ContentHash>::Err(n.code);
    }
    uint32_t hash = s3paper::Fnv1a(head, n.value);
    const uint64_t s = size_;
    hash = s3paper::Fnv1a(reinterpret_cast<const uint8_t *>(&s), sizeof(s),
                          hash);
    hash_ = hash;
    return s3paper::Result<s3paper::ContentHash>::Ok(hash);
}

// ---- Position persistence ----

StatusCode PositionsLoad() {
    std::memset(&s_positions, 0, sizeof(s_positions));
    FILE *f = fopen(kPositionsPath, "rb");
    if (f == nullptr) {
        // Try the backup generation before giving up.
        f = fopen(kPositionsBak, "rb");
        if (f == nullptr) {
            return StatusCode::InvalidArgument;  // fresh card: no records
        }
        ESP_LOGW(kTag, "primary positions missing; using backup");
    }
    PositionsFile loaded;
    const size_t n = fread(&loaded, 1, sizeof(loaded), f);
    fclose(f);
    if (n != sizeof(loaded) || loaded.magic != kPositionsMagic ||
        loaded.version != kPositionsVersion ||
        loaded.count > kMaxPositions || loaded.crc != PositionsCrc(loaded)) {
        ESP_LOGW(kTag, "positions file invalid (len=%u); ignoring",
                 static_cast<unsigned>(n));
        return StatusCode::CorruptData;
    }
    s_positions = loaded;
    ESP_LOGI(kTag, "loaded %u reading position(s)",
             static_cast<unsigned>(s_positions.count));
    return StatusCode::Ok;
}

StatusCode PositionsSave() {
    if (!StorageMounted()) {
        return StatusCode::Busy;
    }
    s_positions.magic = kPositionsMagic;
    s_positions.version = kPositionsVersion;
    s_positions.crc = PositionsCrc(s_positions);
    if (!AtomicWrite(kPositionsTmp, kPositionsBak, kPositionsPath,
                     &s_positions, sizeof(s_positions))) {
        s_position_write_failures++;
        return StatusCode::CorruptData;
    }
    s_position_writes++;
    s_positions_dirty = false;
    return StatusCode::Ok;
}

StatusCode BookmarksSave() {
    if (!StorageMounted()) {
        return StatusCode::Busy;
    }
    s_bookmarks.magic = kBookmarksMagic;
    s_bookmarks.version = kBookmarksVersion;
    s_bookmarks.crc = BookmarksCrc(s_bookmarks);
    if (!AtomicWrite(kBookmarksTmp, kBookmarksBak, kBookmarksPath,
                     &s_bookmarks, sizeof(s_bookmarks))) {
        s_position_write_failures++;
        return StatusCode::CorruptData;
    }
    s_position_writes++;
    s_bookmarks_dirty = false;
    return StatusCode::Ok;
}

void StorageFlushIfDue(int64_t now_us) {
    if (!s_positions_dirty && !s_bookmarks_dirty) {
        return;
    }
    if (now_us - s_dirty_since_us < kFlushAfterUs) {
        return;
    }
    StorageFlushNow();
}

void StorageFlushNow() {
    if (s_positions_dirty) {
        (void)PositionsSave();
    }
    if (s_bookmarks_dirty) {
        (void)BookmarksSave();
    }
}

// ---- Last-book record ----

constexpr uint32_t kLastBookMagic = 0x53334C42;  // "S3LB"
constexpr char kLastBookPath[] = "/sdcard/.s3paper/lastbook.bin";
constexpr char kLastBookTmp[] = "/sdcard/.s3paper/lastbook.tmp";
constexpr char kLastBookBak[] = "/sdcard/.s3paper/lastbook.bak";

struct LastBookFile {
    uint32_t magic;
    uint32_t version;
    char path[96];  // "" = embedded book
    uint32_t crc;
};

void LastBookStore(const char *sd_path_or_empty) {
    if (!StorageMounted()) {
        return;
    }
    LastBookFile record{};
    record.magic = kLastBookMagic;
    record.version = 1;
    snprintf(record.path, sizeof(record.path), "%s", sd_path_or_empty);
    record.crc = s3paper::Fnv1a(reinterpret_cast<const uint8_t *>(&record),
                                offsetof(LastBookFile, crc));
    (void)AtomicWrite(kLastBookTmp, kLastBookBak, kLastBookPath, &record,
                      sizeof(record));
}

bool LastBookGet(char *out, uint32_t out_size) {
    FILE *f = fopen(kLastBookPath, "rb");
    if (f == nullptr) {
        f = fopen(kLastBookBak, "rb");
        if (f == nullptr) {
            return false;
        }
    }
    LastBookFile record;
    const size_t n = fread(&record, 1, sizeof(record), f);
    fclose(f);
    if (n != sizeof(record) || record.magic != kLastBookMagic ||
        record.version != 1 ||
        record.crc != s3paper::Fnv1a(
                          reinterpret_cast<const uint8_t *>(&record),
                          offsetof(LastBookFile, crc))) {
        return false;
    }
    record.path[sizeof(record.path) - 1] = '\0';
    snprintf(out, out_size, "%s", record.path);
    return true;
}

StatusCode BookmarksLoad() {
    std::memset(&s_bookmarks, 0, sizeof(s_bookmarks));
    FILE *f = fopen(kBookmarksPath, "rb");
    if (f == nullptr) {
        f = fopen(kBookmarksBak, "rb");
        if (f == nullptr) {
            return StatusCode::InvalidArgument;
        }
        ESP_LOGW(kTag, "primary bookmarks missing; using backup");
    }
    BookmarksFile loaded;
    const size_t n = fread(&loaded, 1, sizeof(loaded), f);
    fclose(f);
    if (n != sizeof(loaded) || loaded.magic != kBookmarksMagic ||
        loaded.version != kBookmarksVersion ||
        loaded.count > kMaxBookmarks || loaded.crc != BookmarksCrc(loaded)) {
        ESP_LOGW(kTag, "bookmarks file invalid; ignoring");
        return StatusCode::CorruptData;
    }
    s_bookmarks = loaded;
    ESP_LOGI(kTag, "loaded %u bookmark(s)",
             static_cast<unsigned>(s_bookmarks.count));
    return StatusCode::Ok;
}

StatusCode BookmarkToggle(s3paper::ContentHash content,
                          const s3paper::TextLocator &locator,
                          bool *now_set) {
    for (uint32_t i = 0; i < s_bookmarks.count; ++i) {
        if (s_bookmarks.records[i].content == content &&
            s_bookmarks.records[i].byte_offset == locator.byte_offset) {
            s_bookmarks.records[i] =
                s_bookmarks.records[--s_bookmarks.count];
            s_bookmarks_dirty = true;
            s_dirty_since_us = 0;  // bookmark changes flush promptly
            if (now_set != nullptr) {
                *now_set = false;
            }
            return StatusCode::Ok;
        }
    }
    if (s_bookmarks.count >= kMaxBookmarks) {
        return StatusCode::CapacityExceeded;
    }
    s_bookmarks.records[s_bookmarks.count++] =
        BookmarkRecord{content, locator.byte_offset, locator.context_hash};
    s_bookmarks_dirty = true;
    s_dirty_since_us = 0;
    if (now_set != nullptr) {
        *now_set = true;
    }
    return StatusCode::Ok;
}

bool BookmarkIsSet(s3paper::ContentHash content, uint64_t byte_offset) {
    for (uint32_t i = 0; i < s_bookmarks.count; ++i) {
        if (s_bookmarks.records[i].content == content &&
            s_bookmarks.records[i].byte_offset == byte_offset) {
            return true;
        }
    }
    return false;
}

uint32_t BookmarkCountFor(s3paper::ContentHash content) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < s_bookmarks.count; ++i) {
        if (s_bookmarks.records[i].content == content) {
            count++;
        }
    }
    return count;
}

bool BookmarkGet(s3paper::ContentHash content, uint32_t index,
                 s3paper::TextLocator *out) {
    uint32_t seen = 0;
    for (uint32_t i = 0; i < s_bookmarks.count; ++i) {
        if (s_bookmarks.records[i].content != content) {
            continue;
        }
        if (seen++ == index) {
            out->byte_offset = s_bookmarks.records[i].byte_offset;
            out->context_hash = s_bookmarks.records[i].context_hash;
            return true;
        }
    }
    return false;
}

void BookmarksPrint(s3paper::ContentHash content) {
    uint32_t shown = 0;
    for (uint32_t i = 0; i < s_bookmarks.count; ++i) {
        if (s_bookmarks.records[i].content != content) {
            continue;
        }
        printf("  bookmark[%u] offset=%llu\n", static_cast<unsigned>(shown++),
               static_cast<unsigned long long>(
                   s_bookmarks.records[i].byte_offset));
    }
    printf("bookmarks: %u for this book (%u total)\n",
           static_cast<unsigned>(shown),
           static_cast<unsigned>(s_bookmarks.count));
}

bool PositionLookup(s3paper::ContentHash content, s3paper::TextLocator *out) {
    for (uint32_t i = 0; i < s_positions.count; ++i) {
        if (s_positions.records[i].content == content) {
            out->byte_offset = s_positions.records[i].byte_offset;
            out->context_hash = s_positions.records[i].context_hash;
            return true;
        }
    }
    return false;
}

void PositionStore(s3paper::ContentHash content,
                   const s3paper::TextLocator &locator) {
    if (!s_positions_dirty && !s_bookmarks_dirty) {
        s_dirty_since_us = esp_timer_get_time();
    }
    s_positions_dirty = true;
    for (uint32_t i = 0; i < s_positions.count; ++i) {
        if (s_positions.records[i].content == content) {
            s_positions.records[i].byte_offset = locator.byte_offset;
            s_positions.records[i].context_hash = locator.context_hash;
            return;
        }
    }
    if (s_positions.count < kMaxPositions) {
        s_positions.records[s_positions.count++] =
            PositionRecord{content, locator.byte_offset,
                           locator.context_hash};
    } else {
        // Explicit capacity policy: replace the first (oldest) slot.
        s_positions.records[0] =
            PositionRecord{content, locator.byte_offset,
                           locator.context_hash};
    }
}

}  // namespace reader
