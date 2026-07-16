// Fixed-capacity frame arena: owns draw-op payload bytes (text, bitmaps)
// from emission through presentation.
//
// The arena never allocates; the caller provides the buffer (PSRAM on
// device, heap in host tests). Overflow is an explicit CapacityExceeded.
#pragma once

#include <stdint.h>

#include "s3paper/status.h"

namespace s3paper {

class FrameArena {
  public:
    FrameArena(uint8_t *buffer, uint32_t capacity)
        : buffer_(buffer), capacity_(buffer ? capacity : 0) {}

    // Reserves size bytes aligned to `align` (power of two). Returns the
    // offset of the reservation.
    Result<uint32_t> Alloc(uint32_t size, uint32_t align = 4);

    // Copies bytes into the arena and returns their offset.
    Result<uint32_t> PushBytes(const void *src, uint32_t size,
                               uint32_t align = 1);

    uint8_t *Data(uint32_t offset) { return buffer_ + offset; }
    const uint8_t *Data(uint32_t offset) const { return buffer_ + offset; }

    // Invalidates every offset previously handed out. Only legal between
    // frames; the owner task calls this after present completes.
    void Reset() { used_ = 0; }

    uint32_t used() const { return used_; }
    uint32_t capacity() const { return capacity_; }
    uint32_t high_water() const { return high_water_; }

  private:
    uint8_t *buffer_;
    uint32_t capacity_;
    uint32_t used_ = 0;
    uint32_t high_water_ = 0;
};

}  // namespace s3paper
