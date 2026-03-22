#include "alphabet_app.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace alphabet_graffiti {

namespace {

using protractor_demo::OptimalCosineDistance;
using protractor_demo::PointF;
using protractor_demo::Resample;
using protractor_demo::Vectorize;

constexpr int32_t kScreenMargin = 16;
constexpr int32_t kHeaderHeight = 52;
constexpr int32_t kCanvasInset = 14;
constexpr int32_t kCardRadius = 14;
constexpr int32_t kButtonRadius = 10;
constexpr int32_t kChipRadius = 10;
constexpr int32_t kBrushRadius = 4;
constexpr int32_t kBrushStepPx = 2;
constexpr int32_t kBottomBarHeight = 52;
constexpr int32_t kButtonGap = 8;
constexpr int32_t kSlotCellWidth = 76;
constexpr int32_t kSlotCellHeight = 52;
constexpr int32_t kCellGap = 8;
constexpr std::uint32_t kLoopDelayMs = 12;

// Lain / Navi palette — high contrast for e-ink
constexpr std::uint32_t kColorBg = 0xFFFFFF;
constexpr std::uint32_t kColorFg = 0x000000;
constexpr std::uint32_t kColorDim = 0x666666;
constexpr std::uint32_t kColorGhost = 0xAAAAAA;
constexpr std::uint32_t kColorRecorded = 0x404040;
constexpr int32_t kPanelStripHeight = 24;

void DrawPanel(M5GFX& display, const Rect& rect, const char* title)
{
    display.fillRect(rect.x, rect.y, rect.w, rect.h, kColorBg);
    display.drawRect(rect.x, rect.y, rect.w, rect.h, kColorFg);
    display.fillRect(rect.x + 1, rect.y + 1, rect.w - 2, kPanelStripHeight, kColorFg);
    display.setTextFont(2);
    display.setTextColor(kColorBg, kColorFg);
    display.drawString(title, rect.x + 8, rect.y + 4);
}

void DrawButton(M5GFX& display, const Rect& rect, const char* label, bool enabled, bool active, bool /*selected*/ = false, int font = 2)
{
    const std::uint32_t bg = !enabled ? kColorGhost : (active ? kColorFg : kColorBg);
    const std::uint32_t fg = !enabled ? kColorDim : (active ? kColorBg : kColorFg);

    display.fillRect(rect.x, rect.y, rect.w, rect.h, bg);
    display.drawRect(rect.x, rect.y, rect.w, rect.h, kColorFg);

    display.setTextDatum(middle_center);
    display.setTextColor(fg, bg);
    display.setTextFont(font);
    display.drawString(label, rect.x + rect.w / 2, rect.y + rect.h / 2);
    display.setTextDatum(top_left);
    display.setTextFont(2);
}

void DrawCornerMarks(M5GFX& display, const Rect& rect)
{
    constexpr int32_t m = 10;
    const int32_t l = rect.x + 4, t = rect.y + 4;
    const int32_t r = rect.x + rect.w - 5, b = rect.y + rect.h - 5;
    display.drawFastHLine(l, t, m, kColorDim);
    display.drawFastVLine(l, t, m, kColorDim);
    display.drawFastHLine(r - m + 1, t, m, kColorDim);
    display.drawFastVLine(r, t, m, kColorDim);
    display.drawFastHLine(l, b, m, kColorDim);
    display.drawFastVLine(l, b - m + 1, m, kColorDim);
    display.drawFastHLine(r - m + 1, b, m, kColorDim);
    display.drawFastVLine(r, b - m + 1, m, kColorDim);
}

}  // namespace

void AlphabetApp::Run()
{
    InitBoard();
    BuildLayout();
    LoadTemplatesFromDisk();
    RenderFullUi();

    while (true) {
        M5.update();
        HandleTouch();
        ProcessPendingDisplayWork();
        M5.delay(kLoopDelayMs);
    }
}

void AlphabetApp::InitBoard()
{
    auto cfg = M5.config();
    cfg.clear_display = true;
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorFg, kColorBg);

    GlyphStore::InitializeTemplates(templates_);
}

