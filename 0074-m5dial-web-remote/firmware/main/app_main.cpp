#include <cinttypes>
#include <cmath>
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

// ─── Mode system ────────────────────────────────────────────

enum class AppMode : uint8_t {
  kDebug = 0,
  kRadio = 1,
};

// ─── Radio state ────────────────────────────────────────────

enum class StationType : uint8_t {
  kEmpty = 0,
  kClear = 1,
  kStatic = 2,
  kHidden = 3,
  kDistorted = 4,
};

constexpr int kNumStations = 120;
constexpr float kArcStartDeg = 135.0f;
constexpr float kArcSpanDeg = 270.0f;

// Lain palette in RGB565
constexpr uint16_t kRadioBg = TFT_BLACK;
constexpr uint16_t kRadioText = 0xC720;     // soft green ~#c8e6c0
constexpr uint16_t kRadioAccent = 0x07E0;    // bright green #00ff00
constexpr uint16_t kRadioDim = 0x0320;       // very dim green
constexpr uint16_t kRadioWarn = 0xFFE0;      // yellow
constexpr uint16_t kRadioDanger = 0xF800;    // red
constexpr uint16_t kRadioOrange = 0xFBE0;    // orange
constexpr uint16_t kRadioWhite = TFT_WHITE;

struct RadioState {
  StationType stations[kNumStations] = {};
  char station_names[kNumStations][16] = {};
  int32_t freq_pos = 0;
  uint8_t band = 2;  // 0=AM, 1=FM, 2=WIRED
  bool locked = false;
  char reveal_text[64] = {};
  uint64_t reveal_until_ms = 0;
  bool dirty = true;
};

// Precomputed sin/cos for station dot positions
static float s_station_cos[kNumStations];
static float s_station_sin[kNumStations];
static bool s_station_trig_ready = false;

void init_station_trig() {
  if (s_station_trig_ready) return;
  for (int i = 0; i < kNumStations; i++) {
    float angle = (kArcStartDeg + i * kArcSpanDeg / (kNumStations - 1)) * static_cast<float>(M_PI) / 180.0f;
    s_station_cos[i] = cosf(angle);
    s_station_sin[i] = sinf(angle);
  }
  s_station_trig_ready = true;
}

// ─── App context ────────────────────────────────────────────

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
  AppMode mode = AppMode::kDebug;
  RadioState radio = {};
};

// ─── Debug mode screen (unchanged) ─────────────────────────

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

// ─── Radio mode drawing ────────────────────────────────────

