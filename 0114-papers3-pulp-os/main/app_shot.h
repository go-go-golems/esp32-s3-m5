// ESP-56: owner-side framebuffer capture (QOI over USB serial).
#pragma once

#include <stdint.h>

namespace pulp {

// Streams the current M5GFX framebuffer as QOI_BEGIN <len>\n<bytes>\nQOI_END.
// Owner-task-only. Returns false when the encoder failed.
bool ShotToConsole(uint32_t *out_len);

}  // namespace pulp
