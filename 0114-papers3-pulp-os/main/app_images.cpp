// Image gallery catalog + display (ESP-54). See app_images.h for the .g4
// format and the task contract.
//
// Catalog: /sdcard/images/<ts>.g4 files plus index.txt (one basename per
// line). The catalog is cached and rescanned on demand (miss, upload,
// remove). Display reads a .g4, validates the header, copies the packed
// pixels into PSRAM, and builds a one-Bitmap-op frame presented clean-full.
//
// Upload: the httpd POST handler (net_serve.cpp, ESP-54 P3) streams the
// body to a .g4 file on the httpd task, then stages the result here and
// posts ModuleDone{Images}. The owner drains the mailbox into the JS
// received() callback.
#include "app_images.h"

#include <cstring>
#include <cstdio>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "app_owner.h"
#include "s3paper/draw_ops.h"
#include "s3paper/frame_arena.h"
#include "s3paper_runtime/runtime.h"
#include "s3paper_storage/storage.h"

namespace pulp {
namespace {

const char *kTag = "images";

constexpr uint32_t kCatalogCap = kImageMaxCount;
constexpr uint32_t kNameLen = 32;

struct CatalogEntry {
    char name[kNameLen];  // basename, e.g. "1716000000.g4"
};
CatalogEntry s_catalog[kCatalogCap];
uint32_t s_catalog_count = 0;
bool s_catalog_loaded = false;

// Upload result mailbox (filled by httpd, drained by owner).
ImagesUploadResult s_upload_result{};

// Loaded image buffer for display (PSRAM). One in flight at a time.
uint8_t *s_disp_buf = nullptr;
size_t s_disp_buf_cap = 0;

// Rescans /sdcard/images and rebuilds the catalog cache. The index.txt is
// the source of truth; if it is missing or stale, rebuild it from the dir.
void CatalogRescan() {
    s_catalog_count = 0;
    s_catalog_loaded = true;
    if (!s3paper_storage::StorageMounted()) {
        return;
    }
    mkdir(kImagesDir, 0775);
    DIR *d = opendir(kImagesDir);
    if (d == nullptr) {
        return;
    }
    struct dirent *e;
    while (s_catalog_count < kCatalogCap &&
           (e = readdir(d)) != nullptr) {
        const char *nm = e->d_name;
        if (nm[0] == '.') {
            continue;
        }
        const size_t len = strlen(nm);
        if (len < 3 || len >= kNameLen) {
            continue;
        }
        // Only .g4 frames belong in the gallery.
        if (len >= 3 && strcmp(nm + len - 3, ".g4") == 0) {
            snprintf(s_catalog[s_catalog_count].name, kNameLen, "%.31s",
                     nm);
            s_catalog_count++;
        }
    }
    closedir(d);
    ESP_LOGI(kTag, "catalog: %u image(s)",
             static_cast<unsigned>(s_catalog_count));
}

bool G4MagicOk(const char *m) {
    return m[0] == 'G' && m[1] == '4' && m[2] == 'I' && m[3] == 'M';
}

}  // namespace

bool G4ValidateHeader(const G4Header *h) {
    if (h == nullptr) {
        return false;
    }
    if (!G4MagicOk(h->magic)) {
        return false;
    }
    if (h->width != 540 || h->height != 960) {
        return false;
    }
    if (h->depth != 4 || h->version != 1) {
        return false;
    }
    return true;
}

uint32_t ImagesCount() {
    if (!s_catalog_loaded) {
        CatalogRescan();
    }
    return s_catalog_count;
}

const char *ImagesName(uint32_t i) {
    if (!s_catalog_loaded) {
        CatalogRescan();
    }
    return i < s_catalog_count ? s_catalog[i].name : nullptr;
}

StatusCode ImagesDisplay(const char *name) {
    AssertOwner();
    if (!s3paper_storage::StorageMounted()) {
        return StatusCode::Busy;
    }
    if (name == nullptr || name[0] == '\0') {
        return StatusCode::InvalidArgument;
    }
    char path[64];
    snprintf(path, sizeof(path), "%s/%s", kImagesDir, name);
    FILE *f = fopen(path, "rb");
    if (f == nullptr) {
        ESP_LOGW(kTag, "display: %s not found", path);
        return StatusCode::InvalidArgument;
    }
    G4Header hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1 || !G4ValidateHeader(&hdr)) {
        fclose(f);
        ESP_LOGW(kTag, "display: %s bad header", path);
        return StatusCode::CorruptData;
    }
    const uint32_t row_bytes = (hdr.width + 1) / 2;
    const size_t pixel_bytes = static_cast<size_t>(row_bytes) * hdr.height;
    // Grow the PSRAM display buffer if needed.
    if (s_disp_buf == nullptr || s_disp_buf_cap < pixel_bytes) {
        if (s_disp_buf != nullptr) {
            heap_caps_free(s_disp_buf);
            s_disp_buf = nullptr;
        }
        s_disp_buf = static_cast<uint8_t *>(
            heap_caps_malloc(pixel_bytes, MALLOC_CAP_SPIRAM));
        s_disp_buf_cap = s_disp_buf != nullptr ? pixel_bytes : 0;
    }
    if (s_disp_buf == nullptr) {
        fclose(f);
        return StatusCode::OutOfMemory;
    }
    if (fread(s_disp_buf, 1, pixel_bytes, f) != pixel_bytes) {
        fclose(f);
        return StatusCode::CorruptData;
    }
    fclose(f);

