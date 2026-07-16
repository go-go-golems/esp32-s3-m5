#include "s3paper/frame_arena.h"

#include <cstring>

namespace s3paper {

Result<uint32_t> FrameArena::Alloc(uint32_t size, uint32_t align) {
    if (align == 0 || (align & (align - 1)) != 0) {
        return Result<uint32_t>::Err(StatusCode::InvalidArgument);
    }
    if (buffer_ == nullptr) {
        return Result<uint32_t>::Err(StatusCode::CapacityExceeded);
    }
    const uint64_t aligned =
        (static_cast<uint64_t>(used_) + (align - 1)) & ~static_cast<uint64_t>(align - 1);
    const uint64_t end = aligned + size;
    if (end > capacity_) {
        return Result<uint32_t>::Err(StatusCode::CapacityExceeded);
    }
    used_ = static_cast<uint32_t>(end);
    if (used_ > high_water_) {
        high_water_ = used_;
    }
    return Result<uint32_t>::Ok(static_cast<uint32_t>(aligned));
}

Result<uint32_t> FrameArena::PushBytes(const void *src, uint32_t size,
                                       uint32_t align) {
    if (src == nullptr && size > 0) {
        return Result<uint32_t>::Err(StatusCode::InvalidArgument);
    }
    const Result<uint32_t> slot = Alloc(size, align);
    if (!slot.ok()) {
        return slot;
    }
    if (size > 0) {
        std::memcpy(buffer_ + slot.value, src, size);
    }
    return slot;
}

}  // namespace s3paper