void draw_radio_screen(AppContext* ctx) {
  init_station_trig();

  tutorial_0072::LGFX_M5Dial& display = ctx->board.display();
  lgfx::LGFX_Sprite sprite(&display);
  sprite.setColorDepth(16);
  sprite.createSprite(240, 240);
  sprite.fillScreen(kRadioBg);

  const int cx = 120;
  const int cy = 120;
  const int32_t fp = ctx->radio.freq_pos;

  // 1. Background frequency arc (dim green, 270 degrees)
  sprite.fillArc(cx, cy, 118, 112, kArcStartDeg, kArcStartDeg + kArcSpanDeg, kRadioDim);

  // 2. Tuned sweep (bright green from start to current position)
  if (fp > 0) {
    float pos_end = kArcStartDeg + fp * kArcSpanDeg / (kNumStations - 1);
    sprite.fillArc(cx, cy, 118, 112, kArcStartDeg, pos_end, kRadioAccent);
  }

  // 3. Position indicator (white dot on the arc)
  {
    int hx = cx + static_cast<int>(115.0f * s_station_cos[fp]);
    int hy = cy + static_cast<int>(115.0f * s_station_sin[fp]);
    sprite.fillCircle(hx, hy, 5, kRadioWhite);
  }

  // 4. Station dots (inner ring at r=95)
  for (int i = 0; i < kNumStations; i++) {
    int dx = cx + static_cast<int>(95.0f * s_station_cos[i]);
    int dy = cy + static_cast<int>(95.0f * s_station_sin[i]);

    uint16_t color;
    int radius;

    if (i == fp) {
      color = kRadioWhite;
      radius = 4;
    } else {
      radius = 2;
      switch (ctx->radio.stations[i]) {
        case StationType::kClear:     color = kRadioAccent; break;
        case StationType::kStatic:    color = kRadioWarn; break;
        case StationType::kHidden:    color = kRadioDanger; break;
        case StationType::kDistorted: color = kRadioOrange; break;
        default:                      color = 0x0120; radius = 1; break;
      }
    }
    sprite.fillCircle(dx, dy, radius, color);
  }

  // 5. Band label (top center)
  {
    static const char* band_names[] = {"AM", "FM", "WIRED"};
    const char* bname = band_names[ctx->radio.band % 3];
    sprite.setFont(&fonts::Font0);
    sprite.setTextSize(1);
    sprite.setTextColor(kRadioText, kRadioBg);
    int tw = static_cast<int>(std::strlen(bname)) * 6;
    sprite.setCursor(cx - tw / 2, 30);
    sprite.print(bname);
  }

  // 6. Frequency display (center, larger)
  {
    char freq_str[20];
    // Map 0-119 to a frequency range for display flavor
    int mhz = 88 + fp * 30 / 119;
    int frac = (fp * 300 / 119) % 10;
    std::snprintf(freq_str, sizeof(freq_str), "%d.%d MHz", mhz, frac);
    sprite.setTextSize(2);
    sprite.setTextColor(kRadioAccent, kRadioBg);
    int tw = static_cast<int>(std::strlen(freq_str)) * 12;
    sprite.setCursor(cx - tw / 2, cy - 24);
    sprite.print(freq_str);
  }

  // 7. Station label or reveal text (center)
  {
    sprite.setTextSize(1);
    bool showing_reveal = ctx->radio.reveal_until_ms > 0 && now_ms() < ctx->radio.reveal_until_ms;

    if (showing_reveal) {
      sprite.setTextColor(kRadioDanger, kRadioBg);
      int tw = static_cast<int>(std::strlen(ctx->radio.reveal_text)) * 6;
      sprite.setCursor(cx - tw / 2, cy + 2);
      sprite.print(ctx->radio.reveal_text);
    } else {
      const char* label = ctx->radio.station_names[fp][0] ? ctx->radio.station_names[fp] : "---";
      StationType st = ctx->radio.stations[fp];
      uint16_t label_color = kRadioText;
      if (st == StationType::kStatic) label_color = kRadioWarn;
      else if (st == StationType::kHidden) label_color = kRadioDanger;
      else if (st == StationType::kDistorted) label_color = kRadioOrange;

      sprite.setTextColor(label_color, kRadioBg);
      int tw = static_cast<int>(std::strlen(label)) * 6;
      sprite.setCursor(cx - tw / 2, cy + 2);
      sprite.print(label);
    }
  }

  // 8. Lock indicator
  if (ctx->radio.locked) {
    sprite.setTextSize(1);
    sprite.setTextColor(kRadioAccent, kRadioBg);
    sprite.setCursor(cx - 18, cy + 18);
    sprite.print("LOCKED");
  }

  // 9. Bottom hint
  {
    sprite.setTextSize(1);
    sprite.setTextColor(kRadioDim, kRadioBg);
    const char* hint = ctx->radio.locked ? "press to unlock" : "twist to tune";
    int tw = static_cast<int>(std::strlen(hint)) * 6;
    sprite.setCursor(cx - tw / 2, 210);
    sprite.print(hint);
  }

  display.startWrite();
  sprite.pushSprite(0, 0);
  display.endWrite();
  sprite.deleteSprite();
}

// ─── Radio helpers ──────────────────────────────────────────

