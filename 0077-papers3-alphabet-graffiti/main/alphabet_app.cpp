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

constexpr int32_t kScreenMargin = 18;
constexpr int32_t kHeaderHeight = 64;
constexpr int32_t kCanvasCardWidth = 410;
constexpr int32_t kCanvasCardHeight = 420;
constexpr int32_t kCanvasInset = 14;
constexpr int32_t kCardRadius = 14;
constexpr int32_t kButtonRadius = 10;
constexpr int32_t kChipRadius = 10;
constexpr int32_t kBrushRadius = 4;
constexpr int32_t kBrushStepPx = 2;
constexpr std::uint32_t kLoopDelayMs = 12;

constexpr std::uint32_t kColorPaper = 0xFFFFFF;
constexpr std::uint32_t kColorCard = 0xF3F3F1;
constexpr std::uint32_t kColorCardDark = 0xE1E1DE;
constexpr std::uint32_t kColorInk = 0x000000;
constexpr std::uint32_t kColorMuted = 0x5A5A5A;
constexpr std::uint32_t kColorLightInk = 0x888888;
constexpr std::uint32_t kColorAccent = 0x202020;
constexpr std::uint32_t kColorAccentFill = 0xD7D7D2;
constexpr std::uint32_t kColorDisabled = 0xCDCDC8;

void DrawRoundCard(M5GFX& display, const Rect& rect, std::uint32_t fill)
{
    display.fillRoundRect(rect.x, rect.y, rect.w, rect.h, kCardRadius, fill);
    display.drawRoundRect(rect.x, rect.y, rect.w, rect.h, kCardRadius, kColorInk);
}

void DrawButton(M5GFX& display, const Rect& rect, const char* label, bool enabled, bool primary, bool selected = false)
{
    const std::uint32_t fill = !enabled ? kColorDisabled : (primary ? kColorAccentFill : kColorCardDark);
    display.fillRoundRect(rect.x, rect.y, rect.w, rect.h, kButtonRadius, fill);
    display.drawRoundRect(rect.x, rect.y, rect.w, rect.h, kButtonRadius, kColorInk);
    if (selected) {
        display.drawRoundRect(rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2, kButtonRadius - 1, kColorInk);
    }

    display.setTextDatum(middle_center);
    display.setTextColor(kColorInk, fill);
    display.setTextFont(2);
    display.drawString(label, rect.x + rect.w / 2, rect.y + rect.h / 2);
    display.setTextDatum(top_left);
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
    M5.Display.setTextColor(kColorInk, kColorPaper);

    GlyphStore::InitializeTemplates(templates_);
}

void AlphabetApp::BuildLayout()
{
    screen_ = {0, 0, M5.Display.width(), M5.Display.height()};
    header_ = {kScreenMargin, 14, screen_.w - (kScreenMargin * 2), kHeaderHeight};
    train_mode_button_ = {header_.x + header_.w - 198, header_.y + 12, 92, 28};
    write_mode_button_ = {header_.x + header_.w - 98, header_.y + 12, 86, 28};

    canvas_card_ = {kScreenMargin, 92, kCanvasCardWidth, kCanvasCardHeight};
    canvas_ = {
        canvas_card_.x + kCanvasInset,
        canvas_card_.y + 40,
        canvas_card_.w - (kCanvasInset * 2),
        canvas_card_.h - 54,
    };

    const int32_t right_x = canvas_card_.x + canvas_card_.w + 18;
    const int32_t right_w = screen_.w - right_x - kScreenMargin;
    palette_card_ = {right_x, 92, right_w, 172};
    controls_card_ = {right_x, 272, right_w, 84};
    metrics_card_ = {right_x, 364, right_w, 74};
    results_card_ = {right_x, 446, right_w, 66};

    const int32_t slot_w = (palette_card_.w - 44) / 4;
    const int32_t slot_h = 28;
    for (std::size_t i = 0; i < slot_rects_.size(); ++i) {
        const int32_t row = static_cast<int32_t>(i / 4);
        const int32_t col = static_cast<int32_t>(i % 4);
        slot_rects_[i] = {
            palette_card_.x + 12 + (col * (slot_w + 6)),
            palette_card_.y + 44 + (row * (slot_h + 8)),
            slot_w,
            slot_h,
        };
    }

    page_prev_button_ = {palette_card_.x + 12, palette_card_.y + palette_card_.h - 36, 56, 24};
    page_next_button_ = {palette_card_.x + palette_card_.w - 68, palette_card_.y + palette_card_.h - 36, 56, 24};

    const int32_t button_w = (controls_card_.w - 34) / 2;
    const int32_t button_h = 26;
    save_button_ = {controls_card_.x + 12, controls_card_.y + 34, button_w, button_h};
    clear_button_ = {controls_card_.x + 22 + button_w, controls_card_.y + 34, button_w, button_h};
    delete_button_ = {controls_card_.x + 12, controls_card_.y + 34 + button_h + 8, button_w, button_h};
    reload_button_ = {controls_card_.x + 22 + button_w, controls_card_.y + 34 + button_h + 8, button_w, button_h};
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

std::string AlphabetApp::FormatPoint(PointF point) const
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "(%.0f, %.0f)", point.x, point.y);
    return buffer;
}

