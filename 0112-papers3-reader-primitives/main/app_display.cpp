#include "app_display.h"

#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "s3paper/fake_backend.h"
#include "s3paper/frame_arena.h"
#include "s3paper/frame_builder.h"
#include "s3paper_m5/m5_backend.h"

namespace reader {
namespace {

const char *kTag = "display";

// PaperS3 portrait logical viewport.
constexpr s3paper::Size kViewport{540, 960};
constexpr uint32_t kOpCapacity = 512;
constexpr uint32_t kArenaCapacity = 32 * 1024;
constexpr uint32_t kTraceCapacity = 16 * 1024;

s3paper::DrawOp *s_ops = nullptr;
uint8_t *s_arena_buf = nullptr;
char *s_trace_buf = nullptr;
s3paper::FrameArena *s_arena = nullptr;
s3paper::FrameBuilder *s_builder = nullptr;
s3paper::FakeBackend *s_fake = nullptr;
s3paper::M5Backend *s_m5 = nullptr;
s3paper::FrameId s_next_frame_id = 1;

// Deterministic primitive fixture (ticket task tb0m): background, borders,
// corner markers, width ladder 1..16, gray ladder, checkerboard, a clipped
// fill, and a glyph run. Same scene for fake and M5 backends.
s3paper::Status BuildFixture(s3paper::FrameBuilder &fb) {
    using s3paper::Rect;
    s3paper::Status st;
    fb.Begin();
    if (st = fb.FillRect(Rect{0, 0, 540, 960}, 255); !st.ok()) return st;
    if (st = fb.StrokeRect(Rect{0, 0, 540, 960}, 0, 3); !st.ok()) return st;
    // Corner markers.
    for (const Rect corner : {Rect{0, 0, 24, 24}, Rect{516, 0, 24, 24},
                              Rect{0, 936, 24, 24}, Rect{516, 936, 24, 24}}) {
        if (st = fb.FillRect(corner, 0); !st.ok()) return st;
    }
    // Width ladder: bars of width 1..16 (Issue-181-style narrow damage).
    int32_t x = 40;
    for (int32_t w = 1; w <= 16; ++w) {
        if (st = fb.FillRect(Rect{x, 60, w, 120}, 0); !st.ok()) return st;
        x += w + 12;
    }
    // Gray ladder: 16 steps.
    for (int32_t i = 0; i < 16; ++i) {
        const auto gray = static_cast<s3paper::Gray8>(i * 17);
        if (st = fb.FillRect(Rect{40 + i * 29, 220, 29, 80}, gray); !st.ok())
            return st;
    }
    // Checkerboard 8x8 cells of 16 px.
    for (int32_t cy = 0; cy < 8; ++cy) {
        for (int32_t cx = 0; cx < 8; ++cx) {
            if (((cx + cy) & 1) == 0) {
                if (st = fb.FillRect(
                        Rect{40 + cx * 16, 340 + cy * 16, 16, 16}, 0);
                    !st.ok())
                    return st;
            }
        }
    }
    // Clip demonstration: a big rect confined to a small window.
    if (st = fb.PushClip(Rect{300, 340, 100, 100}); !st.ok()) return st;
    if (st = fb.FillRect(Rect{0, 0, 540, 960}, 128); !st.ok()) return st;
    if (st = fb.PopClip(); !st.ok()) return st;
    // Lines.
    if (st = fb.HLine(40, 500, 460, 0); !st.ok()) return st;
    if (st = fb.VLine(270, 520, 200, 0); !st.ok()) return st;
    // Text.
    static const char kText[] = "s3paper phase2 fixture";
    if (st = fb.GlyphRun(Rect{40, 760, 460, 40}, 792, 0, 24, kText,
                         sizeof(kText) - 1, 0);
        !st.ok())
        return st;
    return s3paper::OkStatus();
}

}  // namespace

void DisplayServiceInit() {
    if (s_builder != nullptr) {
        return;
    }
    s_ops = static_cast<s3paper::DrawOp *>(heap_caps_malloc(
        kOpCapacity * sizeof(s3paper::DrawOp), MALLOC_CAP_SPIRAM));
    s_arena_buf = static_cast<uint8_t *>(
        heap_caps_malloc(kArenaCapacity, MALLOC_CAP_SPIRAM));
    s_trace_buf = static_cast<char *>(
        heap_caps_malloc(kTraceCapacity, MALLOC_CAP_SPIRAM));
    if (s_ops == nullptr || s_arena_buf == nullptr || s_trace_buf == nullptr) {
        ESP_LOGE(kTag, "PSRAM allocation for frame storage failed");
        abort();
    }
    static s3paper::FrameArena arena(s_arena_buf, kArenaCapacity);
    static s3paper::FrameBuilder builder(s_ops, kOpCapacity, &arena,
                                         kViewport);
    static s3paper::FakeBackend fake(s_trace_buf, kTraceCapacity,
                                     s3paper::Size{540, 960});
    static s3paper::M5Backend m5;
    s_arena = &arena;
    s_builder = &builder;
    s_fake = &fake;
    s_m5 = &m5;
    (void)s_fake->Init();
    ESP_LOGI(kTag,
             "frame storage ready: ops=%u arena=%uB trace=%uB (PSRAM)",
             static_cast<unsigned>(kOpCapacity),
             static_cast<unsigned>(kArenaCapacity),
             static_cast<unsigned>(kTraceCapacity));
}

s3paper::PresentResult RunFixture(bool use_m5) {
    s3paper::PresentResult result{};
    if (s_builder == nullptr) {
        result.status = s3paper::StatusCode::Busy;
        return result;
    }
    const s3paper::Status built = BuildFixture(*s_builder);
    if (!built.ok()) {
        result.status = built.code;
        return result;
    }
    const s3paper::Result<s3paper::RenderFrame> frame =
        s_builder->Finish(s_next_frame_id++);
    if (!frame.ok()) {
        result.status = frame.code;
        return result;
    }
    if (use_m5) {
        const s3paper::Status init = s_m5->Init();
        if (!init.ok()) {
            result.status = init.code;
            return result;
        }
        return s_m5->Present(frame.value, s3paper::PresentIntent::CleanFull);
    }
    s_fake->ClearTrace();
    return s_fake->Present(frame.value, s3paper::PresentIntent::CleanFull);
}

void PrintFakeTrace() {
    if (s_fake == nullptr) {
        return;
    }
    printf("%s", s_fake->trace());
    if (s_fake->trace_truncated()) {
        printf("(trace truncated at %u bytes)\n",
               static_cast<unsigned>(s_fake->trace_len()));
    }
}

s3paper::BackendState FakeBackendState() {
    return s_fake ? s_fake->GetState() : s3paper::BackendState{};
}

s3paper::BackendState M5BackendState() {
    return s_m5 ? s_m5->GetState() : s3paper::BackendState{};
}

}  // namespace reader