void AlphabetApp::BuildLayout()
{
    screen_ = {0, 0, M5.Display.width(), M5.Display.height()};

    // Header bar
    header_ = {0, 0, screen_.w, kHeaderHeight};
    train_mode_button_ = {screen_.w - 300, 4, 140, 44};
    write_mode_button_ = {screen_.w - 152, 4, 140, 44};

    const int32_t content_y = kHeaderHeight + 4;
    const int32_t bottom_bar_y = screen_.h - kBottomBarHeight - 4;
    const int32_t content_h = bottom_bar_y - content_y - 4;

    if (mode_ == Mode::train) {
        // Train: canvas left, glyph sidebar right, bottom action bar
        constexpr int32_t sidebar_w = 340;
        const int32_t canvas_card_w = screen_.w - kScreenMargin * 3 - sidebar_w;

        canvas_card_ = {kScreenMargin, content_y, canvas_card_w, content_h};
        canvas_ = {
            canvas_card_.x + kCanvasInset,
            canvas_card_.y + 36,
            canvas_card_.w - kCanvasInset * 2,
            canvas_card_.h - 50,
        };

        palette_card_ = {kScreenMargin * 2 + canvas_card_w, content_y, sidebar_w, content_h};

        // Glyph grid: 4 columns x 3 rows with larger cells
        const int32_t grid_x = palette_card_.x + 12;
        const int32_t grid_y = palette_card_.y + 32;
        for (std::size_t i = 0; i < slot_rects_.size(); ++i) {
            const int32_t row = static_cast<int32_t>(i / 4);
            const int32_t col = static_cast<int32_t>(i % 4);
            slot_rects_[i] = {
                grid_x + col * (kSlotCellWidth + kCellGap),
                grid_y + row * (kSlotCellHeight + kCellGap),
                kSlotCellWidth,
                kSlotCellHeight,
            };
        }

        const int32_t nav_y = grid_y + 3 * (kSlotCellHeight + kCellGap) + 8;
        page_prev_button_ = {palette_card_.x + 12, nav_y, 80, 48};
        page_next_button_ = {palette_card_.x + palette_card_.w - 92, nav_y, 80, 48};

        // Bottom bar: 4 large buttons spanning full width
        const int32_t btn_w = (screen_.w - kScreenMargin * 2 - kButtonGap * 3) / 4;
        save_button_ = {kScreenMargin, bottom_bar_y, btn_w, kBottomBarHeight};
        clear_button_ = {kScreenMargin + btn_w + kButtonGap, bottom_bar_y, btn_w, kBottomBarHeight};
        delete_button_ = {kScreenMargin + (btn_w + kButtonGap) * 2, bottom_bar_y, btn_w, kBottomBarHeight};
        reload_button_ = {kScreenMargin + (btn_w + kButtonGap) * 3, bottom_bar_y, btn_w, kBottomBarHeight};

        text_buffer_bar_ = {0, 0, 0, 0};
    } else {
        // Write: text buffer top, full-width canvas, bottom action bar
        text_buffer_bar_ = {kScreenMargin, content_y, screen_.w - kScreenMargin * 2, 72};

        const int32_t canvas_y = content_y + 72 + kButtonGap;
        const int32_t canvas_h = bottom_bar_y - canvas_y - 4;
        canvas_card_ = {kScreenMargin, canvas_y, screen_.w - kScreenMargin * 2, canvas_h};
        canvas_ = {
            canvas_card_.x + kCanvasInset,
            canvas_card_.y + 36,
            canvas_card_.w - kCanvasInset * 2,
            canvas_card_.h - 50,
        };

        // Bottom bar: 3 large buttons spanning full width
        const int32_t btn_w = (screen_.w - kScreenMargin * 2 - kButtonGap * 2) / 3;
        save_button_ = {kScreenMargin, bottom_bar_y, btn_w, kBottomBarHeight};
        clear_button_ = {kScreenMargin + btn_w + kButtonGap, bottom_bar_y, btn_w, kBottomBarHeight};
        delete_button_ = {kScreenMargin + (btn_w + kButtonGap) * 2, bottom_bar_y, btn_w, kBottomBarHeight};
        reload_button_ = {0, 0, 0, 0};

        palette_card_ = {0, 0, 0, 0};
    }
}

void AlphabetApp::LoadTemplatesFromDisk()
{
    storage_ready_ = store_.Mount();
    storage_status_ = store_.LastStatus();
    if (!storage_ready_) {
        return;
    }

    storage_ready_ = store_.Load(templates_);
    storage_status_ = store_.LastStatus();
}

