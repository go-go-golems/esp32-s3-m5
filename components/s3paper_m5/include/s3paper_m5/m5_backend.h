// M5GFX display backend: the ONLY module that may call M5.Display.
//
// Phase 2 transaction shell (design doc §Phase 2): wait, set mode, batched
// write, wait — with an explicit busy timeout. Intent-to-waveform mapping is
// deliberately naive; the Phase 3 refresh planner replaces it. No optical
// quality claims: the panel is unqualified and this backend only reports
// software-side present metrics.
#pragma once

#include "s3paper/display_backend.h"
#include "s3paper/input.h"

namespace s3paper {

class M5Backend : public DisplayBackend {
  public:
    const char *Name() const override { return "m5"; }
    // Calls M5.begin() on first use; safe to call once from the owner task.
    Status Init() override;
    PresentResult Present(const RenderFrame &frame,
                          PresentIntent intent) override;
    BackendState GetState() const override;

    // Polls the GT911 via M5Unified. Coordinates are already in the panel's
    // current logical orientation. Owner-task-only (M5.update mutates M5
    // state). Returns false when not initialized.
    bool ReadTouch(PointerSample *out);

  private:
    bool initialized_ = false;
    Size physical_size_{0, 0};
    uint32_t frames_presented_ = 0;
};

}  // namespace s3paper