    // Build a one-Bitmap-op frame and present it clean-full. A new image is
    // a large grayscale change; ghosting is worst, so CleanFull forces a
    // re-blank. The arena (raised to 320 KiB in app_owner) holds the copy.
    const s3paper::Status m5 = s3paper_runtime::EnsureM5Init();
    if (!m5.ok()) {
        return m5.code;
    }
    s3paper::FrameBuilder &fb = s3paper_runtime::FrameBuilderRef();
    fb.Begin();
    const s3paper::Rect full{0, 0, static_cast<int32_t>(hdr.width),
                             static_cast<int32_t>(hdr.height)};
    const s3paper::Status blit =
        fb.Bitmap(full, s_disp_buf, static_cast<uint32_t>(pixel_bytes),
                  static_cast<int32_t>(row_bytes));
    if (!blit.ok()) {
        return blit.code;
    }
    const s3paper::Result<s3paper::RenderFrame> frame =
        s3paper_runtime::FinishFrame();
    if (!frame.ok()) {
        return frame.code;
    }
    const s3paper_runtime::PlannedPresent presented =
        s3paper_runtime::PresentFramePlanned(
            frame.value, s3paper::PresentIntent::CleanFull, true);
    ESP_LOGI(kTag, "display %s: %u px %s ops=%u",
             name, static_cast<unsigned>(pixel_bytes),
             StatusCodeName(presented.present.status),
             static_cast<unsigned>(frame.value.op_count));
    return presented.present.status;
}

StatusCode ImagesRemove(const char *name) {
    AssertOwner();
    if (name == nullptr || name[0] == '\0') {
        return StatusCode::InvalidArgument;
    }
    if (strstr(name, "..") != nullptr || strchr(name, '/') != nullptr) {
        return StatusCode::InvalidArgument;
    }
    char path[64];
    snprintf(path, sizeof(path), "%s/%s", kImagesDir, name);
    if (unlink(path) != 0) {
        return StatusCode::InvalidArgument;
    }
    // Drop from the in-memory catalog so the next count()/name(i) is fresh.
    for (uint32_t i = 0; i < s_catalog_count; ++i) {
        if (strcmp(s_catalog[i].name, name) == 0) {
            for (uint32_t j = i; j + 1 < s_catalog_count; ++j) {
                s_catalog[j] = s_catalog[j + 1];
            }
            s_catalog_count--;
            break;
        }
    }
    ESP_LOGI(kTag, "removed %s (%u remain)", name,
             static_cast<unsigned>(s_catalog_count));
    return StatusCode::Ok;
}

void ImagesPrintCatalog() {
    if (!s_catalog_loaded) {
        CatalogRescan();
    }
    printf("images: %u on card\n",
           static_cast<unsigned>(s_catalog_count));
    for (uint32_t i = 0; i < s_catalog_count; ++i) {
        printf("  img[%u]: %s\n", static_cast<unsigned>(i),
               s_catalog[i].name);
    }
}

void ImagesSetUploadResult(const ImagesUploadResult &res) {
    s_upload_result = res;
}

const ImagesUploadResult &ImagesGetUploadResult() {
    return s_upload_result;
}

void FillImagesSnapshot(ImagesSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->sd_ok = s3paper_storage::StorageMounted() ? 1 : 0;
    out->count = ImagesCount();
    out->last_bytes = s_upload_result.bytes;
    out->upload_busy = 0;  // set by the httpd handler while streaming
}

// Called after a successful upload (owner-side) to refresh the cache.
void ImagesCatalogInvalidate() {
    s_catalog_loaded = false;
}

}  // namespace pulp
