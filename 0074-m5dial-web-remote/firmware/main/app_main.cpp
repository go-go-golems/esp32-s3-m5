#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif_ip_addr.h"
#include "esp_timer.h"
#include "lwip/inet.h"

#include "input_events.h"
#include "m5dial_board.h"
#include "remote_client.h"
#include "remote_config.h"
#include "remote_console.h"
#include "wifi_console.h"
#include "wifi_mgr.h"

namespace {

static const char* TAG = "m5dial_remote_0074";
constexpr uint32_t kIoTaskStackSize = 4096;
constexpr uint32_t kAppTaskStackSize = 8192;
constexpr UBaseType_t kIoTaskPriority = 6;
constexpr UBaseType_t kAppTaskPriority = 5;
constexpr uint32_t kIoPollMs = 8;
constexpr size_t kInputQueueLength = 32;

struct AppContext {
  tutorial_0072::M5DialBoard board;
  QueueHandle_t input_queue = nullptr;
  int32_t position = 0;
  int32_t last_delta = 0;
  uint32_t sequence = 0;
  char last_event[48] = "boot";
  uint64_t last_event_ms = 0;
};

uint64_t now_ms() {
  return static_cast<uint64_t>(esp_timer_get_time() / 1000LL);
}

void ensure_device_id(RemoteConfig* cfg) {
  if (!cfg || cfg->device_id[0] != '\0') {
    return;
  }

  uint8_t mac[6] = {0};
  ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
  std::snprintf(cfg->device_id,
                sizeof(cfg->device_id),
                "m5dial-%02x%02x%02x",
                mac[3],
                mac[4],
                mac[5]);
  ESP_ERROR_CHECK(remote_config_save(*cfg));
}

void io_task(void* arg) {
  auto* ctx = static_cast<AppContext*>(arg);
  ctx->board.set_button_irq_task(xTaskGetCurrentTaskHandle());

  while (true) {
    ctx->board.poll(ctx->input_queue);
    const TickType_t wait_ticks = pdMS_TO_TICKS(kIoPollMs);
    ulTaskNotifyTake(pdTRUE, wait_ticks == 0 ? 1 : wait_ticks);
  }
}

void shorten(const char* src, char* dst, size_t dst_len) {
  if (!dst || dst_len == 0) {
    return;
  }
  if (!src || src[0] == '\0') {
    std::snprintf(dst, dst_len, "-");
    return;
  }
  const size_t len = std::strlen(src);
  if (len < dst_len) {
    std::snprintf(dst, dst_len, "%s", src);
    return;
  }
  if (dst_len < 5) {
    dst[0] = '\0';
    return;
  }
  const size_t keep = dst_len - 4;
  std::memcpy(dst, src, keep);
  std::memcpy(dst + keep, "...", 4);
}

void draw_status_screen(AppContext* ctx) {
  tutorial_0072::LGFX_M5Dial& display = ctx->board.display();

  wifi_mgr_status_t wifi = {};
  (void)wifi_mgr_get_status(&wifi);

  RemoteClientStatus remote = {};
  remote_client_get_status(&remote);

  char ip_line[48] = "IP: -";
  if (wifi.ip4 != 0) {
    ip4_addr_t ip = {.addr = htonl(wifi.ip4)};
    std::snprintf(ip_line, sizeof(ip_line), "IP: " IPSTR, IP2STR(&ip));
  }

  char url_line[40] = {};
  char event_line[40] = {};
  char err_line[40] = {};
  shorten(remote.url, url_line, sizeof(url_line));
  shorten(ctx->last_event, event_line, sizeof(event_line));
  shorten(remote.last_error, err_line, sizeof(err_line));

  display.startWrite();
  display.fillScreen(TFT_BLACK);
  display.setTextColor(TFT_GREEN, TFT_BLACK);
  display.setFont(&fonts::Font0);
  display.setCursor(6, 6);
  display.printf("M5Dial Web Remote\n");
  display.printf("Pos:%" PRId32 "  d:%" PRId32 "  seq:%" PRIu32 "\n", ctx->position, ctx->last_delta, ctx->sequence);
  display.printf("WiFi:%s\n",
                 wifi.state == WIFI_MGR_STATE_CONNECTED ? "CONNECTED"
                 : wifi.state == WIFI_MGR_STATE_CONNECTING ? "CONNECTING"
                                                           : "IDLE");
  display.printf("%s\n", ip_line);
  display.printf("Remote:%s\n",
                 remote.state == RemoteClientState::kConnected   ? "CONNECTED"
                 : remote.state == RemoteClientState::kConnecting ? "CONNECTING"
                 : remote.state == RemoteClientState::kWaitingForWifi ? "WAIT_WIFI"
                                                                       : remote.state == RemoteClientState::kError
                                                                             ? "ERROR"
                                                                             : "IDLE");
  display.printf("ID:%s\n", remote.device_id[0] ? remote.device_id : "-");
  display.printf("URL:%s\n", url_line);
  display.printf("Last:%s\n", event_line);
  display.printf("TX:%" PRIu32 " RX:%" PRIu32 "\n", remote.tx_count, remote.rx_count);
  display.printf("Err:%s\n", err_line[0] ? err_line : "-");
  display.printf("ACM0: help / wifi scan / remote status\n");
  display.endWrite();
}

void handle_input_event(AppContext* ctx, const tutorial_0072::InputEvent& event) {
  const uint64_t ts_ms = now_ms();

  switch (event.type) {
    case tutorial_0072::InputEventType::kEncoderDelta:
      ctx->last_delta = event.value;
      ctx->position += event.value;
      ctx->sequence++;
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "enc %+ld", static_cast<long>(event.value));
      ctx->last_event_ms = ts_ms;
      (void)remote_client_send_encoder(ctx->sequence, ctx->position, event.value, ts_ms);
      break;
    case tutorial_0072::InputEventType::kButtonShortPress:
      ctx->sequence++;
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "button short");
      ctx->last_event_ms = ts_ms;
      (void)remote_client_send_button(ctx->sequence, "short", ctx->position, ts_ms);
      break;
    case tutorial_0072::InputEventType::kButtonLongPress:
      ctx->sequence++;
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "button long");
      ctx->last_event_ms = ts_ms;
      (void)remote_client_send_button(ctx->sequence, "long", ctx->position, ts_ms);
      break;
    case tutorial_0072::InputEventType::kSwipe:
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "swipe");
      ctx->last_event_ms = ts_ms;
      break;
  }
}