bool AlphabetApp::HasActiveStroke() const
{
    return raw_points_.size() >= 2;
}

bool AlphabetApp::SaveEnabled() const
{
    return mode_ == Mode::train && current_gesture_.valid && HasActiveStroke() && storage_ready_;
}

bool AlphabetApp::DeleteEnabled() const
{
    return mode_ == Mode::train && templates_[selected_index_].recorded && storage_ready_;
}

bool AlphabetApp::HasRecordedGlyphs() const
{
    return std::any_of(templates_.begin(), templates_.end(), [](const GlyphTemplate& glyph) {
        return glyph.recorded;
    });
}

std::size_t AlphabetApp::PageCount() const
{
    return (kGlyphCount + kPageSize - 1) / kPageSize;
}

std::size_t AlphabetApp::PageStart() const
{
    return current_page_ * kPageSize;
}

std::size_t AlphabetApp::CurrentPageRecordedCount() const
{
    const std::size_t start = PageStart();
    const std::size_t end = std::min(start + kPageSize, kGlyphCount);
    std::size_t count = 0;
    for (std::size_t i = start; i < end; ++i) {
        if (templates_[i].recorded) {
            ++count;
        }
    }
    return count;
}

int AlphabetApp::SlotIndexAtPoint(int32_t x, int32_t y) const
{
    for (std::size_t i = 0; i < slot_rects_.size(); ++i) {
        if (slot_rects_[i].Contains(x, y)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

char AlphabetApp::SelectedGlyph() const
{
    return templates_[selected_index_].label;
}

std::size_t AlphabetApp::RecordedCount() const
{
    std::size_t count = 0;
    for (const auto& glyph : templates_) {
        if (glyph.recorded) {
            ++count;
        }
    }
    return count;
}

std::string AlphabetApp::GlyphSummary() const
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%c (%zu/%zu)", SelectedGlyph(), selected_index_ + 1, kGlyphCount);
    return buffer;
}

std::string AlphabetApp::StorageSummary() const
{
    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "%zu saved | page %zu/%zu", RecordedCount(), current_page_ + 1, PageCount());
    return buffer;
}


PointF AlphabetApp::ClampToCanvas(PointF point) const
{
    point.x = std::clamp(point.x, static_cast<float>(canvas_.x + kBrushRadius),
                         static_cast<float>(canvas_.x + canvas_.w - kBrushRadius - 1));
    point.y = std::clamp(point.y, static_cast<float>(canvas_.y + kBrushRadius),
                         static_cast<float>(canvas_.y + canvas_.h - kBrushRadius - 1));
    return point;
}

std::vector<AlphabetApp::RecognitionScore> AlphabetApp::RecognizeCurrentStroke() const
{
    std::vector<RecognitionScore> scores;
    if (!current_gesture_.valid) {
        return scores;
    }

    for (std::size_t i = 0; i < templates_.size(); ++i) {
        if (!templates_[i].recorded) {
            continue;
        }

        const float distance = OptimalCosineDistance(templates_[i].vector, current_gesture_.vector);
        scores.push_back({i, distance, std::cos(distance)});
    }

    std::sort(scores.begin(), scores.end(), [](const RecognitionScore& lhs, const RecognitionScore& rhs) {
        return lhs.cosine_score > rhs.cosine_score;
    });
    return scores;
}

bool AlphabetApp::ReadyForDeferredWriteRender(std::uint32_t now) const
{
    if (mode_ != Mode::write) {
        return true;
    }

    if (touch_down_ || drawing_) {
        return false;
    }

    const bool idle_long_enough = now - last_touch_activity_ms_ >= kWriteUiIdleRefreshMs;
    const bool stale_enough = now - full_render_requested_ms_ >= kWriteUiMaxRefreshLatencyMs;
    return idle_long_enough || stale_enough;
}

void AlphabetApp::SelectGlyph(std::size_t glyph_index)
{
    if (glyph_index >= kGlyphCount) {
        return;
    }

    selected_index_ = glyph_index;
    current_page_ = selected_index_ / kPageSize;
    QueueFullRender();
}

void AlphabetApp::ChangePage(int delta)
{
    const int32_t current = static_cast<int32_t>(current_page_);
    const int32_t minimum = 0;
    const int32_t maximum = static_cast<int32_t>(PageCount()) - 1;
    const int32_t next = std::clamp(current + static_cast<int32_t>(delta), minimum, maximum);
    current_page_ = static_cast<std::size_t>(next);
    const std::size_t start = PageStart();
    const std::size_t end = std::min(start + kPageSize, kGlyphCount);
    if (selected_index_ < start || selected_index_ >= end) {
        selected_index_ = start;
    }
    QueueFullRender();
}

void AlphabetApp::SaveSelectedGlyph()
{
    if (!SaveEnabled()) {
        return;
    }

    auto& glyph = templates_[selected_index_];
    glyph.vector = current_gesture_.vector;
    glyph.recorded = true;

    storage_ready_ = store_.Save(templates_);
    storage_status_ = store_.LastStatus();
    recognition_scores_ = RecognizeCurrentStroke();
    matched_index_ = recognition_scores_.empty() ? -1 : static_cast<int>(recognition_scores_.front().glyph_index);
    QueueFullRender();
}

void AlphabetApp::DeleteSelectedGlyph()
{
    if (!DeleteEnabled()) {
        return;
    }

    templates_[selected_index_].recorded = false;
    templates_[selected_index_].vector.clear();
    storage_ready_ = store_.Save(templates_);
    storage_status_ = store_.LastStatus();
    recognition_scores_ = RecognizeCurrentStroke();
    matched_index_ = recognition_scores_.empty() ? -1 : static_cast<int>(recognition_scores_.front().glyph_index);
    QueueFullRender();
}

void AlphabetApp::ReloadGlyphs()
{
    GlyphStore::InitializeTemplates(templates_);
    LoadTemplatesFromDisk();
    recognition_scores_ = RecognizeCurrentStroke();
    matched_index_ = recognition_scores_.empty() ? -1 : static_cast<int>(recognition_scores_.front().glyph_index);
    write_status_ = "> Templates reloaded.";
    QueueFullRender();
}

void AlphabetApp::ClearStroke()
{
    raw_points_.clear();
    resampled_points_.clear();
    current_gesture_ = {};
    recognition_scores_.clear();
    matched_index_ = -1;
    drawing_ = false;
    canvas_reset_pending_ = true;
    pending_segments_.clear();
    if (mode_ == Mode::write) {
        return;
    }
    QueueFullRender();
}

void AlphabetApp::TryAppendRecognizedGlyph()
{
    if (mode_ != Mode::write) {
        return;
    }

    if (!HasRecordedGlyphs()) {
        write_status_ = "> No templates loaded.";
        return;
    }

    if (recognition_scores_.empty()) {
        write_status_ = "> No match.";
        return;
    }

    const auto& best = recognition_scores_.front();
    if (best.cosine_score < kWriteAcceptanceThreshold) {
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "> %c [%.2f] below threshold.", templates_[best.glyph_index].label,
                      best.cosine_score);
        write_status_ = buffer;
        return;
    }

    write_buffer_.push_back(templates_[best.glyph_index].label);

    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "> APPEND: %c [%.2f]", templates_[best.glyph_index].label,
                  best.cosine_score);
    write_status_ = buffer;
}

