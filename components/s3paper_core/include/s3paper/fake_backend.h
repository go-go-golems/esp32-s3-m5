// Deterministic fake display backend.
//
// Records a normalized text trace of every present and draw op into a
// caller-provided buffer. Host tests assert against the trace; on-device
// diagnostics print it. No pixels are produced.
#pragma once

#include <stdint.h>

#include "s3paper/display_backend.h"

namespace s3paper {

class FakeBackend : public DisplayBackend {
  public:
    FakeBackend(char *trace_buffer, uint32_t trace_capacity,
                Size physical_size);

    const char *Name() const override { return "fake"; }
    Status Init() override;
    PresentResult Present(const RenderFrame &frame,
                          PresentIntent intent) override;
    BackendState GetState() const override;

    const char *trace() const { return trace_ ? trace_ : ""; }
    uint32_t trace_len() const { return trace_len_; }
    // True if any Append since the last ClearTrace didn't fit.
    bool trace_truncated() const { return trace_truncated_; }
    void ClearTrace();

  private:
    void Append(const char *fmt, ...);

    char *trace_;
    uint32_t trace_capacity_;
    uint32_t trace_len_ = 0;
    bool trace_truncated_ = false;
    bool initialized_ = false;
    Size physical_size_;
    uint32_t frames_presented_ = 0;
};

}  // namespace s3paper