const char* swipe_direction_str(tutorial_0072::SwipeDirection dir) {
  switch (dir) {
    case tutorial_0072::SwipeDirection::kLeft:  return "left";
    case tutorial_0072::SwipeDirection::kRight: return "right";
    case tutorial_0072::SwipeDirection::kUp:    return "up";
    case tutorial_0072::SwipeDirection::kDown:  return "down";
    default: return "unknown";
  }
}

void parse_set_station(AppContext* ctx, const RemoteUiCommand& cmd) {
  // cmd.value = position (0-119), cmd.text = "type:name" e.g. "1:phantom_relay"
  int pos = cmd.value;
  if (pos < 0 || pos >= kNumStations) return;

  const char* text = cmd.text;
  int type_val = 0;
  const char* colon = std::strchr(text, ':');
  if (colon) {
    type_val = text[0] - '0';
    const char* name = colon + 1;
    std::snprintf(ctx->radio.station_names[pos], sizeof(ctx->radio.station_names[pos]), "%s", name);
  }

  if (type_val >= 0 && type_val <= 4) {
    ctx->radio.stations[pos] = static_cast<StationType>(type_val);
  }
  ctx->radio.dirty = true;
}

// ─── Input handling ─────────────────────────────────────────

void handle_input_event(AppContext* ctx, const tutorial_0072::InputEvent& event) {
  const uint64_t ts_ms = now_ms();
  const bool radio = ctx->mode == AppMode::kRadio;

  switch (event.type) {
    case tutorial_0072::InputEventType::kEncoderDelta:
      ctx->last_delta = event.value;
      ctx->position += event.value;
      ctx->sequence++;
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "enc %+ld", static_cast<long>(event.value));
      ctx->last_event_ms = ts_ms;

      if (radio) {
        if (!ctx->radio.locked) {
          int32_t p = ctx->radio.freq_pos + event.value;
          ctx->radio.freq_pos = ((p % kNumStations) + kNumStations) % kNumStations;
          ctx->radio.dirty = true;
        }
        (void)remote_client_send_encoder(ctx->sequence, ctx->radio.freq_pos, event.value, ts_ms);
      } else {
        (void)remote_client_send_encoder(ctx->sequence, ctx->position, event.value, ts_ms);
      }
      break;

    case tutorial_0072::InputEventType::kButtonShortPress:
      ctx->sequence++;
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "button short");
      ctx->last_event_ms = ts_ms;

      if (radio) {
        ctx->radio.locked = !ctx->radio.locked;
        ctx->radio.dirty = true;
        (void)remote_client_send_button(ctx->sequence, "short", ctx->radio.freq_pos, ts_ms);
      } else {
        (void)remote_client_send_button(ctx->sequence, "short", ctx->position, ts_ms);
      }
      break;

    case tutorial_0072::InputEventType::kButtonLongPress:
      ctx->sequence++;
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "button long");
      ctx->last_event_ms = ts_ms;
      (void)remote_client_send_button(ctx->sequence, "long",
                                      radio ? ctx->radio.freq_pos : ctx->position, ts_ms);
      break;

    case tutorial_0072::InputEventType::kSwipe: {
      const char* dir = swipe_direction_str(event.swipe);
      ctx->sequence++;
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "swipe %s", dir);
      ctx->last_event_ms = ts_ms;
      (void)remote_client_send_swipe(ctx->sequence, dir, ts_ms);

      if (radio) {
        if (event.swipe == tutorial_0072::SwipeDirection::kLeft) {
          ctx->radio.band = (ctx->radio.band + 2) % 3;
          ctx->radio.dirty = true;
        } else if (event.swipe == tutorial_0072::SwipeDirection::kRight) {
          ctx->radio.band = (ctx->radio.band + 1) % 3;
          ctx->radio.dirty = true;
        }
      }
      break;
    }
  }
}

