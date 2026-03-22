#include "trainer_app.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace protractor_demo {

namespace {

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
constexpr std::uint32_t kColorCard = 0xF1F1F1;
constexpr std::uint32_t kColorCardDark = 0xE0E0E0;
constexpr std::uint32_t kColorInk = 0x000000;
constexpr std::uint32_t kColorMuted = 0x5A5A5A;
constexpr std::uint32_t kColorLightInk = 0x888888;
constexpr std::uint32_t kColorAccent = 0x2C2C2C;
constexpr std::uint32_t kColorAccentFill = 0xD4D4D4;
constexpr std::uint32_t kColorSuccess = 0x6A6A6A;
constexpr std::uint32_t kColorDisabled = 0xC9C9C9;

void DrawRoundCard(M5GFX& display, const Rect& rect, std::uint32_t fill)
{
    display.fillRoundRect(rect.x, rect.y, rect.w, rect.h, kCardRadius, fill);
    display.drawRoundRect(rect.x, rect.y, rect.w, rect.h, kCardRadius, kColorInk);
}

void DrawButton(M5GFX& display, const Rect& rect, const char* label, bool enabled, bool primary)
{
    const std::uint32_t fill = !enabled ? kColorDisabled : (primary ? kColorAccentFill : kColorCardDark);
    display.fillRoundRect(rect.x, rect.y, rect.w, rect.h, kButtonRadius, fill);
    display.drawRoundRect(rect.x, rect.y, rect.w, rect.h, kButtonRadius, kColorInk);

    display.setTextDatum(middle_center);
    display.setTextColor(kColorInk, fill);
    display.setTextFont(2);
    display.drawString(label, rect.x + rect.w / 2, rect.y + rect.h / 2);
    display.setTextDatum(top_left);
}

}  // namespace

void TrainerApp::Run()
{
    InitBoard();
    BuildLayout();
    RenderFullUi();

    while (true) {
        M5.update();
        HandleTouch();
        M5.delay(kLoopDelayMs);
    }
}

void TrainerApp::InitBoard()
{
    auto cfg = M5.config();
    cfg.clear_display = true;
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorInk, kColorPaper);

    for (std::size_t i = 0; i < slots_.size(); ++i) {
        slots_[i].label = static_cast<char>('A' + static_cast<int>(i));
    }
}

void TrainerApp::BuildLayout()
{
    screen_ = {0, 0, M5.Display.width(), M5.Display.height()};
    header_ = {kScreenMargin, 14, screen_.w - (kScreenMargin * 2), kHeaderHeight};
    canvas_card_ = {kScreenMargin, 92, kCanvasCardWidth, kCanvasCardHeight};
    canvas_ = {
        canvas_card_.x + kCanvasInset,
        canvas_card_.y + 40,
        canvas_card_.w - (kCanvasInset * 2),
        canvas_card_.h - 54,
    };

    const int32_t right_x = canvas_card_.x + canvas_card_.w + 18;
    const int32_t right_w = screen_.w - right_x - kScreenMargin;
    templates_card_ = {right_x, 92, right_w, 124};
    controls_card_ = {right_x, 224, right_w, 84};
    metrics_card_ = {right_x, 316, right_w, 92};
    results_card_ = {right_x, 416, right_w, 96};

    const int32_t slot_w = (templates_card_.w - 36) / 4;
    const int32_t slot_h = 30;
    for (std::size_t i = 0; i < slot_rects_.size(); ++i) {
        const int32_t row = static_cast<int32_t>(i / 4);
        const int32_t col = static_cast<int32_t>(i % 4);
        slot_rects_[i] = {
            templates_card_.x + 12 + (col * (slot_w + 4)),
            templates_card_.y + 38 + (row * (slot_h + 10)),
            slot_w,
            slot_h,
        };
    }

    const int32_t button_w = (controls_card_.w - 34) / 2;
    const int32_t button_h = 26;
    save_button_ = {controls_card_.x + 12, controls_card_.y + 34, button_w, button_h};
    clear_button_ = {controls_card_.x + 22 + button_w, controls_card_.y + 34, button_w, button_h};
    delete_button_ = {controls_card_.x + 12, controls_card_.y + 34 + button_h + 8, button_w, button_h};
    reset_button_ = {controls_card_.x + 22 + button_w, controls_card_.y + 34 + button_h + 8, button_w, button_h};
}

