// Display backend boundary.
//
// Widgets and layout never choose EPD waveforms; they express PresentIntent
// and the refresh planner (Phase 3) maps intent to the qualified backend
// policy. Phase 2 backends map intents naively. Only backend implementations
// may include vendor headers; this interface stays pure.
#pragma once

#include <stdint.h>

#include "s3paper/draw_ops.h"
#include "s3paper/status.h"

namespace s3paper {

enum class PresentIntent : uint8_t {
    InteractiveInk = 0,
    TextRegion,
    TextPage,
    ImageQuality,
    CleanFull,
};

const char *PresentIntentName(PresentIntent intent);

struct PresentResult {
    StatusCode status;
    FrameId frame_id;
    uint32_t ops_drawn;
    uint32_t ops_skipped;  // unsupported kinds (explicit, not silent)
    Rect damage;
    uint32_t render_us;
    uint32_t wait_us;  // panel busy/flush wait
};

struct BackendState {
    bool initialized;
    Size physical_size;
    uint32_t frames_presented;
};

class DisplayBackend {
  public:
    virtual ~DisplayBackend() = default;
    virtual const char *Name() const = 0;
    virtual Status Init() = 0;
    virtual PresentResult Present(const RenderFrame &frame,
                                  PresentIntent intent) = 0;
    virtual BackendState GetState() const = 0;
};

}  // namespace s3paper
