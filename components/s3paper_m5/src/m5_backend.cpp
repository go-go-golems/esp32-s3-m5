#include "s3paper_m5/m5_backend.h"

#include <M5Unified.h>

#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "s3paper/frame_arena.h"
#include "s3paper/text.h"

namespace s3paper {
namespace {

const char *kTag = "m5_backend";

// Bounded panel-busy wait so a wedged EPD cannot deadlock the owner task.
constexpr int64_t kBusyTimeoutUs = 5'000'000;

// Returns false on timeout.
bool WaitNotBusy() {
    const int64_t start = esp_timer_get_time();
    while (M5.Display.displayBusy()) {
        if (esp_timer_get_time() - start > kBusyTimeoutUs) {
            return false;
        }
        vTaskDelay(1);
    }
    return true;
}

epd_mode_t ModeForIntent(PresentIntent intent) {
    // Naive Phase 2 mapping; the Phase 3 refresh planner owns this later.
    switch (intent) {
        case PresentIntent::InteractiveInk: return epd_mode_t::epd_fastest;
        case PresentIntent::TextRegion: return epd_mode_t::epd_fast;
        case PresentIntent::TextPage: return epd_mode_t::epd_text;
        case PresentIntent::ImageQuality: return epd_mode_t::epd_quality;
        case PresentIntent::CleanFull: return epd_mode_t::epd_quality;
    }
    return epd_mode_t::epd_quality;
}

uint32_t GrayToColor(Gray8 gray) {
    return M5.Display.color888(gray, gray, gray);
}

// ---- TTF glyph cache (PSRAM) ----
//
// Rasterized 8-bit coverage bitmaps keyed by (font_id, codepoint); pixel
// size is fixed per font id. When either the slot table or the byte pool
// fills, the whole cache resets (explicit, counted) rather than evicting
// silently.
constexpr uint32_t kGlyphCacheSlots = 512;
constexpr uint32_t kGlyphCacheBytes = 256 * 1024;

struct GlyphCacheEntry {
    bool valid;
    uint8_t font_id;
    uint32_t codepoint;
    GlyphRaster raster;
    uint32_t offset;
};

GlyphCacheEntry *s_glyph_slots = nullptr;
uint8_t *s_glyph_pool = nullptr;
uint32_t s_glyph_pool_used = 0;
uint32_t s_glyph_cache_resets = 0;

void GlyphCacheInit() {
    if (s_glyph_slots != nullptr) {
        return;
    }
    s_glyph_slots = static_cast<GlyphCacheEntry *>(heap_caps_calloc(
        kGlyphCacheSlots, sizeof(GlyphCacheEntry), MALLOC_CAP_SPIRAM));
    s_glyph_pool = static_cast<uint8_t *>(
        heap_caps_malloc(kGlyphCacheBytes, MALLOC_CAP_SPIRAM));
    if (s_glyph_slots == nullptr || s_glyph_pool == nullptr) {
        ESP_LOGE(kTag, "glyph cache PSRAM allocation failed");
        abort();
    }
}

void GlyphCacheReset() {
    std::memset(s_glyph_slots, 0,
                kGlyphCacheSlots * sizeof(GlyphCacheEntry));
    s_glyph_pool_used = 0;
    s_glyph_cache_resets++;
    ESP_LOGW(kTag, "glyph cache reset (%u total)",
             static_cast<unsigned>(s_glyph_cache_resets));
}

// Returns the cached (or freshly rasterized) coverage bitmap, or nullptr
// when the glyph has no raster (missing from font).
const uint8_t *GlyphCacheGet(uint8_t font_id, uint32_t codepoint,
                             GlyphRaster *out_raster) {
    const uint32_t hash =
        (codepoint * 2654435761u + font_id) % kGlyphCacheSlots;
    for (uint32_t probe = 0; probe < kGlyphCacheSlots; ++probe) {
        GlyphCacheEntry &slot =
            s_glyph_slots[(hash + probe) % kGlyphCacheSlots];
        if (slot.valid && slot.font_id == font_id &&
            slot.codepoint == codepoint) {
            *out_raster = slot.raster;
            return s_glyph_pool + slot.offset;
        }
        if (!slot.valid) {
            // Miss: rasterize into the pool at this slot.
            const uint32_t remaining = kGlyphCacheBytes - s_glyph_pool_used;
            const Result<GlyphRaster> raster = RasterizeGlyph(
                font_id, codepoint, s_glyph_pool + s_glyph_pool_used,
                remaining);
            if (!raster.ok()) {
                if (raster.code == StatusCode::CapacityExceeded) {
                    GlyphCacheReset();
                    return GlyphCacheGet(font_id, codepoint, out_raster);
                }
                return nullptr;  // glyph not in font
            }
            slot.valid = true;
            slot.font_id = font_id;
            slot.codepoint = codepoint;
            slot.raster = raster.value;
            slot.offset = s_glyph_pool_used;
            s_glyph_pool_used += static_cast<uint32_t>(raster.value.width) *
                                 raster.value.height;
            *out_raster = slot.raster;
            return s_glyph_pool + slot.offset;
        }
    }
    // Slot table full of other glyphs: reset and retry once.
    GlyphCacheReset();
    return GlyphCacheGet(font_id, codepoint, out_raster);
}

// Blits an 8-bit coverage bitmap as horizontal runs of equal quantized
// gray. AA mode quantizes coverage to the panel's 16 levels; 1-bit mode
// thresholds at 50% (AA fringes are unpredictable in fast waveforms).
void BlitCoverage(const uint8_t *coverage, const GlyphRaster &raster,
                  int32_t pen_x, int32_t baseline_y, Gray8 text_gray,
                  bool antialias) {
    const int32_t x0 = pen_x + raster.x_offset;
    const int32_t y0 = baseline_y + raster.y_offset;
    for (int32_t row = 0; row < raster.height; ++row) {
        const uint8_t *src = coverage + row * raster.width;
        int32_t run_start = -1;
        uint8_t run_value = 0;
        for (int32_t col = 0; col <= raster.width; ++col) {
            uint8_t v = 0;
            if (col < raster.width) {
                uint8_t c = src[col];
                if (!antialias) {
                    c = c >= 128 ? 255 : 0;
                } else {
                    c = static_cast<uint8_t>((c >> 4) * 17);  // 16 levels
                }
                // Composite text_gray over an assumed white background.
                v = static_cast<uint8_t>(255 -
                                         ((255 - text_gray) * c) / 255);
            } else {
                v = 255;  // sentinel: flush at end of row
            }
            if (run_start >= 0 && v != run_value) {
                M5.Display.writeFastHLine(x0 + run_start, y0 + row,
                                          col - run_start,
                                          GrayToColor(run_value));
                run_start = -1;
            }
            if (col < raster.width && v != 255 && run_start < 0) {
                run_start = col;
                run_value = v;
            }
        }
    }
}

// Renders a glyph run from a registered TTF font with kerning.
void RenderTtfGlyphRun(const DrawOp &op, const FrameArena *arena,
                       bool antialias) {
    const GlyphRunPayload &g = op.payload.glyph_run;
    const char *text =
        reinterpret_cast<const char *>(arena->Data(g.text_offset));
    int32_t pen_x = op.bounds.x;
    uint32_t pos = 0;
    uint32_t cp = 0;
    uint32_t prev_cp = 0;
    while (Utf8Next(text, g.text_len, &pos, &cp)) {
        const Result<GlyphMetrics> m = GetGlyphMetrics(g.font_id, cp);
        if (!m.ok()) {
            continue;
        }
        if (prev_cp != 0) {
            pen_x += GetKernAdvance(g.font_id, prev_cp, cp);
        }
        if (m.value.fallback) {
            M5.Display.drawRect(pen_x + m.value.x_offset,
                                g.baseline_y + m.value.y_offset,
                                m.value.width, m.value.height,
                                GrayToColor(op.gray));
        } else if (cp != ' ') {
            GlyphRaster raster;
            const uint8_t *coverage =
                GlyphCacheGet(g.font_id, cp, &raster);
            if (coverage != nullptr && raster.width > 0) {
                BlitCoverage(coverage, raster, pen_x, g.baseline_y, op.gray,
                             antialias);
            }
        }
        pen_x += m.value.advance;
        prev_cp = cp;
    }
}

// Blits one glyph from the vendored GFX bitmap data as horizontal runs of
// set bits. Same data source as host-side measurement: one metrics truth.
void BlitGlyph(const GfxFont &font, const GfxGlyph &glyph, int32_t pen_x,
               int32_t baseline_y, uint32_t color) {
    const uint8_t *bits = font.bitmaps + glyph.bitmap_offset;
    uint32_t bit = 0;
    const int32_t x0 = pen_x + glyph.x_offset;
    const int32_t y0 = baseline_y + glyph.y_offset;
    for (int32_t row = 0; row < glyph.height; ++row) {
        int32_t run_start = -1;
        for (int32_t col = 0; col < glyph.width; ++col, ++bit) {
            const bool set = (bits[bit >> 3] >> (7 - (bit & 7))) & 1;
            if (set && run_start < 0) {
                run_start = col;
            } else if (!set && run_start >= 0) {
                M5.Display.writeFastHLine(x0 + run_start, y0 + row,
                                          col - run_start, color);
                run_start = -1;
            }
        }
        if (run_start >= 0) {
            M5.Display.writeFastHLine(x0 + run_start, y0 + row,
                                      glyph.width - run_start, color);
        }
    }
}

// Renders a glyph run with the s3paper font data; unknown codepoints get
// the deterministic fallback box.
void RenderGlyphRun(const DrawOp &op, const FrameArena *arena) {
    const GlyphRunPayload &g = op.payload.glyph_run;
    const GfxFont *font = GetFont(g.font_id);
    if (font == nullptr || arena == nullptr) {
        return;
    }
    const char *text =
        reinterpret_cast<const char *>(arena->Data(g.text_offset));
    const uint32_t color = GrayToColor(op.gray);
    int32_t pen_x = op.bounds.x;
    uint32_t pos = 0;
    uint32_t cp = 0;
    while (Utf8Next(text, g.text_len, &pos, &cp)) {
        const Result<GlyphMetrics> m = GetGlyphMetrics(g.font_id, cp);
        if (!m.ok()) {
            continue;
        }
        if (m.value.fallback) {
            M5.Display.drawRect(pen_x + m.value.x_offset,
                                g.baseline_y + m.value.y_offset,
                                m.value.width, m.value.height, color);
        } else if (cp != ' ') {
            BlitGlyph(*font, font->glyphs[cp - font->first], pen_x,
                      g.baseline_y, color);
        }
        pen_x += m.value.advance;
    }
}

}  // namespace

Status M5Backend::Init() {
    if (initialized_) {
        return OkStatus();
    }
    auto cfg = M5.config();
    M5.begin(cfg);
    if (M5.Display.width() <= 0 || M5.Display.height() <= 0) {
        ESP_LOGE(kTag, "no display detected");
        return ErrStatus(StatusCode::CorruptData);
    }
    physical_size_ = Size{M5.Display.width(), M5.Display.height()};
    GlyphCacheInit();
    initialized_ = true;
    ESP_LOGI(kTag, "init board=%d display=%dx%d",
             static_cast<int>(M5.getBoard()),
             static_cast<int>(physical_size_.w),
             static_cast<int>(physical_size_.h));
    return OkStatus();
}

PresentResult M5Backend::Present(const RenderFrame &frame,
                                 PresentIntent intent) {
    PresentResult result{};
    result.frame_id = frame.id;
    if (!initialized_) {
        result.status = StatusCode::Busy;
        return result;
    }
    if (!WaitNotBusy()) {
        result.status = StatusCode::Timeout;
        return result;
    }
    const int64_t render_start = esp_timer_get_time();
    M5.Display.setEpdMode(ModeForIntent(intent));
    M5.Display.startWrite();
    for (uint32_t i = 0; i < frame.op_count; ++i) {
        const DrawOp &op = frame.ops[i];
        M5.Display.setClipRect(op.clip.x, op.clip.y, op.clip.w, op.clip.h);
        switch (op.kind) {
            case DrawOpKind::FillRect:
                M5.Display.fillRect(op.bounds.x, op.bounds.y, op.bounds.w,
                                    op.bounds.h, GrayToColor(op.gray));
                result.ops_drawn++;
                break;
            case DrawOpKind::StrokeRect: {
                const int32_t t = op.payload.stroke.thickness;
                for (int32_t s = 0; s < t; ++s) {
                    M5.Display.drawRect(op.bounds.x + s, op.bounds.y + s,
                                        op.bounds.w - 2 * s,
                                        op.bounds.h - 2 * s,
                                        GrayToColor(op.gray));
                }
                result.ops_drawn++;
                break;
            }
            case DrawOpKind::HLine:
                M5.Display.writeFastHLine(op.bounds.x, op.bounds.y,
                                          op.bounds.w, GrayToColor(op.gray));
                result.ops_drawn++;
                break;
            case DrawOpKind::VLine:
                M5.Display.writeFastVLine(op.bounds.x, op.bounds.y,
                                          op.bounds.h, GrayToColor(op.gray));
                result.ops_drawn++;
                break;
            case DrawOpKind::GlyphRun:
                // A font is renderable when a TTF is registered for the id
                // OR a bitmap fallback exists (GetFont only knows ids 0/1;
                // TTF-only ids like the display faces must not be skipped).
                if (frame.arena == nullptr ||
                    (!IsTtfFont(op.payload.glyph_run.font_id) &&
                     GetFont(op.payload.glyph_run.font_id) == nullptr)) {
                    result.ops_skipped++;
                    break;
                }
                if (IsTtfFont(op.payload.glyph_run.font_id)) {
                    // AA only in grayscale-capable waveform intents; fast
                    // 1-bit waveforms threshold AA fringes unpredictably.
                    const bool antialias =
                        intent == PresentIntent::TextPage ||
                        intent == PresentIntent::ImageQuality ||
                        intent == PresentIntent::CleanFull;
                    RenderTtfGlyphRun(op, frame.arena, antialias);
                } else {
                    RenderGlyphRun(op, frame.arena);
                }
                result.ops_drawn++;
                break;
            case DrawOpKind::Line: {
                // The panel clip (set above from op.clip) confines the
                // rasterization; endpoints are true geometry. Thickness
                // is emulated by parallel lines offset along the minor
                // axis (M5GFX has no thick-line primitive).
                const LinePayload &l = op.payload.line;
                const int32_t dx = l.x1 - l.x0;
                const int32_t dy = l.y1 - l.y0;
                const bool steep = (dy < 0 ? -dy : dy) > (dx < 0 ? -dx : dx);
                for (int32_t s = 0; s < l.thickness; ++s) {
                    const int32_t off = s - l.thickness / 2;
                    if (steep) {
                        M5.Display.drawLine(l.x0 + off, l.y0, l.x1 + off,
                                            l.y1, GrayToColor(op.gray));
                    } else {
                        M5.Display.drawLine(l.x0, l.y0 + off, l.x1,
                                            l.y1 + off, GrayToColor(op.gray));
                    }
                }
                result.ops_drawn++;
                break;
            }
            case DrawOpKind::Circle: {
                const CirclePayload &c = op.payload.circle;
                if (c.thickness == 0) {
                    M5.Display.fillCircle(c.cx, c.cy, c.r,
                                          GrayToColor(op.gray));
                } else {
                    // Ring: concentric outlines stepping inward. Never
                    // paint the interior (it would erase what is behind).
                    for (int32_t s = 0; s < c.thickness; ++s) {
                        M5.Display.drawCircle(c.cx, c.cy, c.r - s,
                                              GrayToColor(op.gray));
                    }
                }
                result.ops_drawn++;
                break;
            }
            case DrawOpKind::Bitmap:
                // Explicitly unsupported in Phase 2.
                result.ops_skipped++;
                break;
        }
    }
    M5.Display.clearClipRect();
    M5.Display.endWrite();
    const int64_t flush_start = esp_timer_get_time();
    result.render_us = static_cast<uint32_t>(flush_start - render_start);
    if (!WaitNotBusy()) {
        result.status = StatusCode::Timeout;
        result.wait_us =
            static_cast<uint32_t>(esp_timer_get_time() - flush_start);
        return result;
    }
    result.wait_us = static_cast<uint32_t>(esp_timer_get_time() - flush_start);
    result.damage = frame.damage;
    result.status = StatusCode::Ok;
    frames_presented_++;
    ESP_LOGI(kTag,
             "present id=%u intent=%s ops=%u skipped=%u damage=%d,%d,%d,%d "
             "render_us=%u wait_us=%u",
             static_cast<unsigned>(frame.id), PresentIntentName(intent),
             static_cast<unsigned>(result.ops_drawn),
             static_cast<unsigned>(result.ops_skipped),
             static_cast<int>(frame.damage.x), static_cast<int>(frame.damage.y),
             static_cast<int>(frame.damage.w), static_cast<int>(frame.damage.h),
             static_cast<unsigned>(result.render_us),
             static_cast<unsigned>(result.wait_us));
    return result;
}

BackendState M5Backend::GetState() const {
    return BackendState{initialized_, physical_size_, frames_presented_};
}

bool M5Backend::ReadTouch(PointerSample *out) {
    if (!initialized_) {
        return false;
    }
    M5.update();
    const auto detail = M5.Touch.getDetail();
    out->touching = detail.isPressed();
    out->pos = Point{detail.x, detail.y};
    out->t_us = esp_timer_get_time();
    return true;
}

}  // namespace s3paper