bool TrainerApp::HasActiveStroke() const
{
    return raw_points_.size() >= 2;
}

bool TrainerApp::SaveEnabled() const
{
    return current_gesture_.valid && HasActiveStroke();
}

bool TrainerApp::DeleteEnabled() const
{
    return slots_[selected_slot_].recorded;
}

PointF TrainerApp::ClampToCanvas(PointF point) const
{
    point.x = std::clamp(point.x, static_cast<float>(canvas_.x + kBrushRadius),
                         static_cast<float>(canvas_.x + canvas_.w - kBrushRadius - 1));
    point.y = std::clamp(point.y, static_cast<float>(canvas_.y + kBrushRadius),
                         static_cast<float>(canvas_.y + canvas_.h - kBrushRadius - 1));
    return point;
}

float TrainerApp::Degrees(float radians) const
{
    return radians * 180.0f / 3.14159265358979323846f;
}

std::string TrainerApp::FormatPoint(PointF point) const
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "(%.0f, %.0f)", point.x, point.y);
    return buffer;
}

std::string TrainerApp::FormatDegrees(float radians) const
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.1f deg", Degrees(radians));
    return buffer;
}

void TrainerApp::ClearCanvasForLiveStroke()
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

void TrainerApp::DrawLiveStrokeSegment(const PointF& from, const PointF& to)
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

void TrainerApp::BeginStroke(const PointF& point)
{
    raw_points_.clear();
    resampled_points_.clear();
    current_gesture_ = {};
    recognition_scores_.clear();
    matched_slot_ = -1;
    drawing_ = true;

    ClearCanvasForLiveStroke();

    const PointF clamped = ClampToCanvas(point);
    raw_points_.push_back(clamped);
    last_draw_point_ = clamped;
    DrawLiveStrokeSegment(clamped, clamped);
}

void TrainerApp::ExtendStroke(const PointF& point)
{
    if (!drawing_) {
        return;
    }

    const PointF clamped = ClampToCanvas(point);
    raw_points_.push_back(clamped);
    DrawLiveStrokeSegment(last_draw_point_, clamped);
    last_draw_point_ = clamped;
}

void TrainerApp::FinishStroke()
{
    drawing_ = false;
    AnalyzeStroke();
    RenderFullUi();
}

void TrainerApp::AnalyzeStroke()
{
    resampled_points_ = Resample(raw_points_, kResampleCount);
    current_gesture_ = Vectorize(resampled_points_);
    recognition_scores_ = RecognizeCurrentStroke();
    matched_slot_ = recognition_scores_.empty() ? -1 : static_cast<int>(recognition_scores_.front().slot_index);
}

std::vector<TrainerApp::RecognitionScore> TrainerApp::RecognizeCurrentStroke() const
{
    std::vector<RecognitionScore> scores;
    if (!current_gesture_.valid) {
        return scores;
    }

    for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (!slots_[i].recorded) {
            continue;
        }

        const float distance = OptimalCosineDistance(slots_[i].vector, current_gesture_.vector);
        scores.push_back({
            i,
            distance,
            std::cos(distance),
        });
    }

    std::sort(scores.begin(), scores.end(), [](const RecognitionScore& lhs, const RecognitionScore& rhs) {
        return lhs.cosine_score > rhs.cosine_score;
    });
    return scores;
}

void TrainerApp::SaveToSelectedSlot()
{
    if (!SaveEnabled()) {
        return;
    }

    auto& slot = slots_[selected_slot_];
    slot.vector = current_gesture_.vector;
    slot.recorded = true;
    recognition_scores_ = RecognizeCurrentStroke();
    matched_slot_ = recognition_scores_.empty() ? -1 : static_cast<int>(recognition_scores_.front().slot_index);
    RenderFullUi();
}

