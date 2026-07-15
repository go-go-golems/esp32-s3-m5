#include "s3paper_m5/m5_backend.h"

#include <M5Unified.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "s3paper/frame_arena.h"

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
            case DrawOpKind::GlyphRun: {
                const GlyphRunPayload &g = op.payload.glyph_run;
                if (frame.arena == nullptr) {
                    result.ops_skipped++;
                    break;
                }
                // Phase 2: builtin font scaled to size_px; the Phase 5 text
                // pipeline replaces this with measured glyph rendering.
                const char *text = reinterpret_cast<const char *>(
                    frame.arena->Data(g.text_offset));
                M5.Display.setTextColor(GrayToColor(op.gray));
                M5.Display.setTextSize(
                    g.size_px > 8 ? static_cast<float>(g.size_px) / 8.0f
                                  : 1.0f);
                M5.Display.setCursor(op.bounds.x, op.bounds.y);
                M5.Display.printf("%.*s", static_cast<int>(g.text_len), text);
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
