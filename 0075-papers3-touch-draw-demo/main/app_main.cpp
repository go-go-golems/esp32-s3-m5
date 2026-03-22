#include <M5Unified.hpp>
#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace {

constexpr char kTitle[] = "PaperS3 Touch Draw Demo";
constexpr char kSubtitle[] = "Draw inside the frame. Tap CLEAR to erase.";
constexpr int32_t kScreenMargin = 16;
constexpr int32_t kHeaderHeight = 76;
constexpr int32_t kButtonWidth = 180;
constexpr int32_t kButtonHeight = 48;
constexpr int32_t kButtonRadius = 10;
constexpr int32_t kCanvasRadius = 12;
constexpr int32_t kBrushRadius = 4;
constexpr int32_t kBrushStepPx = 2;
constexpr std::uint32_t kLoopDelayMs = 12;
constexpr std::uint32_t kUiFillColor = 0xF3F3F3;
constexpr std::uint32_t kButtonFillColor = 0xD8D8D8;

struct Rect {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;

    bool contains(int32_t px, int32_t py) const
    {
        return px >= x && px < (x + w) && py >= y && py < (y + h);
    }
};

struct Point {
    int32_t x;
    int32_t y;
};

class PaperS3DrawDemo {
public:
    void run()
    {
        initBoard();
        buildLayout();
        redrawUi();

        while (true) {
            M5.update();
            handleTouch();
            M5.delay(kLoopDelayMs);
        }
    }

private:
    Rect clear_button_{};
    Rect canvas_{};
    bool touch_down_ = false;
    bool clear_gesture_armed_ = false;
    bool stroke_active_ = false;
    Point last_touch_{};
    Point last_stroke_point_{};

    void initBoard()
    {
        auto cfg = M5.config();
        cfg.clear_display = true;
        M5.begin(cfg);
        M5.Display.setRotation(1);
        M5.Display.setTextDatum(top_left);
        M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.Display.setTextSize(1);
        M5.Display.setTextFont(2);
    }

    void buildLayout()
    {
        const int32_t width = M5.Display.width();
        const int32_t height = M5.Display.height();

        clear_button_ = {
            width - kScreenMargin - kButtonWidth,
            kScreenMargin + 12,
            kButtonWidth,
            kButtonHeight,
        };

        canvas_ = {
            kScreenMargin,
            kHeaderHeight + kScreenMargin,
            width - (kScreenMargin * 2),
            height - kHeaderHeight - (kScreenMargin * 2),
        };
    }

    void redrawUi()
    {
        M5.Display.waitDisplay();
        M5.Display.setEpdMode(epd_mode_t::epd_text);
        M5.Display.startWrite();
        M5.Display.fillScreen(TFT_WHITE);

        M5.Display.fillRoundRect(kScreenMargin, kScreenMargin, M5.Display.width() - (kScreenMargin * 2), kHeaderHeight,
                                 kCanvasRadius, kUiFillColor);
        M5.Display.drawRoundRect(kScreenMargin, kScreenMargin, M5.Display.width() - (kScreenMargin * 2), kHeaderHeight,
                                 kCanvasRadius, TFT_BLACK);

        M5.Display.setTextFont(4);
        M5.Display.drawString(kTitle, kScreenMargin + 16, kScreenMargin + 12);
        M5.Display.setTextFont(2);
        M5.Display.setTextColor(0x303030, kUiFillColor);
        M5.Display.drawString(kSubtitle, kScreenMargin + 18, kScreenMargin + 44);

        drawClearButton();
        drawCanvasFrame();

        M5.Display.endWrite();
        M5.Display.waitDisplay();
    }