void app_task(void* arg) {
  auto* ctx = static_cast<AppContext*>(arg);
  draw_status_screen(ctx);

  uint64_t last_draw_ms = 0;
  while (true) {
    tutorial_0072::InputEvent event;
    const bool got_event = xQueueReceive(ctx->input_queue, &event, pdMS_TO_TICKS(100)) == pdTRUE;
    if (got_event) {
      handle_input_event(ctx, event);
      while (xQueueReceive(ctx->input_queue, &event, 0) == pdTRUE) {
        handle_input_event(ctx, event);
      }
    }

    const uint64_t ms = now_ms();
    if (got_event || ms - last_draw_ms >= 250) {
      draw_status_screen(ctx);
      last_draw_ms = ms;
    }
  }
}

void register_console_commands() {
  remote_console_register();
}

}  // namespace

extern "C" void app_main(void) {
  static AppContext app;

  ESP_LOGI(TAG, "booting M5Dial web remote");
  app.input_queue = xQueueCreate(kInputQueueLength, sizeof(tutorial_0072::InputEvent));
  if (!app.input_queue) {
    ESP_LOGE(TAG, "input queue creation failed");
    return;
  }

  if (!app.board.init()) {
    ESP_LOGE(TAG, "board init failed");
    return;
  }

  ESP_ERROR_CHECK(wifi_mgr_start());

  RemoteConfig cfg = {};
  ESP_ERROR_CHECK(remote_config_load(&cfg));
  ensure_device_id(&cfg);

  ESP_ERROR_CHECK(remote_client_init());
  remote_client_set_config(cfg);
  if (cfg.url[0] != '\0') {
    ESP_ERROR_CHECK(remote_client_connect());
  }

  wifi_console_config_t console_cfg = {};
  console_cfg.prompt = "m5dial> ";
  console_cfg.register_extra = &register_console_commands;
  wifi_console_start(&console_cfg);

  xTaskCreatePinnedToCore(app_task, "m5dial_app", kAppTaskStackSize, &app, kAppTaskPriority, nullptr, 1);
  xTaskCreatePinnedToCore(io_task, "m5dial_io", kIoTaskStackSize, &app, kIoTaskPriority, nullptr, 0);
}