void AlphabetApp::AddSpace()
{
    write_buffer_.push_back(' ');
    write_status_ = "> Space.";
    QueueWriteTextBarRender();
}

void AlphabetApp::BackspaceText()
{
    if (!write_buffer_.empty()) {
        write_buffer_.pop_back();
        write_status_ = "> Character removed.";
    } else {
        write_status_ = "> Buffer empty.";
    }
    QueueWriteTextBarRender();
}

void AlphabetApp::ClearText()
{
    write_buffer_.clear();
    write_status_ = "> Buffer cleared.";
    QueueWriteTextBarRender();
}

void AlphabetApp::BeginStroke(const PointF& point)
{
    raw_points_.clear();
    resampled_points_.clear();
    current_gesture_ = {};
    recognition_scores_.clear();
    matched_index_ = -1;
    drawing_ = true;
    canvas_reset_pending_ = true;
    pending_segments_.clear();

    const PointF clamped = ClampToCanvas(point);
    raw_points_.push_back(clamped);
    last_draw_point_ = clamped;
    pending_segments_.push_back({clamped, clamped});
}

void AlphabetApp::ExtendStroke(const PointF& point)
{
    if (!drawing_) {
        return;
    }

    const PointF clamped = ClampToCanvas(point);
    raw_points_.push_back(clamped);
    pending_segments_.push_back({last_draw_point_, clamped});
    last_draw_point_ = clamped;
}

