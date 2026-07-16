#include "app_storage.h"

#include <cstring>

#include "app_display.h"
#include "app_reader_book.h"

namespace reader {
namespace {

// The SPI bus is shared with the display; the component must never mount
// before M5GFX owns the bus (ESP-50 diary S9). Injected as the pre-mount
// hook so the component itself stays display-agnostic.
s3paper::Status PreMountEnsureDisplay() { return EnsureM5Init(); }

void ConfigureOnce() {
    static bool configured = false;
    if (configured) {
        return;
    }
    s3paper_storage::StorageConfig config{};
    config.pre_mount = &PreMountEnsureDisplay;
    config.seed_path = "/sdcard/books/alice-demo.txt";
    config.seed_text = kEmbeddedBookText;
    config.seed_len = sizeof(kEmbeddedBookText) - 1;
    s3paper_storage::StorageConfigure(config);
    configured = true;
}

}  // namespace

StatusCode StorageMount() {
    ConfigureOnce();
    return s3paper_storage::StorageMount();
}

StatusCode StorageWriteDemoBook() {
    ConfigureOnce();
    return s3paper_storage::StorageWriteSeedBook();
}

void FillSdSnapshot(SdSnapshot *out) {
    StorageStats stats;
    s3paper_storage::GetStats(&stats);
    std::memset(out, 0, sizeof(*out));
    out->mounted = stats.mounted;
    out->capacity_mib = stats.capacity_mib;
    out->book_count = stats.book_count;
    out->position_records = stats.position_records;
    out->position_writes = stats.position_writes;
    out->position_write_failures = stats.position_write_failures;
    out->scan_cached = stats.scan_cached;
    out->scan_hashed = stats.scan_hashed;
    out->scan_ms = stats.scan_ms;
    out->catalog_writes = stats.catalog_writes;
}

}  // namespace reader
