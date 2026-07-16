// Content sources (Phase 6/7): random-access byte access to book content.
//
// Pure interface: the SD adapter lives in firmware; host tests and the
// embedded fixture use MemoryContentSource.
#pragma once

#include <stdint.h>

#include "s3paper/status.h"

namespace s3paper {

using ContentHash = uint32_t;

class ContentSource {
  public:
    virtual ~ContentSource() = default;
    virtual Result<uint64_t> Size() = 0;
    // Reads up to len bytes at offset; returns the count actually read
    // (short reads only at end of content).
    virtual Result<uint32_t> ReadAt(uint64_t offset, uint8_t *buf,
                                    uint32_t len) = 0;
    // Stable identity: FNV-1a over size and the first 4 KiB. Cheap enough
    // to compute on open, stable across sessions for unchanged content.
    virtual Result<ContentHash> Hash() = 0;
};

class MemoryContentSource : public ContentSource {
  public:
    MemoryContentSource(const uint8_t *data, uint64_t size)
        : data_(data), size_(data ? size : 0) {}
    MemoryContentSource(const char *text, uint64_t size)
        : MemoryContentSource(reinterpret_cast<const uint8_t *>(text), size) {}

    Result<uint64_t> Size() override { return Result<uint64_t>::Ok(size_); }
    Result<uint32_t> ReadAt(uint64_t offset, uint8_t *buf,
                            uint32_t len) override;
    Result<ContentHash> Hash() override;

  private:
    const uint8_t *data_;
    uint64_t size_;
};

// FNV-1a helper shared by content adapters and locator context hashes.
uint32_t Fnv1a(const uint8_t *data, uint32_t len, uint32_t seed = 2166136261u);

}  // namespace s3paper