void AlphabetApp::FinishStroke()
{
    drawing_ = false;
    AnalyzeStroke();
    if (mode_ == Mode::write) {
        TryAppendRecognizedGlyph();
        QueueWriteTextBarRender();
        canvas_reset_pending_ = true;
        pending_segments_.clear();
        return;
    }
    QueueFullRender();
}

void AlphabetApp::AnalyzeStroke()
{
    resampled_points_ = Resample(raw_points_, kResampleCount);
    current_gesture_ = Vectorize(resampled_points_);
    recognition_scores_ = RecognizeCurrentStroke();
    matched_index_ = recognition_scores_.empty() ? -1 : static_cast<int>(recognition_scores_.front().glyph_index);
}

void AlphabetApp::HandleTouch()
{
    const bool has_touch = M5.Touch.getCount() > 0;
    const std::uint32_t now = M5.millis();

    if (has_touch) {
        const auto& detail = M5.Touch.getDetail();
        PointF point{static_cast<float>(detail.x), static_cast<float>(detail.y)};
        last_touch_ = point;
        last_touch_activity_ms_ = now;

        if (!touch_down_) {
            touch_down_ = true;
            pressed_action_ = ActionButton::none;
            pressed_slot_ = -1;

            if (canvas_.Contains(detail.x, detail.y)) {
                BeginStroke(point);
                return;
            }

            const int slot_index = mode_ == Mode::train ? SlotIndexAtPoint(detail.x, detail.y) : -1;
            if (slot_index >= 0) {
                pressed_slot_ = slot_index;
                return;
            }

            if (train_mode_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::mode_train;
            } else if (write_mode_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::mode_write;
            } else if (mode_ == Mode::train && page_prev_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::page_prev;
            } else if (mode_ == Mode::train && page_next_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::page_next;
            } else if (save_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = mode_ == Mode::train ? ActionButton::save_glyph : ActionButton::write_space;
            } else if (clear_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = mode_ == Mode::train ? ActionButton::clear_stroke : ActionButton::write_backspace;
            } else if (delete_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = mode_ == Mode::train ? ActionButton::delete_glyph : ActionButton::write_clear_text;
            } else if (reload_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = mode_ == Mode::train ? ActionButton::reload_store : ActionButton::clear_stroke;
            }
            return;
        }

        if (drawing_) {
            ExtendStroke(point);
        }
        return;
    }

    if (!touch_down_) {
        return;
    }

    touch_down_ = false;

    if (drawing_) {
        FinishStroke();
        return;
    }

    if (pressed_slot_ >= 0 && slot_rects_[pressed_slot_].Contains(static_cast<int32_t>(last_touch_.x),
                                                                  static_cast<int32_t>(last_touch_.y))) {
        const std::size_t glyph_index = PageStart() + static_cast<std::size_t>(pressed_slot_);
        if (glyph_index < kGlyphCount) {
            SelectGlyph(glyph_index);
        }
    } else {
        switch (pressed_action_) {
        case ActionButton::mode_train:
            if (train_mode_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                mode_ = Mode::train;
                write_status_ = "> TRAIN protocol active.";
                BuildLayout();
                QueueFullRender();
            }
            break;
        case ActionButton::mode_write:
            if (write_mode_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                mode_ = Mode::write;
                write_status_ = HasRecordedGlyphs() ? "> WRITE protocol active."
                                                    : "> No templates loaded.";
                BuildLayout();
                QueueFullRender();
            }
            break;
        case ActionButton::page_prev:
            if (page_prev_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                ChangePage(-1);
            }
            break;
        case ActionButton::page_next:
            if (page_next_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                ChangePage(1);
            }
            break;
        case ActionButton::save_glyph:
            if (save_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                SaveSelectedGlyph();
            }
            break;
        case ActionButton::clear_stroke:
            if (clear_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                ClearStroke();
            }
            break;
        case ActionButton::delete_glyph:
            if (delete_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                DeleteSelectedGlyph();
            }
            break;
        case ActionButton::reload_store:
            if (reload_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                ReloadGlyphs();
            }
            break;
        case ActionButton::write_space:
            if (save_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                AddSpace();
            }
            break;
        case ActionButton::write_backspace:
            if (clear_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                BackspaceText();
            }
            break;
        case ActionButton::write_clear_text:
            if (delete_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                ClearText();
            }
            break;
        case ActionButton::none:
            break;
        }
    }

    pressed_slot_ = -1;
    pressed_action_ = ActionButton::none;
}