void TrainerApp::DeleteSelectedSlot()
{
    if (!DeleteEnabled()) {
        return;
    }

    slots_[selected_slot_] = TemplateSlot{static_cast<char>('A' + static_cast<int>(selected_slot_)), false, {}};
    recognition_scores_ = RecognizeCurrentStroke();
    matched_slot_ = recognition_scores_.empty() ? -1 : static_cast<int>(recognition_scores_.front().slot_index);
    RenderFullUi();
}

void TrainerApp::ClearStroke()
{
    raw_points_.clear();
    resampled_points_.clear();
    current_gesture_ = {};
    recognition_scores_.clear();
    matched_slot_ = -1;
    drawing_ = false;
    RenderFullUi();
}

void TrainerApp::ResetAll()
{
    ClearStroke();
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        slots_[i] = TemplateSlot{static_cast<char>('A' + static_cast<int>(i)), false, {}};
    }
    RenderFullUi();
}

void TrainerApp::HandleTouch()
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

            for (std::size_t i = 0; i < slot_rects_.size(); ++i) {
                if (slot_rects_[i].Contains(detail.x, detail.y)) {
                    pressed_slot_ = static_cast<int>(i);
                    return;
                }
            }

            if (save_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::save;
            } else if (clear_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::clear_stroke;
            } else if (delete_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::delete_slot;
            } else if (reset_button_.Contains(detail.x, detail.y)) {
                pressed_action_ = ActionButton::reset_all;
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
        selected_slot_ = static_cast<std::size_t>(pressed_slot_);
        RenderFullUi();
    } else {
        switch (pressed_action_) {
        case ActionButton::save:
            if (save_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                SaveToSelectedSlot();
            }
            break;
        case ActionButton::clear_stroke:
            if (clear_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                ClearStroke();
            }
            break;
        case ActionButton::delete_slot:
            if (delete_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                DeleteSelectedSlot();
            }
            break;
        case ActionButton::reset_all:
            if (reset_button_.Contains(static_cast<int32_t>(last_touch_.x), static_cast<int32_t>(last_touch_.y))) {
                ResetAll();
            }
            break;
        case ActionButton::none:
            break;
        }
    }

    pressed_slot_ = -1;
    pressed_action_ = ActionButton::none;
}

void TrainerApp::RenderFullUi()
{
    M5.Display.waitDisplay();
    M5.Display.setEpdMode(epd_mode_t::epd_text);
    M5.Display.startWrite();
    M5.Display.fillScreen(kColorPaper);

    DrawHeader();
    DrawCanvasCard();
    DrawTemplatesCard();
    DrawControlsCard();
    DrawMetricsCard();
    DrawResultsCard();

    M5.Display.endWrite();
    M5.Display.waitDisplay();
}

void TrainerApp::DrawHeader()
{
    DrawRoundCard(M5.Display, header_, kColorCard);
    M5.Display.setTextFont(4);
    M5.Display.setTextColor(kColorInk, kColorCard);
    M5.Display.drawString("PaperS3 Protractor Trainer", header_.x + 16, header_.y + 10);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString("Tap a slot, draw a gesture, SAVE it, then compare matches on release.", header_.x + 18,
                          header_.y + 38);
}

void TrainerApp::DrawCanvasCard()
{
    DrawRoundCard(M5.Display, canvas_card_, kColorCard);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString("Canvas", canvas_card_.x + 14, canvas_card_.y + 12);
    M5.Display.drawString("Raw stroke, resampled points, centroid, angle", canvas_card_.x + 90, canvas_card_.y + 12);

    M5.Display.fillRoundRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, kCardRadius - 4, kColorPaper);
    M5.Display.drawRoundRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, kCardRadius - 4, kColorInk);
    M5.Display.drawRoundRect(canvas_.x + 1, canvas_.y + 1, canvas_.w - 2, canvas_.h - 2, kCardRadius - 5,
                             kColorLightInk);
    DrawGestureOverlay();
}

