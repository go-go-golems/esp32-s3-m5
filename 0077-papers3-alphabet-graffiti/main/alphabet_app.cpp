#include "alphabet_app.h"

namespace alphabet_graffiti {

namespace {

constexpr std::uint32_t kPaper = 0xFFFFFF;
constexpr std::uint32_t kCard = 0xF1F1F1;
constexpr std::uint32_t kInk = 0x000000;
constexpr std::uint32_t kMuted = 0x666666;

}  // namespace

void AlphabetApp::Run()
{
    InitBoard();
    DrawSkeletonUi();

    while (true) {
        M5.update();
        M5.delay(20);
    }
}

void AlphabetApp::InitBoard()
{
    auto cfg = M5.config();
    cfg.clear_display = true;
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextFont(2);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextColor(kInk, kPaper);
}

void AlphabetApp::DrawSkeletonUi()
{
    auto& display = M5.Display;
    const int32_t width = display.width();
    const int32_t height = display.height();

    display.waitDisplay();
    display.setEpdMode(epd_mode_t::epd_text);
    display.startWrite();
    display.fillScreen(kPaper);

    display.fillRoundRect(18, 16, width - 36, 64, 14, kCard);
    display.drawRoundRect(18, 16, width - 36, 64, 14, kInk);
    display.setTextFont(4);
    display.drawString("PaperS3 Alphabet Graffiti", 34, 28);
    display.setTextFont(2);
    display.setTextColor(kMuted, kCard);
    display.drawString("Task 1 skeleton: training mode plus writing mode foundation.", 36, 56);

    display.setTextColor(kInk, kPaper);
    display.fillRoundRect(18, 94, 220, 46, 12, 0xDCDCDC);
    display.drawRoundRect(18, 94, 220, 46, 12, kInk);
    display.drawString("TRAIN MODE", 84, 110);

    display.fillRoundRect(252, 94, 220, 46, 12, kCard);
    display.drawRoundRect(252, 94, 220, 46, 12, kInk);
    display.drawString("WRITE MODE", 320, 110);

    display.fillRoundRect(18, 154, 454, height - 172, 16, kCard);
    display.drawRoundRect(18, 154, 454, height - 172, 16, kInk);
    display.setTextColor(kMuted, kCard);
    display.drawString("Canvas and glyph controls land in the next task.", 34, 176);
    display.drawString("Persistent template storage will be added on-disk.", 34, 198);

    display.fillRoundRect(490, 154, width - 508, height - 172, 16, kCard);
    display.drawRoundRect(490, 154, width - 508, height - 172, 16, kInk);
    display.drawString("Task plan", 506, 176);
    display.drawString("1. Project skeleton", 506, 206);
    display.drawString("2. Persistent training", 506, 226);
    display.drawString("3. Graffiti writing", 506, 246);

    display.endWrite();
    display.waitDisplay();
}

}  // namespace alphabet_graffiti