void AlphabetApp::RenderFullUi()
{
    M5.Display.setEpdMode(epd_mode_t::epd_text);
    M5.Display.startWrite();
    M5.Display.fillScreen(kColorBg);

    DrawHeader();
    DrawCanvasCard();
    if (mode_ == Mode::train) {
        DrawPaletteCard();
    } else {
        DrawTextBufferBar();
    }
    DrawBottomBar();

    M5.Display.endWrite();
}

void AlphabetApp::QueueFullRender()
{
    full_render_pending_ = true;
    full_render_requested_ms_ = M5.millis();
}

void AlphabetApp::QueueWriteTextBarRender()
{
    if (mode_ != Mode::write) {
        return;
    }
    write_text_bar_pending_ = true;
}

void AlphabetApp::ProcessPendingDisplayWork()
{
    if (M5.Display.displayBusy()) {
        return;
    }

    if (canvas_reset_pending_) {
        ClearCanvasForLiveStroke();
        canvas_reset_pending_ = false;
        return;
    }

    if (!pending_segments_.empty()) {
        const std::size_t flush_count = std::min(kMaxQueuedSegmentsPerFlush, pending_segments_.size());
        for (std::size_t i = 0; i < flush_count; ++i) {
            DrawLiveStrokeSegment(pending_segments_[i].from, pending_segments_[i].to);
        }
        pending_segments_.erase(pending_segments_.begin(), pending_segments_.begin() + flush_count);
        return;
    }

    if (write_text_bar_pending_) {
        RenderWriteTextBufferBar();
        write_text_bar_pending_ = false;
        return;
    }

    if (full_render_pending_ && ReadyForDeferredWriteRender(M5.millis())) {
        RenderFullUi();
        full_render_pending_ = false;
    }
}

void AlphabetApp::ClearCanvasForLiveStroke()
{
    M5.Display.setEpdMode(epd_mode_t::epd_fast);
    M5.Display.startWrite();
    M5.Display.fillRect(canvas_.x + 2, canvas_.y + 2, canvas_.w - 4, canvas_.h - 4, kColorBg);
    M5.Display.drawRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, kColorFg);
    DrawCornerMarks(M5.Display, canvas_);
    M5.Display.endWrite();
}

void AlphabetApp::DrawLiveStrokeSegment(const PointF& from, const PointF& to)
{
    const PointF clamped_from = ClampToCanvas(from);
    const PointF clamped_to = ClampToCanvas(to);
    const float dx = clamped_to.x - clamped_from.x;
    const float dy = clamped_to.y - clamped_from.y;
    const int32_t steps = std::max(static_cast<int32_t>(std::fabs(dx)), static_cast<int32_t>(std::fabs(dy)));

    M5.Display.setEpdMode(epd_mode_t::epd_fast);
    M5.Display.startWrite();
    M5.Display.setClipRect(canvas_.x + 2, canvas_.y + 2, canvas_.w - 4, canvas_.h - 4);

    if (steps == 0) {
        M5.Display.fillCircle(static_cast<int32_t>(clamped_from.x), static_cast<int32_t>(clamped_from.y), kBrushRadius,
                              kColorFg);
    } else {
        for (int32_t i = 0; i <= steps; i += kBrushStepPx) {
            const int32_t x = static_cast<int32_t>(clamped_from.x + (dx * static_cast<float>(i) / steps));
            const int32_t y = static_cast<int32_t>(clamped_from.y + (dy * static_cast<float>(i) / steps));
            M5.Display.fillCircle(x, y, kBrushRadius, kColorFg);
        }
        M5.Display.fillCircle(static_cast<int32_t>(clamped_to.x), static_cast<int32_t>(clamped_to.y), kBrushRadius,
                              kColorFg);
    }

    M5.Display.clearClipRect();
    M5.Display.endWrite();
}