    void drawClearButton()
    {
        M5.Display.fillRoundRect(clear_button_.x, clear_button_.y, clear_button_.w, clear_button_.h, kButtonRadius,
                                 kButtonFillColor);
        M5.Display.drawRoundRect(clear_button_.x, clear_button_.y, clear_button_.w, clear_button_.h, kButtonRadius,
                                 TFT_BLACK);

        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(TFT_BLACK, kButtonFillColor);
        M5.Display.setTextFont(4);
        M5.Display.drawString("CLEAR", clear_button_.x + clear_button_.w / 2, clear_button_.y + clear_button_.h / 2);
        M5.Display.setTextDatum(top_left);
        M5.Display.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.Display.setTextFont(2);
    }

    void drawCanvasFrame()
    {
        M5.Display.drawRoundRect(canvas_.x, canvas_.y, canvas_.w, canvas_.h, kCanvasRadius, TFT_BLACK);
        M5.Display.drawRoundRect(canvas_.x + 1, canvas_.y + 1, canvas_.w - 2, canvas_.h - 2, kCanvasRadius - 2,
                                 0x8A8A8A);
    }

    Point clampToCanvas(Point point) const
    {
        point.x = std::clamp(point.x, canvas_.x + kBrushRadius, canvas_.x + canvas_.w - 1 - kBrushRadius);
        point.y = std::clamp(point.y, canvas_.y + kBrushRadius, canvas_.y + canvas_.h - 1 - kBrushRadius);
        return point;
    }

    bool isInsideCanvas(Point point) const
    {
        return canvas_.contains(point.x, point.y);
    }

    void clearCanvas()
    {
        redrawUi();
    }

    void drawBrushStroke(Point from, Point to)
    {
        from = clampToCanvas(from);
        to = clampToCanvas(to);

        const int32_t dx = to.x - from.x;
        const int32_t dy = to.y - from.y;
        const int32_t steps = std::max(std::abs(dx), std::abs(dy));

        M5.Display.setEpdMode(epd_mode_t::epd_fast);
        M5.Display.startWrite();
        M5.Display.setClipRect(canvas_.x + 2, canvas_.y + 2, canvas_.w - 4, canvas_.h - 4);

        if (steps == 0) {
            M5.Display.fillCircle(from.x, from.y, kBrushRadius, TFT_BLACK);
        } else {
            for (int32_t i = 0; i <= steps; i += kBrushStepPx) {
                const int32_t x = from.x + (dx * i) / steps;
                const int32_t y = from.y + (dy * i) / steps;
                M5.Display.fillCircle(x, y, kBrushRadius, TFT_BLACK);
            }
            M5.Display.fillCircle(to.x, to.y, kBrushRadius, TFT_BLACK);
        }

        M5.Display.clearClipRect();
        M5.Display.endWrite();
    }

    void handleTouch()
    {
        const bool has_touch = M5.Touch.getCount() > 0;

        if (has_touch) {
            const auto& detail = M5.Touch.getDetail();
            Point point{detail.x, detail.y};

            if (!touch_down_) {
                touch_down_ = true;
                last_touch_ = point;
                clear_gesture_armed_ = clear_button_.contains(point.x, point.y);
                stroke_active_ = !clear_gesture_armed_ && isInsideCanvas(point);

                if (stroke_active_) {
                    last_stroke_point_ = clampToCanvas(point);
                    drawBrushStroke(last_stroke_point_, last_stroke_point_);
                }

                return;
            }

            last_touch_ = point;
            if (stroke_active_) {
                const Point next = clampToCanvas(point);
                if (next.x != last_stroke_point_.x || next.y != last_stroke_point_.y) {
                    drawBrushStroke(last_stroke_point_, next);
                    last_stroke_point_ = next;
                }
            }

            return;
        }

        if (!touch_down_) {
            return;
        }

        if (clear_gesture_armed_ && clear_button_.contains(last_touch_.x, last_touch_.y)) {
            clearCanvas();
        }

        touch_down_ = false;
        clear_gesture_armed_ = false;
        stroke_active_ = false;
    }
};

}  // namespace

extern "C" void app_main(void)
{
    PaperS3DrawDemo app;
    app.run();
}
