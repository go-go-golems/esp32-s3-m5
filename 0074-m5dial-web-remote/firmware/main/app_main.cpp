#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>

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
constexpr size_t kUiCommandQueueLength = 8;

struct AppContext {
  tutorial_0072::M5DialBoard board;
  QueueHandle_t input_queue = nullptr;
  QueueHandle_t ui_command_queue = nullptr;
  int32_t position = 0;
  int32_t last_delta = 0;
  uint32_t sequence = 0;
  char last_event[48] = "boot";
  char last_ui_message[64] = "-";
  uint64_t last_event_ms = 0;
};

struct ScreenState {
  std::string title;
  std::string position_line;
  std::string wifi_line;
  std::string ip_line;
  std::string remote_line;
  std::string id_line;
  std::string url_line;
  std::string event_line;
  std::string traffic_line;
  std::string error_line;
  std::string footer_line;

  bool operator==(const ScreenState& other) const {
    return title == other.title && position_line == other.position_line && wifi_line == other.wifi_line &&
           ip_line == other.ip_line && remote_line == other.remote_line && id_line == other.id_line &&
           url_line == other.url_line && event_line == other.event_line && traffic_line == other.traffic_line &&
           error_line == other.error_line && footer_line == other.footer_line;
  }
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

ScreenState make_screen_state(AppContext* ctx) {
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

  ScreenState state = {};
  state.title = "M5Dial Web Remote";
  {
    char line[64];
    std::snprintf(line, sizeof(line), "Pos:%" PRId32 " d:%" PRId32 " seq:%" PRIu32, ctx->position, ctx->last_delta, ctx->sequence);
    state.position_line = line;
  }
  {
    const char* wifi_state = wifi.state == WIFI_MGR_STATE_CONNECTED   ? "CONNECTED"
                             : wifi.state == WIFI_MGR_STATE_CONNECTING ? "CONNECTING"
                                                                       : "IDLE";
    state.wifi_line = std::string("WiFi:") + wifi_state;
  }
  state.ip_line = ip_line;
  {
    const char* remote_state = remote.state == RemoteClientState::kConnected   ? "CONNECTED"
                               : remote.state == RemoteClientState::kConnecting ? "CONNECTING"
                               : remote.state == RemoteClientState::kWaitingForWifi ? "WAIT_WIFI"
                               : remote.state == RemoteClientState::kError          ? "ERROR"
                                                                                     : "IDLE";
    state.remote_line = std::string("Remote:") + remote_state;
  }
  state.id_line = std::string("ID:") + (remote.device_id[0] ? remote.device_id : "-");
  state.url_line = std::string("URL:") + url_line;
  state.event_line = std::string("Last:") + event_line;
  {
    char line[40];
    std::snprintf(line, sizeof(line), "TX:%" PRIu32 " RX:%" PRIu32, remote.tx_count, remote.rx_count);
    state.traffic_line = line;
  }
  state.error_line = std::string("Msg:") + ctx->last_ui_message;
  state.footer_line = "ACM0: help / wifi / remote";
  return state;
}

void draw_status_screen(AppContext* ctx, const ScreenState& state) {
  tutorial_0072::LGFX_M5Dial& display = ctx->board.display();
  lgfx::LGFX_Sprite sprite(&display);
  sprite.setColorDepth(16);
  sprite.createSprite(display.width(), display.height());
  sprite.fillScreen(TFT_BLACK);
  sprite.setTextColor(TFT_GREEN, TFT_BLACK);
  sprite.setFont(&fonts::Font0);
  sprite.setCursor(6, 6);
  sprite.println(state.title.c_str());
  sprite.println(state.position_line.c_str());
  sprite.println(state.wifi_line.c_str());
  sprite.println(state.ip_line.c_str());
  sprite.println(state.remote_line.c_str());
  sprite.println(state.id_line.c_str());
  sprite.println(state.url_line.c_str());
  sprite.println(state.event_line.c_str());
  sprite.println(state.traffic_line.c_str());
  sprite.println(state.error_line.c_str());
  sprite.println(state.footer_line.c_str());

  display.startWrite();
  sprite.pushSprite(0, 0);
  display.endWrite();
  sprite.deleteSprite();
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

void handle_ui_command(AppContext* ctx, const RemoteUiCommand& cmd) {
  const uint64_t ts_ms = now_ms();
  ctx->sequence++;

  switch (cmd.type) {
    case RemoteUiCommandType::kShowMessage:
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "%s", cmd.text[0] ? cmd.text : "-");
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "ui msg");
      (void)remote_client_send_ui_ack(
          ctx->sequence, cmd.request_id, cmd.command, "applied", ctx->position, ctx->last_ui_message, ts_ms);
      break;
    case RemoteUiCommandType::kSetPosition:
      ctx->position = cmd.value;
      ctx->last_delta = 0;
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "pos=%" PRId32, cmd.value);
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "ui pos");
      (void)remote_client_send_ui_ack(
          ctx->sequence, cmd.request_id, cmd.command, "applied", ctx->position, ctx->last_ui_message, ts_ms);
      break;
    case RemoteUiCommandType::kUnknown:
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "unknown");
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "ui ?");
      (void)remote_client_send_ui_ack(
          ctx->sequence, cmd.request_id, cmd.command, "ignored", ctx->position, ctx->last_ui_message, ts_ms);
      break;
  }
  ctx->last_event_ms = ts_ms;
}

void app_task(void* arg) {
  auto* ctx = static_cast<AppContext*>(arg);
  ScreenState last_state = {};
  bool have_last_state = false;

  while (true) {
    tutorial_0072::InputEvent event;
    const bool got_event = xQueueReceive(ctx->input_queue, &event, pdMS_TO_TICKS(100)) == pdTRUE;
    if (got_event) {
      handle_input_event(ctx, event);
      while (xQueueReceive(ctx->input_queue, &event, 0) == pdTRUE) {
        handle_input_event(ctx, event);
      }
    }

    RemoteUiCommand cmd;
    while (xQueueReceive(ctx->ui_command_queue, &cmd, 0) == pdTRUE) {
      handle_ui_command(ctx, cmd);
    }

    const ScreenState next_state = make_screen_state(ctx);
    if (!have_last_state || !(next_state == last_state)) {
      draw_status_screen(ctx, next_state);
      last_state = next_state;
      have_last_state = true;
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
  app.ui_command_queue = xQueueCreate(kUiCommandQueueLength, sizeof(RemoteUiCommand));
  if (!app.ui_command_queue) {
    ESP_LOGE(TAG, "ui command queue creation failed");
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
  remote_client_set_command_queue(app.ui_command_queue);
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