std::string AlphabetApp::ModeSubtitle() const
{
    return mode_ == Mode::train ? "Record templates for A-Z and 0-9, one glyph at a time."
                                : "Write mode arrives in the next task; templates already persist.";
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

void AlphabetApp::SelectGlyph(std::size_t glyph_index)
{
    if (glyph_index >= kGlyphCount) {
        return;
    }

    selected_index_ = glyph_index;
    current_page_ = selected_index_ / kPageSize;
    RenderFullUi();
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
    RenderFullUi();
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
    RenderFullUi();
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
    RenderFullUi();
}

void AlphabetApp::ReloadGlyphs()
{
    GlyphStore::InitializeTemplates(templates_);
    LoadTemplatesFromDisk();
    recognition_scores_ = RecognizeCurrentStroke();
    matched_index_ = recognition_scores_.empty() ? -1 : static_cast<int>(recognition_scores_.front().glyph_index);
    RenderFullUi();
}

void AlphabetApp::ClearStroke()
{
    raw_points_.clear();
    resampled_points_.clear();
    current_gesture_ = {};
    recognition_scores_.clear();
    matched_index_ = -1;
    drawing_ = false;
    RenderFullUi();
}

void AlphabetApp::BeginStroke(const PointF& point)
{
    raw_points_.clear();
    resampled_points_.clear();
    current_gesture_ = {};
    recognition_scores_.clear();
    matched_index_ = -1;
    drawing_ = true;

    ClearCanvasForLiveStroke();

    const PointF clamped = ClampToCanvas(point);
    raw_points_.push_back(clamped);
    last_draw_point_ = clamped;
    DrawLiveStrokeSegment(clamped, clamped);
}

void AlphabetApp::ExtendStroke(const PointF& point)
{
    if (!drawing_) {
        return;
    }

    const PointF clamped = ClampToCanvas(point);
    raw_points_.push_back(clamped);
    DrawLiveStrokeSegment(last_draw_point_, clamped);
    last_draw_point_ = clamped;
}

void AlphabetApp::FinishStroke()
{
    drawing_ = false;
    AnalyzeStroke();
    RenderFullUi();
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

    if (has_touch) {
        const auto& detail = M5.Touch.getDetail();
        PointF point{static_cast<float>(detail.x), static_cast<float>(detail.y)};
        last_touch_ = point;

        if (!touch_down_) {
            touch_down_ = true;
            pressed_action_ = ActionButton::none;
            pressed_slot_ = -1;

            if (canvas_.Contains(detail.x, detail.y)) {
                BeginStroke(point);
                return;
            }

            const int slot_index = SlotIndexAtPoint(detail.x, detail.y);
            if (slot_index >= 0) {
                pressed_slot_ = slot_index;
                return;
            }

            if (train_mode_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::mode_train;
            } else if (write_mode_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::mode_write;
            } else if (page_prev_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::page_prev;
            } else if (page_next_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::page_next;
            } else if (save_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::save_glyph;
            } else if (clear_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::clear_stroke;
            } else if (delete_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::delete_glyph;
            } else if (reload_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::reload_store;
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
                RenderFullUi();
            }
            break;
        case ActionButton::mode_write:
            if (write_mode_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                mode_ = Mode::write;
                RenderFullUi();
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
        case ActionButton::none:
            break;
        }
    }

    pressed_slot_ = -1;
    pressed_action_ = ActionButton::none;
}

void AlphabetApp::RenderFullUi()
{
    M5.Display.waitDisplay();
    M5.Display.setEpdMode(epd_mode_t::epd_text);
    M5.Display.startWrite();
    M5.Display.fillScreen(kColorPaper);

    DrawHeader();
    DrawCanvasCard();
    DrawPaletteCard();
    DrawControlsCard();
    DrawMetricsCard();
    DrawResultsCard();

    M5.Display.endWrite();
    M5.Display.waitDisplay();
}

void AlphabetApp::ClearCanvasForLiveStroke()
{
    M5.Display.waitDisplay();
    M5.Display.setEpdMode(epd_mode_t::epd_fast);
    M5.Display.startWrite();
    M5.Display.fillRect(canvas_.x + 2, canvas_.y + 2, canvas_.w - 4, canvas_.h - 4, kColorPaper);
    M5.Display.drawRoundRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, kCardRadius - 4, kColorInk);
    M5.Display.drawRoundRect(canvas_.x + 1, canvas_.y + 1, canvas_.w - 2, canvas_.h - 2, kCardRadius - 5,
                             kColorLightInk);
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
                              kColorInk);
    } else {
        for (int32_t i = 0; i <= steps; i += kBrushStepPx) {
            const int32_t x = static_cast<int32_t>(clamped_from.x + (dx * static_cast<float>(i) / steps));
            const int32_t y = static_cast<int32_t>(clamped_from.y + (dy * static_cast<float>(i) / steps));
            M5.Display.fillCircle(x, y, kBrushRadius, kColorInk);
        }
        M5.Display.fillCircle(static_cast<int32_t>(clamped_to.x), static_cast<int32_t>(clamped_to.y), kBrushRadius,
                              kColorInk);
    }

    M5.Display.clearClipRect();
    M5.Display.endWrite();
}