void TrainerApp::DrawTemplatesCard()
{
    DrawRoundCard(M5.Display, templates_card_, kColorCard);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString("Template slots", templates_card_.x + 12, templates_card_.y + 10);

    for (std::size_t i = 0; i < slot_rects_.size(); ++i) {
        const auto& rect = slot_rects_[i];
        const bool selected = i == selected_slot_;
        const bool matched = static_cast<int>(i) == matched_slot_;
        const bool recorded = slots_[i].recorded;
        const std::uint32_t fill = selected ? kColorAccentFill : (recorded ? kColorCardDark : kColorPaper);

        M5.Display.fillRoundRect(rect.x, rect.y, rect.w, rect.h, kChipRadius, fill);
        M5.Display.drawRoundRect(rect.x, rect.y, rect.w, rect.h, kChipRadius, matched ? kColorSuccess : kColorInk);
        if (selected) {
            M5.Display.drawRoundRect(rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2, kChipRadius - 1, kColorInk);
        }

        char label[16];
        std::snprintf(label, sizeof(label), "%c%s", slots_[i].label, recorded ? "*" : "");
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(kColorInk, fill);
        M5.Display.drawString(label, rect.x + rect.w / 2, rect.y + rect.h / 2);
        M5.Display.setTextDatum(top_left);
    }

    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString("Selected slot is overwritten when SAVE is tapped.", templates_card_.x + 12,
                          templates_card_.y + templates_card_.h - 18);
}

void TrainerApp::DrawControlsCard()
{
    DrawRoundCard(M5.Display, controls_card_, kColorCard);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString("Actions", controls_card_.x + 12, controls_card_.y + 10);

    DrawButton(M5.Display, save_button_, "SAVE SLOT", SaveEnabled(), true);
    DrawButton(M5.Display, clear_button_, "CLEAR STROKE", true, false);
    DrawButton(M5.Display, delete_button_, "DELETE SLOT", DeleteEnabled(), false);
    DrawButton(M5.Display, reset_button_, "RESET ALL", true, false);
}

void TrainerApp::DrawMetricsCard()
{
    DrawRoundCard(M5.Display, metrics_card_, kColorCard);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString("Preprocessing", metrics_card_.x + 12, metrics_card_.y + 10);

    const int32_t left = metrics_card_.x + 12;
    const int32_t right = metrics_card_.x + metrics_card_.w - 12;
    const int32_t top = metrics_card_.y + 32;
    const int32_t row_h = 16;

    auto draw_row = [&](int32_t row, const char* label, const std::string& value) {
        M5.Display.setTextColor(kColorMuted, kColorCard);
        M5.Display.drawString(label, left, top + row * row_h);
        M5.Display.setTextDatum(top_right);
        M5.Display.setTextColor(kColorInk, kColorCard);
        M5.Display.drawString(value.c_str(), right, top + row * row_h);
        M5.Display.setTextDatum(top_left);
    };

    draw_row(0, "Raw points", std::to_string(raw_points_.size()));
    draw_row(1, "Resampled", std::to_string(resampled_points_.size()));
    draw_row(2, "Centroid", current_gesture_.valid ? FormatPoint(current_gesture_.centroid) : std::string("--"));
    draw_row(3, "Angle / delta",
             current_gesture_.valid ? (FormatDegrees(current_gesture_.indicative_angle_rad) + " / " +
                                       FormatDegrees(current_gesture_.rotation_delta_rad))
                                    : std::string("--"));
}

