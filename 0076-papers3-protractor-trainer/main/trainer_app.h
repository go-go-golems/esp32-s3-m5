#pragma once

#include "protractor_math.h"

#include <M5Unified.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace protractor_demo {

struct Rect {
    int32_t x = 0;
    int32_t y = 0;
    int32_t w = 0;
    int32_t h = 0;

    bool Contains(int32_t px, int32_t py) const
    {
        return px >= x && px < (x + w) && py >= y && py < (y + h);
    }
};

class TrainerApp {
public:
    void Run();

private:
    static constexpr std::size_t kTemplateCount = 8;
    static constexpr std::size_t kResampleCount = 16;

    struct TemplateSlot {
        char label = '?';
        bool recorded = false;
        std::vector<float> vector;
    };

    struct RecognitionScore {
        std::size_t slot_index = 0;
        float distance = 0.0f;
        float cosine_score = 0.0f;
    };

    enum class ActionButton {
        none,
        save,
        clear_stroke,
        delete_slot,
        reset_all,
    };

    void InitBoard();
    void BuildLayout();
    void HandleTouch();
    void BeginStroke(const PointF& point);
    void ExtendStroke(const PointF& point);
    void FinishStroke();
    void AnalyzeStroke();

    void SaveToSelectedSlot();
    void DeleteSelectedSlot();
    void ClearStroke();
    void ResetAll();

    void RenderFullUi();
    void ClearCanvasForLiveStroke();
    void DrawLiveStrokeSegment(const PointF& from, const PointF& to);

    void DrawHeader();
    void DrawCanvasCard();
    void DrawTemplatesCard();
    void DrawControlsCard();
    void DrawMetricsCard();
    void DrawResultsCard();
    void DrawGestureOverlay();

    bool HasActiveStroke() const;
    bool SaveEnabled() const;
    bool DeleteEnabled() const;
    PointF ClampToCanvas(PointF point) const;
    float Degrees(float radians) const;
    std::string FormatPoint(PointF point) const;
    std::string FormatDegrees(float radians) const;
    std::vector<RecognitionScore> RecognizeCurrentStroke() const;

    Rect screen_{};
    Rect header_{};
    Rect canvas_card_{};
    Rect canvas_{};
    Rect templates_card_{};
    Rect controls_card_{};
    Rect metrics_card_{};
    Rect results_card_{};
    Rect save_button_{};
    Rect clear_button_{};
    Rect delete_button_{};
    Rect reset_button_{};
    std::array<Rect, kTemplateCount> slot_rects_{};

    std::array<TemplateSlot, kTemplateCount> slots_{};
    std::size_t selected_slot_ = 0;
    int matched_slot_ = -1;

    bool touch_down_ = false;
    bool drawing_ = false;
    int pressed_slot_ = -1;
    ActionButton pressed_action_ = ActionButton::none;
    PointF last_touch_{};
    PointF last_draw_point_{};

    std::vector<PointF> raw_points_;
    std::vector<PointF> resampled_points_;
    VectorizedGesture current_gesture_;
    std::vector<RecognitionScore> recognition_scores_;
};

}  // namespace protractor_demo