void AlphabetApp::DrawHeader()
{
    DrawRoundCard(M5.Display, header_, kColorCard);
    M5.Display.setTextFont(4);
    M5.Display.setTextColor(kColorInk, kColorCard);
    M5.Display.drawString("PaperS3 Alphabet Graffiti", header_.x + 16, header_.y + 8);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString(ModeSubtitle().c_str(), header_.x + 18, header_.y + 40);

    DrawButton(M5.Display, train_mode_button_, "TRAIN", true, mode_ == Mode::train, mode_ == Mode::train);
    DrawButton(M5.Display, write_mode_button_, "WRITE", true, mode_ == Mode::write, mode_ == Mode::write);
}

void AlphabetApp::DrawCanvasCard()
{
    DrawRoundCard(M5.Display, canvas_card_, kColorCard);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString(mode_ == Mode::train ? "Stroke canvas" : "Stroke canvas (preview)", canvas_card_.x + 14,
                          canvas_card_.y + 12);
    M5.Display.drawString(mode_ == Mode::train ? "Draw the selected glyph and save it to flash."
                                               : "Writing mode commit is next; recognition preview already works.",
                          canvas_card_.x + 116, canvas_card_.y + 12);

    M5.Display.fillRoundRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, kCardRadius - 4, kColorPaper);
    M5.Display.drawRoundRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, kCardRadius - 4, kColorInk);
    M5.Display.drawRoundRect(canvas_.x + 1, canvas_.y + 1, canvas_.w - 2, canvas_.h - 2, kCardRadius - 5,
                             kColorLightInk);
    DrawGestureOverlay();
}

void AlphabetApp::DrawPaletteCard()
{
    DrawRoundCard(M5.Display, palette_card_, kColorCard);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString("Glyph picker", palette_card_.x + 12, palette_card_.y + 10);

    const std::size_t start = PageStart();
    const std::size_t end = std::min(start + kPageSize, kGlyphCount);
    for (std::size_t slot = 0; slot < slot_rects_.size(); ++slot) {
        const std::size_t glyph_index = start + slot;
        const Rect& rect = slot_rects_[slot];
        const bool valid = glyph_index < end;
        const bool selected = valid && glyph_index == selected_index_;
        const bool recorded = valid && templates_[glyph_index].recorded;
        const bool matched = valid && static_cast<int>(glyph_index) == matched_index_;
        const std::uint32_t fill = !valid ? kColorDisabled : (selected ? kColorAccentFill : (recorded ? kColorCardDark : kColorPaper));

        M5.Display.fillRoundRect(rect.x, rect.y, rect.w, rect.h, kChipRadius, fill);
        M5.Display.drawRoundRect(rect.x, rect.y, rect.w, rect.h, kChipRadius, matched ? kColorAccent : kColorInk);
        if (selected) {
            M5.Display.drawRoundRect(rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2, kChipRadius - 1, kColorInk);
        }

        if (valid) {
            char label[16];
            std::snprintf(label, sizeof(label), "%c%s", templates_[glyph_index].label, recorded ? "*" : "");
            M5.Display.setTextDatum(middle_center);
            M5.Display.setTextColor(kColorInk, fill);
            M5.Display.drawString(label, rect.x + rect.w / 2, rect.y + rect.h / 2);
            M5.Display.setTextDatum(top_left);
        }
    }

    DrawButton(M5.Display, page_prev_button_, "< PREV", current_page_ > 0, false);
    DrawButton(M5.Display, page_next_button_, "NEXT >", current_page_ + 1 < PageCount(), false);

    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString(StorageSummary().c_str(), palette_card_.x + 78, palette_card_.y + palette_card_.h - 32);
    char page_buffer[32];
    std::snprintf(page_buffer, sizeof(page_buffer), "page %zu", current_page_ + 1);
    M5.Display.drawString(page_buffer, palette_card_.x + 78, palette_card_.y + palette_card_.h - 16);
}