void TrainerApp::DrawResultsCard()
{
    DrawRoundCard(M5.Display, results_card_, kColorCard);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(kColorMuted, kColorCard);
    M5.Display.drawString("Recognition", results_card_.x + 12, results_card_.y + 10);

    if (recognition_scores_.empty()) {
        const char* message = std::none_of(slots_.begin(), slots_.end(), [](const TemplateSlot& slot) {
                                  return slot.recorded;
                              })
                                  ? "Save a template, then draw to classify."
                                  : "Draw a gesture and release to see matches.";
        M5.Display.setTextColor(kColorMuted, kColorCard);
        M5.Display.drawString(message, results_card_.x + 12, results_card_.y + 40);
        return;
    }

    const float max_cosine = std::max(0.001f, recognition_scores_.front().cosine_score);
    const int32_t start_y = results_card_.y + 30;

    for (std::size_t i = 0; i < recognition_scores_.size() && i < 4; ++i) {
        const auto& score = recognition_scores_[i];
        const auto& slot = slots_[score.slot_index];
        const int32_t row_y = start_y + static_cast<int32_t>(i) * 16;
        const int32_t bar_x = results_card_.x + 56;
        const int32_t bar_w = results_card_.w - 128;
        const int32_t bar_h = 8;
        const int32_t fill_w =
            static_cast<int32_t>(std::clamp(score.cosine_score / max_cosine, 0.0f, 1.0f) * bar_w);

        char label[8];
        std::snprintf(label, sizeof(label), "%c", slot.label);
        M5.Display.setTextColor(kColorInk, kColorCard);
        M5.Display.drawString(label, results_card_.x + 14, row_y - 3);

        M5.Display.fillRoundRect(bar_x, row_y, bar_w, bar_h, 4, kColorPaper);
        M5.Display.drawRoundRect(bar_x, row_y, bar_w, bar_h, 4, kColorLightInk);
        M5.Display.fillRoundRect(bar_x, row_y, fill_w, bar_h, 4, i == 0 ? kColorAccent : kColorCardDark);

        char value[24];
        std::snprintf(value, sizeof(value), "%.3f", score.cosine_score);
        M5.Display.setTextDatum(top_right);
        M5.Display.drawString(value, results_card_.x + results_card_.w - 12, row_y - 3);
        M5.Display.setTextDatum(top_left);
    }
}

void TrainerApp::DrawGestureOverlay()
{
    if (raw_points_.empty()) {
        M5.Display.setTextFont(2);
        M5.Display.setTextColor(kColorLightInk, kColorPaper);
        M5.Display.setTextDatum(middle_center);
        M5.Display.drawString("Draw a gesture here", canvas_.x + canvas_.w / 2, canvas_.y + canvas_.h / 2);
        M5.Display.setTextDatum(top_left);
        return;
    }

    M5.Display.startWrite();
    for (std::size_t i = 1; i < raw_points_.size(); ++i) {
        M5.Display.drawLine(static_cast<int32_t>(raw_points_[i - 1].x), static_cast<int32_t>(raw_points_[i - 1].y),
                            static_cast<int32_t>(raw_points_[i].x), static_cast<int32_t>(raw_points_[i].y), kColorInk);
    }

    if (!resampled_points_.empty()) {
        for (std::size_t i = 0; i < resampled_points_.size(); ++i) {
            const int32_t radius = i == 0 ? 4 : 3;
            M5.Display.fillCircle(static_cast<int32_t>(resampled_points_[i].x), static_cast<int32_t>(resampled_points_[i].y),
                                  radius, kColorLightInk);
        }

        if (current_gesture_.valid) {
            const int32_t cx = static_cast<int32_t>(current_gesture_.centroid.x);
            const int32_t cy = static_cast<int32_t>(current_gesture_.centroid.y);
            M5.Display.drawCircle(cx, cy, 10, kColorSuccess);
            M5.Display.fillCircle(cx, cy, 4, kColorSuccess);

            const int32_t line_len = 42;
            const int32_t ex = cx + static_cast<int32_t>(std::cos(current_gesture_.indicative_angle_rad) * line_len);
            const int32_t ey = cy + static_cast<int32_t>(std::sin(current_gesture_.indicative_angle_rad) * line_len);
            M5.Display.drawLine(cx, cy, ex, ey, kColorMuted);
        }
    }
    M5.Display.endWrite();
}

}  // namespace protractor_demo
