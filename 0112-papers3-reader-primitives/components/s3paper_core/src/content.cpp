#include "s3paper/content.h"

#include <cstring>

namespace s3paper {

uint32_t Fnv1a(const uint8_t *data, uint32_t len, uint32_t seed) {
    uint32_t hash = seed;
    for (uint32_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

Result<uint32_t> MemoryContentSource::ReadAt(uint64_t offset, uint8_t *buf,
                                             uint32_t len) {
    if (buf == nullptr) {
        return Result<uint32_t>::Err(StatusCode::InvalidArgument);
    }
    if (offset >= size_) {
        return Result<uint32_t>::Ok(0);
    }
    const uint64_t available = size_ - offset;
    const uint32_t n =
        static_cast<uint32_t>(available < len ? available : len);
    std::memcpy(buf, data_ + offset, n);
    return Result<uint32_t>::Ok(n);
}

Result<ContentHash> MemoryContentSource::Hash() {
    const uint32_t head =
        static_cast<uint32_t>(size_ < 4096 ? size_ : 4096);
    uint32_t hash = Fnv1a(data_, head);
    // Fold in the size so truncations change identity.
    const uint64_t s = size_;
    hash = Fnv1a(reinterpret_cast<const uint8_t *>(&s), sizeof(s), hash);
    return Result<ContentHash>::Ok(hash);
}

}  // namespace s3paper
