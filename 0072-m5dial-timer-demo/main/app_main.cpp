#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "lvgl_port_m5dial.h"
#include "m5dial_board.h"
#include "timer_controller.h"
#include "timer_model.h"
#include "ui_timer_screen.h"

namespace {

static const char *TAG = "m5dial_timer_0072";
constexpr uint32_t kAppTaskStackSize = 12288;
constexpr UBaseType_t kAppTaskPriority = 5;
constexpr uint32_t kLoopDelayMs = 20;

void timer_demo_task(void * /*arg*/) {
  tutorial_0072::M5DialBoard board;
  if (!board.init()) {
    ESP_LOGE(TAG, "board init failed");
    vTaskDelete(nullptr);
    return;
  }

  tutorial_0072::LvglPortM5DialConfig lvgl_cfg{};
  lvgl_cfg.buffer_lines = 40;
  lvgl_cfg.tick_ms = 2;
  lvgl_cfg.double_buffer = false;
  lvgl_cfg.swap_bytes = false;
  if (!tutorial_0072::lvgl_port_m5dial_init(board.display(), lvgl_cfg)) {
    ESP_LOGE(TAG, "LVGL port init failed");
    vTaskDelete(nullptr);
    return;
  }

  tutorial_0072::TimerScreen screen;
  if (!screen.init()) {
    ESP_LOGE(TAG, "timer screen init failed");
    vTaskDelete(nullptr);
    return;
  }

  tutorial_0072::TimerModel model;
  tutorial_0072::TimerController controller;
  screen.apply(model.snapshot());
  ESP_LOGI(TAG, "timer demo started");

  while (true) {
    board.poll();
    const uint64_t now_us = static_cast<uint64_t>(esp_timer_get_time());
    controller.update(board, model, screen, now_us);
    model.tick(now_us);
    screen.apply(model.snapshot());
    lv_timer_handler();

    const TickType_t delay_ticks = pdMS_TO_TICKS(kLoopDelayMs);
    vTaskDelay(delay_ticks == 0 ? 1 : delay_ticks);
  }
}

}  // namespace

extern "C" void app_main(void) {
  xTaskCreatePinnedToCore(timer_demo_task, "m5dial_timer", kAppTaskStackSize, nullptr, kAppTaskPriority, nullptr, 1);
}
