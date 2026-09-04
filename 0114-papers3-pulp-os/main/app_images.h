// Image gallery module (ESP-54): stores browser-converted 4-bit grayscale
// frames (.g4) on the SD card, catalogs them, and displays them on the
// e-ink panel via the DrawOpKind::Bitmap path (implemented in Phase 4).
//
// .g4 format (12-byte header + packed 4-bit pixels, 2 px/byte, high nibble
// first, row-major, width padded to even bytes):
//   0  4  magic "G4IM"
//   4  2  width  (uint16 LE, 540)
//   6  2  height (uint16 LE, 960)
//   8  1  depth  (4)
//   9  1  version (1)
//  10  2  reserved (0)
//  12  .  pixel data (ceil(width/2) * height bytes)
//
// Task contract:
//  - Catalog read/display/remove are OWNER-ONLY.
//  - The POST upload body is streamed to SD on the httpd task (the one
//    sanctioned off-owner SD WRITE, to /sdcard/images/ only, mirroring
//    ESP-53's sanctioned off-owner SD read for static files). The httpd
//    task fills the upload result mailbox and posts ModuleDone{Images}.
//  - The owner owns the catalog and the JS completion callback.
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "app_events.h"

namespace pulp {

// .g4 header layout (mirrors the on-wire/on-card format).
struct G4Header {
    char magic[4];       // "G4IM"
    uint16_t width;
    uint16_t height;
    uint8_t depth;
    uint8_t version;
    uint16_t reserved;
} __attribute__((packed));
static_assert(sizeof(G4Header) == 12, "G4 header must be 12 bytes");

constexpr uint32_t kImageMaxBytes = 280 * 1024;  // hard upload cap (~280 KiB)
constexpr uint32_t kImageMaxCount = 64;          // catalog hard cap
constexpr const char *kImagesDir = "/sdcard/images";
constexpr const char *kImagesIndex = "/sdcard/images/index.txt";

// Validates a .g4 header: magic, dims 540x960, depth 4, version 1.
bool G4ValidateHeader(const G4Header *h);

// ---- owner-only catalog + display ----

uint32_t ImagesCount();              // cached count; rescans on demand
const char *ImagesName(uint32_t i);   // basename; nullptr out of range
StatusCode ImagesDisplay(const char *name);  // blit a stored frame
StatusCode ImagesRemove(const char *name);   // delete frame + catalog entry
void ImagesPrintCatalog();           // console: list names

// ---- upload (httpd task fills mailbox, owner drains) ----

struct ImagesUploadResult {
    char name[32];   // basename of the written file
    uint32_t bytes;
    int32_t err;     // 0 ok, module-specific otherwise
};

// Called by the httpd POST handler (off-owner) to stage the completion
// result. The owner reads it after ModuleDone{Images} arrives.
void ImagesSetUploadResult(const ImagesUploadResult &res);
const ImagesUploadResult &ImagesGetUploadResult();

// ---- status ----

struct ImagesSnapshot;
void FillImagesSnapshot(ImagesSnapshot *out);

// Called after a successful upload (owner-side) to refresh the cache.
void ImagesCatalogInvalidate();

}  // namespace pulp
