#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "film_catalog.h"
#include "film_selector_controller.h"
#include "film_selector_screen.h"
#include "input_events.h"
#include "lvgl_port_m5dial.h"
#include "m5dial_board.h"
#include "recipe_selector_model.h"

namespace {

static const char *TAG = "m5dial_film_0073";
constexpr uint32_t kUiTaskStackSize = 12288;
constexpr uint32_t kIoTaskStackSize = 4096;
constexpr UBaseType_t kUiTaskPriority = 5;
constexpr UBaseType_t kIoTaskPriority = 6;
constexpr uint32_t kUiTickMs = 10;
constexpr uint32_t kIoPollMs = 8;
constexpr size_t kInputQueueLength = 32;

struct AppContext {
  tutorial_0072::M5DialBoard board;
  tutorial_0073::FilmCatalog catalog;
  tutorial_0073::RecipeSelectorModel selector;
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
  lvgl_cfg.buffer_lines = 40;
  lvgl_cfg.tick_ms = 2;
  lvgl_cfg.double_buffer = false;
  lvgl_cfg.swap_bytes = false;
  if (!tutorial_0072::lvgl_port_m5dial_init(ctx->board.display(), lvgl_cfg)) {
    ESP_LOGE(TAG, "LVGL port init failed");
    vTaskDelete(nullptr);
    return;
  }

  tutorial_0073::FilmSelectorScreen screen;
  if (!screen.init()) {
    ESP_LOGE(TAG, "selector screen init failed");
    vTaskDelete(nullptr);
    return;
  }

  tutorial_0073::FilmSelectorController controller;
  screen.apply(ctx->selector.snapshot());
  ESP_LOGI(TAG, "film developer selector started");

  while (true) {
    tutorial_0072::InputEvent event;
    const TickType_t wait_ticks = pdMS_TO_TICKS(kUiTickMs);
    if (xQueueReceive(ctx->input_queue, &event, wait_ticks == 0 ? 1 : wait_ticks) == pdTRUE) {
      controller.handle_event(event, ctx->selector, screen);
      while (xQueueReceive(ctx->input_queue, &event, 0) == pdTRUE) {
        controller.handle_event(event, ctx->selector, screen);
      }
    }

    screen.apply(ctx->selector.snapshot());
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

  if (!app.catalog.init()) {
    ESP_LOGE(TAG, "film catalog init failed");
    return;
  }

  const tutorial_0073::FilmCatalogStats &catalog_stats = app.catalog.stats();
  ESP_LOGI(TAG,
           "film catalog ready: recipes=%u films=%u developers=%u",
           static_cast<unsigned>(catalog_stats.recipe_count),
           static_cast<unsigned>(catalog_stats.film_count),
           static_cast<unsigned>(catalog_stats.developer_count));

  if (!app.selector.init(app.catalog)) {
    ESP_LOGE(TAG, "selector init failed");
    return;
  }

  const tutorial_0073::SelectorSnapshot selector_snapshot = app.selector.snapshot();
  if (selector_snapshot.resolved_recipe) {
    ESP_LOGI(TAG,
             "selector ready: film=%s developer=%s dilution=%s temp=%d.%dC push=%s time=%us",
             selector_snapshot.selection.film.data(),
             selector_snapshot.selection.developer.data(),
             selector_snapshot.selection.dilution.data(),
             selector_snapshot.selection.temperature_tenths_c / 10,
             selector_snapshot.selection.temperature_tenths_c % 10,
             selector_snapshot.selection.push_pull_type.data(),
             static_cast<unsigned>(selector_snapshot.resolved_recipe->time_seconds));
  }

  xTaskCreatePinnedToCore(ui_task, "m5dial_ui", kUiTaskStackSize, &app, kUiTaskPriority, nullptr, 1);
  xTaskCreatePinnedToCore(io_task, "m5dial_io", kIoTaskStackSize, &app, kIoTaskPriority, nullptr, 0);
}
