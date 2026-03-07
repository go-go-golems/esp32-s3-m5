#include <inttypes.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "m5dial_board.h"

namespace {

static const char *TAG = "m5dial_timer_0072";

constexpr uint32_t kBgColor = 0x101416;
constexpr uint32_t kTextColor = 0xF6F1E8;
constexpr uint32_t kAccentColor = 0xE0B04A;
constexpr uint32_t kMutedColor = 0x7A8688;

void draw_smoke_screen(tutorial_0072::M5DialBoard &board,
                       int encoder_total,
                       int button_count,
                       const tutorial_0072::TouchState &touch) {
  auto &display = board.display();

  display.fillScreen(kBgColor);
  display.setTextDatum(lgfx::textdatum_t::middle_center);
  display.setTextColor(kAccentColor, kBgColor);
  display.setTextSize(1);
  display.drawString("M5DIAL TIMER", 120, 34);

  display.setTextColor(kTextColor, kBgColor);
  display.setTextSize(2);
  display.drawString("0072", 120, 74);

  display.setTextColor(kMutedColor, kBgColor);
  display.setTextSize(1);
  display.drawString("Stage 1: Board smoke test", 120, 102);

  display.drawCircle(120, 120, 96, 0x303638);
  display.drawCircle(120, 120, 82, 0x23292B);

  char line[64];
  display.setTextColor(kTextColor, kBgColor);
  snprintf(line, sizeof(line), "Encoder total: %d", encoder_total);
  display.drawString(line, 120, 140);
  snprintf(line, sizeof(line), "Button presses: %d", button_count);
  display.drawString(line, 120, 162);

  if (touch.pressed) {
    snprintf(line, sizeof(line), "Touch: %d, %d", touch.x, touch.y);
  } else {
    snprintf(line, sizeof(line), "Touch: none");
  }
  display.drawString(line, 120, 184);

  display.setTextColor(kMutedColor, kBgColor);
  display.drawString("Rotate / press / touch to test inputs", 120, 214);
}

}  // namespace

extern "C" void app_main(void) {
  tutorial_0072::M5DialBoard board;
  if (!board.init()) {
    ESP_LOGE(TAG, "board init failed");
    return;
  }

  int encoder_total = 0;
  int button_count = 0;
  tutorial_0072::TouchState touch = {};

  draw_smoke_screen(board, encoder_total, button_count, touch);

  while (true) {
    board.poll();

    bool dirty = false;

    const int delta = board.take_encoder_steps();
    if (delta != 0) {
      encoder_total += delta;
      ESP_LOGI(TAG, "encoder delta=%d total=%d", delta, encoder_total);
      dirty = true;
    }

    if (board.take_button_press()) {
      button_count++;
      ESP_LOGI(TAG, "button press count=%d", button_count);
      dirty = true;
    }

    const auto latest_touch = board.read_touch();
    if (latest_touch.pressed != touch.pressed || latest_touch.x != touch.x || latest_touch.y != touch.y) {
      touch = latest_touch;
      dirty = true;
    }

    if (dirty) {
      draw_smoke_screen(board, encoder_total, button_count, touch);
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}
