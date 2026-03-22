#pragma once

#include "glyph_store.h"
#include "protractor_math.h"

#include <M5Unified.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace alphabet_graffiti {

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

class AlphabetApp {
public:
    void Run();

private:
    static constexpr std::size_t kGlyphCount = GlyphStore::kGlyphCount;
    static constexpr std::size_t kPageSize = 12;
    static constexpr std::size_t kResampleCount = 16;
    static constexpr float kWriteAcceptanceThreshold = 0.82f;
    static constexpr std::size_t kMaxQueuedSegmentsPerFlush = 8;
    static constexpr std::uint32_t kWriteUiIdleRefreshMs = 180;
    static constexpr std::uint32_t kWriteUiMaxRefreshLatencyMs = 600;

    struct RecognitionScore {
        std::size_t glyph_index = 0;
        float distance = 0.0f;
        float cosine_score = 0.0f;
    };

    struct PendingSegment {
        protractor_demo::PointF from;
        protractor_demo::PointF to;
    };

    enum class Mode {
        train,
        write,
    };

    enum class ActionButton {
        none,
        mode_train,
        mode_write,
        page_prev,
        page_next,
        save_glyph,
        clear_stroke,
        delete_glyph,
        reload_store,
        write_space,
        write_backspace,
        write_clear_text,
    };

    void InitBoard();
    void BuildLayout();
    void LoadTemplatesFromDisk();
    void HandleTouch();
    void BeginStroke(const protractor_demo::PointF& point);
    void ExtendStroke(const protractor_demo::PointF& point);
    void FinishStroke();
    void AnalyzeStroke();

    void SelectGlyph(std::size_t glyph_index);
    void ChangePage(int delta);
    void SaveSelectedGlyph();
    void DeleteSelectedGlyph();
    void ReloadGlyphs();
    void ClearStroke();
    void TryAppendRecognizedGlyph();
    void AddSpace();
    void BackspaceText();
    void ClearText();

    void RenderFullUi();
    void QueueFullRender();
    void ProcessPendingDisplayWork();
    void ClearCanvasForLiveStroke();
    void DrawLiveStrokeSegment(const protractor_demo::PointF& from, const protractor_demo::PointF& to);

    void DrawHeader();
    void DrawCanvasCard();
    void DrawPaletteCard();
    void DrawBottomBar();
    void DrawTextBufferBar();
    void DrawGestureOverlay();

    bool HasActiveStroke() const;
    bool SaveEnabled() const;
    bool DeleteEnabled() const;
    bool HasRecordedGlyphs() const;
    std::size_t PageCount() const;
    std::size_t PageStart() const;
    std::size_t CurrentPageRecordedCount() const;
    int SlotIndexAtPoint(int32_t x, int32_t y) const;
    char SelectedGlyph() const;
    std::size_t RecordedCount() const;
    std::string GlyphSummary() const;
    std::string StorageSummary() const;
    protractor_demo::PointF ClampToCanvas(protractor_demo::PointF point) const;
    std::vector<RecognitionScore> RecognizeCurrentStroke() const;
    bool ReadyForDeferredWriteRender(std::uint32_t now) const;

    Rect screen_{};
    Rect header_{};
    Rect train_mode_button_{};
    Rect write_mode_button_{};
    Rect canvas_card_{};
    Rect canvas_{};
    Rect palette_card_{};
    Rect text_buffer_bar_{};
    Rect page_prev_button_{};
    Rect page_next_button_{};
    Rect save_button_{};
    Rect clear_button_{};
    Rect delete_button_{};
    Rect reload_button_{};
    std::array<Rect, kPageSize> slot_rects_{};

    GlyphStore store_{};
    std::array<GlyphTemplate, kGlyphCount> templates_{};
    Mode mode_ = Mode::train;
    std::size_t selected_index_ = 0;
    std::size_t current_page_ = 0;
    bool storage_ready_ = false;
    std::string storage_status_;
    std::string write_buffer_;
    std::string write_status_ = "> Select protocol.";
    int matched_index_ = -1;
    bool canvas_reset_pending_ = false;
    bool full_render_pending_ = false;
    std::uint32_t last_touch_activity_ms_ = 0;
    std::uint32_t full_render_requested_ms_ = 0;

    bool touch_down_ = false;
    bool drawing_ = false;
    int pressed_slot_ = -1;
    ActionButton pressed_action_ = ActionButton::none;
    protractor_demo::PointF last_touch_{};
    protractor_demo::PointF last_draw_point_{};

    std::vector<protractor_demo::PointF> raw_points_;
    std::vector<protractor_demo::PointF> resampled_points_;
    protractor_demo::VectorizedGesture current_gesture_;
    std::vector<RecognitionScore> recognition_scores_;
    std::vector<PendingSegment> pending_segments_;
};

}  // namespace alphabet_graffiti
