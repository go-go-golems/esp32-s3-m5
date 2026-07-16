#include "app_display.h"

#include <initializer_list>

#include "s3paper/text.h"

namespace reader {
namespace {

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

}  // namespace

void DisplayServiceInit() {
    s3paper_runtime::RuntimeInit(s3paper_runtime::RuntimeConfig{});
}

PlannedPresent RunFixture(bool use_m5) {
    PlannedPresent out{};
    const s3paper::Status built = BuildFixture(FrameBuilderRef());
    if (!built.ok()) {
        out.present.status = built.code;
        return out;
    }
    const s3paper::Result<s3paper::RenderFrame> frame = FinishFrame();
    if (!frame.ok()) {
        out.present.status = frame.code;
        return out;
    }
    if (use_m5) {
        const s3paper::Status init = EnsureM5Init();
        if (!init.ok()) {
            out.present.status = init.code;
            return out;
        }
    }
    return PresentFramePlanned(frame.value,
                               s3paper::PresentIntent::CleanFull, use_m5);
}

PlannedPresent RunTextFixture(bool use_m5) {
    PlannedPresent out{};
    const s3paper::Status built = BuildTextPage(FrameBuilderRef());
    if (!built.ok()) {
        out.present.status = built.code;
        return out;
    }
    const s3paper::Result<s3paper::RenderFrame> frame = FinishFrame();
    if (!frame.ok()) {
        out.present.status = frame.code;
        return out;
    }
    if (use_m5) {
        const s3paper::Status init = EnsureM5Init();
        if (!init.ok()) {
            out.present.status = init.code;
            return out;
        }
    }
    return PresentFramePlanned(frame.value,
                               s3paper::PresentIntent::TextPage, use_m5);
}

PlannedPresent RunSoakStep(uint32_t step_index) {
    PlannedPresent out{};
    const s3paper::Status init = EnsureM5Init();
    if (!init.ok()) {
        out.present.status = init.code;
        return out;
    }
    using s3paper::Rect;
    // Deterministic pseudo-scatter: primes keep successive steps apart so
    // damage rects rarely merge and the panel sees varied regions.
    const int32_t x = static_cast<int32_t>((step_index * 97) % 440);
    const int32_t y = static_cast<int32_t>((step_index * 211) % 860);
    const bool black = (step_index & 1) == 0;
    const s3paper::Gray8 gray = black ? 0 : 255;
    s3paper::FrameBuilder &fb = FrameBuilderRef();
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
    const s3paper::Result<s3paper::RenderFrame> frame = FinishFrame();
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
    return PresentFramePlanned(frame.value, intent, true);
}

}  // namespace reader
