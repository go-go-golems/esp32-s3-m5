#include "app_display.h"

#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "s3paper/fake_backend.h"
#include "s3paper/frame_arena.h"
#include "s3paper/frame_builder.h"
#include "s3paper/text.h"
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
s3paper::RefreshPlanner *s_planner = nullptr;
s3paper::FrameId s_next_frame_id = 1;

// Soak policy: a longer turn budget than the default so a 10k-step soak is
// dominated by partial updates with periodic planner-driven clean fulls.
s3paper::RefreshPolicy MakePlannerPolicy() {
    s3paper::RefreshPolicy policy;
    policy.max_turns_between_full = 64;
    return policy;
}

// Presents a frozen frame through the planner on the chosen backend and
// commits the result to refresh history.
PlannedPresent PresentPlanned(const s3paper::RenderFrame &frame,
                              s3paper::PresentIntent intent, bool use_m5) {
    PlannedPresent out{};
    s3paper::Status st = s_planner->AddDamage(frame.damage);
    if (!st.ok()) {
        out.present.status = st.code;
        return out;
    }
    const int64_t now_us = esp_timer_get_time();
    out.plan = s_planner->Plan(intent, now_us);
    // The backend still consumes per-op clip rects; the plan chooses the
    // effective intent (a planner-forced full becomes CleanFull).
    const s3paper::PresentIntent effective =
        out.plan.full_refresh ? s3paper::PresentIntent::CleanFull : intent;
    if (use_m5) {
        out.present = s_m5->Present(frame, effective);
    } else {
        s_fake->ClearTrace();
        out.present = s_fake->Present(frame, effective);
    }
    if (out.present.status == s3paper::StatusCode::Ok) {
        s_planner->RecordPresent(out.plan, esp_timer_get_time());
    } else {
        s_planner->ClearDamage();
    }
    return out;
}

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

// Embedded PT Serif subset (Latin + Ukrainian), registered for both font
// ids. Pixel sizes chosen so the body line height (~42 px) matches the
// previous bitmap font's rhythm. See design-doc/04 for the decision record.
extern "C" const uint8_t _binary_PTSerifUkr_ttf_start[];
extern "C" const uint8_t _binary_PTSerifUkr_ttf_end[];

// Liberation Sans Bold subset: the PULP OS display faces (Swiss chrome).
extern "C" const uint8_t _binary_LibSansBoldUkr_ttf_start[];
extern "C" const uint8_t _binary_LibSansBoldUkr_ttf_end[];