void AlphabetApp::DrawControlsCard()
{
    DrawRoundCard(M5.Display, controls_card_, kColorCard);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString(mode_ == Mode::train ? "Template actions" : "Canvas actions", controls_card_.x + 12,
                          controls_card_.y + 10);

    DrawButton(M5.Display, save_button_, "SAVE GLYPH", SaveEnabled(), true);
    DrawButton(M5.Display, clear_button_, "CLEAR STROKE", true, false);
    DrawButton(M5.Display, delete_button_, "DELETE GLYPH", DeleteEnabled(), false);
    DrawButton(M5.Display, reload_button_, "RELOAD DISK", storage_ready_, false);
}

void AlphabetApp::DrawMetricsCard()
{
    DrawRoundCard(M5.Display, metrics_card_, kColorCard);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString("Status", metrics_card_.x + 12, metrics_card_.y + 10);

    const int32_t left = metrics_card_.x + 12;
    const int32_t top = metrics_card_.y + 30;
    const int32_t row_h = 14;
    const std::string centroid = current_gesture_.valid ? FormatPoint(current_gesture_.centroid) : std::string("--");
    const std::string raw_points = std::to_string(raw_points_.size());
    const std::string glyph = GlyphSummary();

    auto draw_row = [&](int32_t row, const char* label, const std::string& value) {
        M5.Display.setTextColor(kColorMuted, kColorCard);
        M5.Display.drawString(label, left, top + row * row_h);
        M5.Display.setTextColor(kColorInk, kColorCard);
        M5.Display.drawString(value.c_str(), left + 78, top + row * row_h);
    };

    draw_row(0, "Selected", glyph);
    draw_row(1, "Raw points", raw_points);
    draw_row(2, "Centroid", centroid);
}

void AlphabetApp::DrawResultsCard()
{
    DrawRoundCard(M5.Display, results_card_, kColorCard);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString("Disk + match state", results_card_.x + 12, results_card_.y + 10);

    M5.Display.setTextColor(kColorInk, kColorCard);
    M5.Display.drawString(storage_status_.c_str(), results_card_.x + 12, results_card_.y + 28);

    if (recognition_scores_.empty()) {
        const char* message = HasRecordedGlyphs() ? "Draw and release to preview the closest template."
                                                  : "No glyphs saved yet. Pick one and tap SAVE GLYPH.";
        M5.Display.setTextColor(kColorMuted, kColorCard);
        M5.Display.drawString(message, results_card_.x + 12, results_card_.y + 46);
        return;
    }

    char buffer[96];
    std::snprintf(buffer, sizeof(buffer), "Top match: %c (%.3f)", templates_[recognition_scores_.front().glyph_index].label,
                  recognition_scores_.front().cosine_score);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString(buffer, results_card_.x + 12, results_card_.y + 46);
}

void AlphabetApp::DrawGestureOverlay()
{
    if (raw_points_.empty()) {
        M5.Display.setTextFont(2);
        M5.Display.setTextColor(kColorLightInk, kColorPaper);
        M5.Display.setTextDatum(middle_center);
        M5.Display.drawString(mode_ == Mode::train ? "Draw the selected glyph here" : "Draw here; WRITE mode is next",
                              canvas_.x + canvas_.w / 2, canvas_.y + canvas_.h / 2);
        M5.Display.setTextDatum(top_left);
        return;
    }

    M5.Display.startWrite();
    for (std::size_t i = 1; i < raw_points_.size(); ++i) {
        M5.Display.drawLine(static_cast<int32_t>(raw_points_[i - 1].x), static_cast<int32_t>(raw_points_[i - 1].y),
                            static_cast<int32_t>(raw_points_[i].x), static_cast<int32_t>(raw_points_[i].y), kColorInk);
    }

    for (std::size_t i = 0; i < resampled_points_.size(); ++i) {
        const int32_t radius = i == 0 ? 4 : 3;
        M5.Display.fillCircle(static_cast<int32_t>(resampled_points_[i].x), static_cast<int32_t>(resampled_points_[i].y),
                              radius, kColorLightInk);
    }
    M5.Display.endWrite();
}

}  // namespace alphabet_graffiti