void AlphabetApp::DrawHeader()
{
    // Full black Navi-style header bar
    M5.Display.fillRect(header_.x, header_.y, header_.w, header_.h, kColorFg);

    M5.Display.setTextFont(4);
    M5.Display.setTextColor(kColorBg, kColorFg);
    M5.Display.drawString("NAVI://graffiti", 16, 14);

    // Mode buttons on dark bar — active is bright (white), inactive blends in
    auto draw_mode_btn = [&](const Rect& r, const char* label, bool active) {
        if (active) {
            M5.Display.fillRect(r.x, r.y, r.w, r.h, kColorBg);
            M5.Display.setTextDatum(middle_center);
            M5.Display.setTextColor(kColorFg, kColorBg);
        } else {
            M5.Display.drawRect(r.x, r.y, r.w, r.h, kColorDim);
            M5.Display.setTextDatum(middle_center);
            M5.Display.setTextColor(kColorDim, kColorFg);
        }
        M5.Display.setTextFont(4);
        M5.Display.drawString(label, r.x + r.w / 2, r.y + r.h / 2);
        M5.Display.setTextDatum(top_left);
        M5.Display.setTextFont(2);
    };

    draw_mode_btn(train_mode_button_, "TRAIN", mode_ == Mode::train);
    draw_mode_btn(write_mode_button_, "WRITE", mode_ == Mode::write);

    // Layer indicator
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorDim, kColorFg);
    M5.Display.drawString("LAYER:07", screen_.w - 80, 36);
}

void AlphabetApp::DrawCanvasCard()
{
    DrawPanel(M5.Display, canvas_card_,
              mode_ == Mode::train ? "INPUT_STREAM" : "HANDWRITING_INPUT");

    // Canvas drawing surface
    M5.Display.fillRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, kColorBg);
    M5.Display.drawRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, kColorFg);
    DrawCornerMarks(M5.Display, canvas_);
    DrawGestureOverlay();
}

void AlphabetApp::DrawPaletteCard()
{
    DrawPanel(M5.Display, palette_card_, "GLYPH_DB");

    const std::size_t start = PageStart();
    const std::size_t end = std::min(start + kPageSize, kGlyphCount);
    for (std::size_t slot = 0; slot < slot_rects_.size(); ++slot) {
        const std::size_t glyph_index = start + slot;
        const Rect& rect = slot_rects_[slot];
        const bool valid = glyph_index < end;
        const bool selected = valid && glyph_index == selected_index_;
        const bool recorded = valid && templates_[glyph_index].recorded;
        const bool matched = valid && static_cast<int>(glyph_index) == matched_index_;

        // Lain glyph chips: selected=inverted, recorded=dark, empty=outlined
        const std::uint32_t fill = !valid ? kColorGhost
            : (selected ? kColorFg : (recorded ? kColorRecorded : kColorBg));
        const std::uint32_t text = !valid ? kColorDim
            : (selected ? kColorBg : (recorded ? kColorBg : kColorFg));

        M5.Display.fillRect(rect.x, rect.y, rect.w, rect.h, fill);
        M5.Display.drawRect(rect.x, rect.y, rect.w, rect.h, kColorFg);
        if (matched) {
            M5.Display.drawRect(rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2, kColorFg);
        }

        if (valid) {
            char label[16];
            std::snprintf(label, sizeof(label), "%c%s", templates_[glyph_index].label, recorded ? "*" : "");
            M5.Display.setTextDatum(middle_center);
            M5.Display.setTextColor(text, fill);
            M5.Display.setTextFont(4);
            M5.Display.drawString(label, rect.x + rect.w / 2, rect.y + rect.h / 2);
            M5.Display.setTextDatum(top_left);
            M5.Display.setTextFont(2);
        }
    }

    DrawButton(M5.Display, page_prev_button_, "< PREV", current_page_ > 0, false);
    DrawButton(M5.Display, page_next_button_, "NEXT >", current_page_ + 1 < PageCount(), false);

    // Status area below page nav
    const int32_t status_y = page_next_button_.y + page_next_button_.h + 16;
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorDim, kColorBg);
    M5.Display.drawString(StorageSummary().c_str(), palette_card_.x + 14, status_y);

    M5.Display.setTextColor(kColorFg, kColorBg);
    M5.Display.drawString((">> " + GlyphSummary()).c_str(), palette_card_.x + 14, status_y + 20);

    if (!recognition_scores_.empty()) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "MATCH: %c [%.2f]",
                      templates_[recognition_scores_.front().glyph_index].label,
                      recognition_scores_.front().cosine_score);
        M5.Display.drawString(buffer, palette_card_.x + 14, status_y + 40);
    }

    M5.Display.setTextColor(kColorDim, kColorBg);
    M5.Display.drawString(storage_status_.c_str(), palette_card_.x + 14, status_y + 64);
}