void DisplayServiceInit() {
    if (s_builder != nullptr) {
        return;
    }
    const uint32_t font_size = static_cast<uint32_t>(
        _binary_PTSerifUkr_ttf_end - _binary_PTSerifUkr_ttf_start);
    const s3paper::Status ui_font = s3paper::RegisterTtfFont(
        s3paper::kFontUi, _binary_PTSerifUkr_ttf_start, font_size, 22);
    const s3paper::Status body_font = s3paper::RegisterTtfFont(
        s3paper::kFontBody, _binary_PTSerifUkr_ttf_start, font_size, 34);
    const uint32_t sans_size = static_cast<uint32_t>(
        _binary_LibSansBoldUkr_ttf_end - _binary_LibSansBoldUkr_ttf_start);
    const s3paper::Status display_font = s3paper::RegisterTtfFont(
        s3paper::kFontDisplay, _binary_LibSansBoldUkr_ttf_start, sans_size,
        44);
    const s3paper::Status xl_font = s3paper::RegisterTtfFont(
        s3paper::kFontXL, _binary_LibSansBoldUkr_ttf_start, sans_size, 84);
    if (!display_font.ok() || !xl_font.ok()) {
        ESP_LOGE(kTag, "display font registration failed (%s/%s)",
                 s3paper::StatusCodeName(display_font.code),
                 s3paper::StatusCodeName(xl_font.code));
    }
    if (!ui_font.ok() || !body_font.ok()) {
        ESP_LOGE(kTag, "TTF font registration failed (%s/%s); GFX fallback",
                 s3paper::StatusCodeName(ui_font.code),
                 s3paper::StatusCodeName(body_font.code));
    } else {
        const s3paper::FontLineMetrics body =
            s3paper::GetFontLineMetrics(s3paper::kFontBody).value;
        ESP_LOGI(kTag,
                 "PT Serif registered: %u bytes, body ascent=%d descent=%d "
                 "line_height=%d",
                 static_cast<unsigned>(font_size),
                 static_cast<int>(body.ascent),
                 static_cast<int>(body.descent),
                 static_cast<int>(body.line_height));
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
    static s3paper::RefreshPlanner planner(kViewport, MakePlannerPolicy());
    s_arena = &arena;
    s_builder = &builder;
    s_fake = &fake;
    s_m5 = &m5;
    s_planner = &planner;
    (void)s_fake->Init();
    ESP_LOGI(kTag,
             "frame storage ready: ops=%u arena=%uB trace=%uB (PSRAM)",
             static_cast<unsigned>(kOpCapacity),
             static_cast<unsigned>(kArenaCapacity),
             static_cast<unsigned>(kTraceCapacity));
}

PlannedPresent RunFixture(bool use_m5) {
    PlannedPresent out{};
    if (s_builder == nullptr) {
        out.present.status = s3paper::StatusCode::Busy;
        return out;
    }
    const s3paper::Status built = BuildFixture(*s_builder);
    if (!built.ok()) {
        out.present.status = built.code;
        return out;
    }
    const s3paper::Result<s3paper::RenderFrame> frame =
        s_builder->Finish(s_next_frame_id++);
    if (!frame.ok()) {
        out.present.status = frame.code;
        return out;
    }
    if (use_m5) {
        const s3paper::Status init = s_m5->Init();
        if (!init.ok()) {
            out.present.status = init.code;
            return out;
        }
    }
    return PresentPlanned(frame.value, s3paper::PresentIntent::CleanFull,
                          use_m5);
}

// Canned body text for the Phase 5 typography fixture: English paragraph,
// the Ukrainian pangram + apostrophe cases (ticket 3r0u acceptance), an
// accented-Latin line, and a CJK codepoint to prove the fallback box.
constexpr const char kFixtureTitle[] = "Typography fixture / \xD0\xA8\xD1\x80\xD0\xB8\xD1\x84\xD1\x82";
constexpr const char kFixtureBody[] =
    "Alice was beginning to get very tired of sitting by her sister on the "
    "bank, and of having nothing to do: once or twice she had peeped into "
    "the book her sister was reading, but it had no pictures or "
    "conversations in it. AVATAR Yield Tempo \xE2\x80\x94 kerning pairs.\n"
    "\xD0\xA7\xD1\x83\xD1\x94\xD1\x88 \xD1\x97\xD1\x85, \xD0\xB4\xD0\xBE"
    "\xD1\x86\xD1\x8E, \xD0\xB3\xD0\xB0? \xD0\x9A\xD1\x83\xD0\xBC\xD0\xB5"
    "\xD0\xB4\xD0\xBD\xD0\xB0 \xD0\xB6 \xD1\x82\xD0\xB8, \xD0\xBF\xD1\x80"
    "\xD0\xBE\xD1\x89\xD0\xB0\xD0\xB9\xD1\x81\xD1\x8F \xD0\xB1\xD0\xB5"
    "\xD0\xB7 \xD2\x91\xD0\xBE\xD0\xBB\xD1\x8C\xD1\x84\xD1\x96\xD0\xB2!\n"
    "\xD0\x9F'\xD1\x8F\xD1\x82\xD1\x8C \xE2\x80\x99 \xD0\xBC\xE2\x80\x99"
    "\xD1\x8F\xD1\x87, \xD0\x9A\xD0\xB8\xD1\x97\xD0\xB2, \xD0\x84\xD0\xB2"
    "\xD1\x80\xD0\xBE\xD0\xBF\xD0\xB0, \xD2\x90\xD0\xB0\xD0\xBD\xD0\xBE"
    "\xD0\xBA \xE2\x80\x94 \xC2\xAB\xD0\xBB\xD0\xB0\xD0\xBF\xD0\xBA\xD0\xB8"
    "\xC2\xBB.\n"
    "Latin accents: caf\xC3\xA9 na\xC3\xAFve \xC3\xA9tude; fallback box: "
    "\xE4\xB8\x80.";

s3paper::Status BuildTextPage(s3paper::FrameBuilder &fb) {
    using s3paper::Rect;
    constexpr int32_t kMarginX = 40;
    constexpr int32_t kMarginTop = 60;
    constexpr int32_t kTextWidth = 540 - 2 * kMarginX;
    fb.Begin();
    s3paper::Status st = fb.FillRect(Rect{0, 0, 540, 960}, 255);
    if (!st.ok()) return st;

    const s3paper::FontLineMetrics ui =
        s3paper::GetFontLineMetrics(s3paper::kFontUi).value;
    const s3paper::FontLineMetrics body =
        s3paper::GetFontLineMetrics(s3paper::kFontBody).value;
    int32_t baseline = kMarginTop + ui.line_height;
    // Title in the UI font.
    st = fb.GlyphRun(Rect{kMarginX, baseline - ui.ascent, kTextWidth,
                          ui.line_height + 8},
                     baseline, s3paper::kFontUi, 0, kFixtureTitle,
                     sizeof(kFixtureTitle) - 1, 0);
    if (!st.ok()) return st;
    st = fb.HLine(kMarginX, baseline + 10, kTextWidth, 0);
    if (!st.ok()) return st;
    baseline += ui.line_height + body.line_height;

    // Body: paragraphs -> measured lines -> one GlyphRun per line.
    s3paper::TextSpan paras[16];
    const uint32_t para_count = s3paper::SplitParagraphs(
        kFixtureBody, sizeof(kFixtureBody) - 1, paras, 16);
    for (uint32_t p = 0; p < para_count && p < 16; ++p) {
        s3paper::LineSpan lines[64];
        const s3paper::Result<uint32_t> n = s3paper::BreakLines(
            s3paper::kFontBody, kFixtureBody + paras[p].byte_start,
            paras[p].byte_len, kTextWidth, lines, 64);
        if (!n.ok()) return s3paper::ErrStatus(n.code);
        for (uint32_t i = 0; i < n.value; ++i) {
            if (baseline > 940) {
                return s3paper::OkStatus();  // page full
            }
            st = fb.GlyphRun(
                Rect{kMarginX, baseline - body.ascent, lines[i].width,
                     body.line_height},
                baseline, s3paper::kFontBody, 0,
                kFixtureBody + paras[p].byte_start + lines[i].byte_start,
                lines[i].byte_len, 0);
            if (!st.ok()) return st;
            baseline += body.line_height;
        }
        baseline += body.line_height / 2;  // paragraph spacing
    }
    return s3paper::OkStatus();
}

PlannedPresent RunTextFixture(bool use_m5) {
    PlannedPresent out{};
    if (s_builder == nullptr) {
        out.present.status = s3paper::StatusCode::Busy;
        return out;
    }
    const s3paper::Status built = BuildTextPage(*s_builder);
    if (!built.ok()) {
        out.present.status = built.code;
        return out;
    }
    const s3paper::Result<s3paper::RenderFrame> frame =
        s_builder->Finish(s_next_frame_id++);
    if (!frame.ok()) {
        out.present.status = frame.code;
        return out;
    }
    if (use_m5) {
        const s3paper::Status init = s_m5->Init();
        if (!init.ok()) {
            out.present.status = init.code;
            return out;
        }
    }
    return PresentPlanned(frame.value, s3paper::PresentIntent::TextPage,
                          use_m5);
}

PlannedPresent RunSoakStep(uint32_t step_index) {
    PlannedPresent out{};
    if (s_builder == nullptr) {
        out.present.status = s3paper::StatusCode::Busy;
        return out;
    }
    const s3paper::Status init = s_m5->Init();
    if (!init.ok()) {
        out.present.status = init.code;
        return out;
    }
    using s3paper::Rect;
    // Deterministic pseudo-scatter: primes keep successive steps apart so
    // damage rects rarely merge and the panel sees varied regions.
    const int32_t x =
        static_cast<int32_t>((step_index * 97) % 440);
    const int32_t y = static_cast<int32_t>((step_index * 211) % 860);
    const bool black = (step_index & 1) == 0;
    const s3paper::Gray8 gray = black ? 0 : 255;
    s3paper::FrameBuilder &fb = *s_builder;
    fb.Begin();
    s3paper::Status st = fb.FillRect(Rect{x, y, 64, 48}, gray);
    if (st.ok() && step_index % 8 == 0) {
        // Periodic text-shaped update.
        static const char kSoakText[] = "soak";
        st = fb.GlyphRun(Rect{x, y, 64, 24}, y + 20, 0, 16, kSoakText,
                         sizeof(kSoakText) - 1, black ? 255 : 0);
    }
    if (!st.ok()) {
        out.present.status = st.code;
        return out;
    }
    const s3paper::Result<s3paper::RenderFrame> frame =
        s_builder->Finish(s_next_frame_id++);
    if (!frame.ok()) {
        out.present.status = frame.code;
        return out;
    }
    // Cycle intents so every waveform class appears in the soak.
    s3paper::PresentIntent intent;
    switch (step_index % 4) {
        case 0: intent = s3paper::PresentIntent::InteractiveInk; break;
        case 1: intent = s3paper::PresentIntent::TextRegion; break;
        case 2: intent = s3paper::PresentIntent::TextPage; break;
        default:
            intent = (step_index % 64 == 3)
                         ? s3paper::PresentIntent::ImageQuality
                         : s3paper::PresentIntent::TextRegion;
            break;
    }
    return PresentPlanned(frame.value, intent, true);
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

const char *FakeTrace() {
    return s_fake != nullptr ? s_fake->trace() : "";
}

s3paper::BackendState FakeBackendState() {
    return s_fake ? s_fake->GetState() : s3paper::BackendState{};
}

s3paper::BackendState M5BackendState() {
    return s_m5 ? s_m5->GetState() : s3paper::BackendState{};
}

s3paper::RefreshPlanner &Planner() { return *s_planner; }

s3paper::FrameBuilder &FrameBuilderRef() { return *s_builder; }

s3paper::Result<s3paper::RenderFrame> FinishFrame() {
    return s_builder->Finish(s_next_frame_id++);
}

PlannedPresent PresentFramePlanned(const s3paper::RenderFrame &frame,
                                   s3paper::PresentIntent intent,
                                   bool use_m5) {
    return PresentPlanned(frame, intent, use_m5);
}

s3paper::Status EnsureM5Init() {
    if (s_m5 == nullptr) {
        return s3paper::ErrStatus(s3paper::StatusCode::Busy);
    }
    return s_m5->Init();
}

bool ReadM5Touch(s3paper::PointerSample *out) {
    return s_m5 != nullptr && s_m5->ReadTouch(out);
}

}  // namespace reader