// ─── UI command handling ────────────────────────────────────

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
      if (ctx->mode == AppMode::kRadio) {
        ctx->radio.freq_pos = ((cmd.value % kNumStations) + kNumStations) % kNumStations;
        ctx->radio.dirty = true;
      }
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "pos=%" PRId32, cmd.value);
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "ui pos");
      (void)remote_client_send_ui_ack(
          ctx->sequence, cmd.request_id, cmd.command, "applied", ctx->position, ctx->last_ui_message, ts_ms);
      break;

    case RemoteUiCommandType::kSetMode: {
      uint8_t new_mode = static_cast<uint8_t>(cmd.value);
      if (new_mode <= 1) {
        ctx->mode = static_cast<AppMode>(new_mode);
        if (ctx->mode == AppMode::kRadio) {
          ctx->radio.freq_pos = 0;
          ctx->radio.locked = false;
          ctx->radio.dirty = true;
        }
      }
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "mode=%d", new_mode);
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "ui mode");
      (void)remote_client_send_ui_ack(
          ctx->sequence, cmd.request_id, cmd.command, "applied",
          ctx->mode == AppMode::kRadio ? ctx->radio.freq_pos : ctx->position,
          ctx->last_ui_message, ts_ms);
      break;
    }

    case RemoteUiCommandType::kSetStation:
      parse_set_station(ctx, cmd);
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "station %" PRId32, cmd.value);
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "ui station");
      (void)remote_client_send_ui_ack(
          ctx->sequence, cmd.request_id, cmd.command, "applied",
          ctx->radio.freq_pos, ctx->last_ui_message, ts_ms);
      break;

    case RemoteUiCommandType::kSetBand: {
      if (std::strcmp(cmd.text, "AM") == 0) ctx->radio.band = 0;
      else if (std::strcmp(cmd.text, "FM") == 0) ctx->radio.band = 1;
      else ctx->radio.band = 2;
      ctx->radio.dirty = true;
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "band=%.*s",
                    static_cast<int>(sizeof(ctx->last_ui_message) - sizeof("band=")), cmd.text);
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "ui band");
      (void)remote_client_send_ui_ack(
          ctx->sequence, cmd.request_id, cmd.command, "applied",
          ctx->radio.freq_pos, ctx->last_ui_message, ts_ms);
      break;
    }

    case RemoteUiCommandType::kShowReveal:
      std::snprintf(ctx->radio.reveal_text, sizeof(ctx->radio.reveal_text), "%s", cmd.text);
      ctx->radio.reveal_until_ms = ts_ms + 3000;
      ctx->radio.dirty = true;
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "reveal");
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "ui reveal");
      (void)remote_client_send_ui_ack(
          ctx->sequence, cmd.request_id, cmd.command, "applied",
          ctx->radio.freq_pos, cmd.text, ts_ms);
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

// ─── Main app loop ──────────────────────────────────────────

void app_task(void* arg) {
  auto* ctx = static_cast<AppContext*>(arg);
  ScreenState last_debug_state = {};
  bool have_last_debug_state = false;

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

    // Check if reveal text has expired
    if (ctx->mode == AppMode::kRadio && ctx->radio.reveal_until_ms > 0 && now_ms() >= ctx->radio.reveal_until_ms) {
      ctx->radio.reveal_until_ms = 0;
      ctx->radio.dirty = true;
    }

    // Draw based on current mode
    if (ctx->mode == AppMode::kRadio) {
      if (ctx->radio.dirty || got_event) {
        draw_radio_screen(ctx);
        ctx->radio.dirty = false;
      }
      have_last_debug_state = false;  // force debug redraw on mode switch back
    } else {
      const ScreenState next_state = make_screen_state(ctx);
      if (!have_last_debug_state || !(next_state == last_debug_state)) {
        draw_status_screen(ctx, next_state);
        last_debug_state = next_state;
        have_last_debug_state = true;
      }
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