void AlphabetApp::DrawBottomBar()
{
    if (mode_ == Mode::train) {
        DrawButton(M5.Display, save_button_, ">SAVE", SaveEnabled(), true, false, 4);
        DrawButton(M5.Display, clear_button_, ">CLEAR", true, false, false, 4);
        DrawButton(M5.Display, delete_button_, ">DELETE", DeleteEnabled(), false, false, 4);
        DrawButton(M5.Display, reload_button_, ">RELOAD", storage_ready_, false, false, 4);
    } else {
        DrawButton(M5.Display, save_button_, ">SPACE", true, false, false, 4);
        DrawButton(M5.Display, clear_button_, ">BACK", true, false, false, 4);
        DrawButton(M5.Display, delete_button_, ">CLEAR", true, true, false, 4);
    }
}

void AlphabetApp::DrawTextBufferBar()
{
    DrawPanel(M5.Display, text_buffer_bar_, "OUTPUT_BUFFER");

    // Written text in large font
    M5.Display.setTextFont(4);
    const std::string visible = write_buffer_.empty() ? std::string("_")
        : (write_buffer_.size() > 40 ? write_buffer_.substr(write_buffer_.size() - 40) : write_buffer_);
    M5.Display.setTextColor(write_buffer_.empty() ? kColorGhost : kColorFg, kColorBg);
    M5.Display.drawString(visible.c_str(), text_buffer_bar_.x + 14, text_buffer_bar_.y + 28);

    // Status line
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorDim, kColorBg);
    M5.Display.drawString(write_status_.c_str(), text_buffer_bar_.x + 14, text_buffer_bar_.y + 54);
}

void AlphabetApp::RenderWriteTextBufferBar()
{
    if (mode_ != Mode::write || text_buffer_bar_.w <= 0 || text_buffer_bar_.h <= 0) {
        return;
    }

    M5.Display.setEpdMode(epd_mode_t::epd_text);
    M5.Display.startWrite();
    M5.Display.fillRect(text_buffer_bar_.x, text_buffer_bar_.y, text_buffer_bar_.w, text_buffer_bar_.h, kColorBg);
    DrawTextBufferBar();
    M5.Display.endWrite();
}

void AlphabetApp::DrawGestureOverlay()
{
    if (raw_points_.empty()) {
        M5.Display.setTextFont(2);
        M5.Display.setTextColor(kColorGhost, kColorBg);
        M5.Display.setTextDatum(middle_center);
        M5.Display.drawString(mode_ == Mode::train ? "> input glyph here"
                                                   : "> draw to append",
                              canvas_.x + canvas_.w / 2, canvas_.y + canvas_.h / 2);
        M5.Display.setTextDatum(top_left);
        return;
    }

    M5.Display.startWrite();
    for (std::size_t i = 1; i < raw_points_.size(); ++i) {
        M5.Display.drawLine(static_cast<int32_t>(raw_points_[i - 1].x), static_cast<int32_t>(raw_points_[i - 1].y),
                            static_cast<int32_t>(raw_points_[i].x), static_cast<int32_t>(raw_points_[i].y), kColorFg);
    }

    for (std::size_t i = 0; i < resampled_points_.size(); ++i) {
        const int32_t radius = i == 0 ? 4 : 3;
        M5.Display.fillCircle(static_cast<int32_t>(resampled_points_[i].x), static_cast<int32_t>(resampled_points_[i].y),
                              radius, kColorGhost);
    }
    M5.Display.endWrite();
}

}  // namespace alphabet_graffiti
