#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "input_events.h"
#include "lvgl_port_m5dial.h"
#include "m5dial_board.h"
#include "timer_controller.h"
#include "timer_model.h"
#include "ui_timer_screen.h"

namespace {

static const char *TAG = "m5dial_timer_0072";
constexpr uint32_t kUiTaskStackSize = 12288;
constexpr uint32_t kIoTaskStackSize = 4096;
constexpr UBaseType_t kUiTaskPriority = 5;
constexpr UBaseType_t kIoTaskPriority = 6;
constexpr uint32_t kUiTickMs = 10;
constexpr uint32_t kIoPollMs = 8;
constexpr size_t kInputQueueLength = 32;

struct AppContext {
  tutorial_0072::M5DialBoard board;
  QueueHandle_t input_queue = nullptr;
};

void io_task(void *arg) {
  auto *ctx = static_cast<AppContext *>(arg);
  ctx->board.set_button_irq_task(xTaskGetCurrentTaskHandle());

  while (true) {
    ctx->board.poll(ctx->input_queue);
    const TickType_t wait_ticks = pdMS_TO_TICKS(kIoPollMs);
    ulTaskNotifyTake(pdTRUE, wait_ticks == 0 ? 1 : wait_ticks);
  }
}

void ui_task(void *arg) {
  auto *ctx = static_cast<AppContext *>(arg);

  tutorial_0072::LvglPortM5DialConfig lvgl_cfg{};
  lvgl_cfg.buffer_lines = 120;
  lvgl_cfg.tick_ms = 2;
  lvgl_cfg.double_buffer = true;
  lvgl_cfg.swap_bytes = false;
  if (!tutorial_0072::lvgl_port_m5dial_init(ctx->board.display(), lvgl_cfg)) {
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
    tutorial_0072::InputEvent event;
    const TickType_t wait_ticks = pdMS_TO_TICKS(kUiTickMs);
    if (xQueueReceive(ctx->input_queue, &event, wait_ticks == 0 ? 1 : wait_ticks) == pdTRUE) {
      controller.handle_event(event, model, screen);
      while (xQueueReceive(ctx->input_queue, &event, 0) == pdTRUE) {
        controller.handle_event(event, model, screen);
      }
    }

    const uint64_t now_us = static_cast<uint64_t>(esp_timer_get_time());
    model.tick(now_us);
    screen.apply(model.snapshot());
    lv_timer_handler();
  }
}

}  // namespace

extern "C" void app_main(void) {
  static AppContext app;

  app.input_queue = xQueueCreate(kInputQueueLength, sizeof(tutorial_0072::InputEvent));
  if (!app.input_queue) {
    ESP_LOGE(TAG, "input queue creation failed");
    return;
  }

  if (!app.board.init()) {
    ESP_LOGE(TAG, "board init failed");
    return;
  }

  xTaskCreatePinnedToCore(ui_task, "m5dial_ui", kUiTaskStackSize, &app, kUiTaskPriority, nullptr, 1);
  xTaskCreatePinnedToCore(io_task, "m5dial_io", kIoTaskStackSize, &app, kIoTaskPriority, nullptr, 0);
}
