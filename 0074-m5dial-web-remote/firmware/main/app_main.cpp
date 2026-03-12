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
#include "app_commands.h"
#include "app_debug.h"
#include "display_console.h"
#include "js_console.h"
#include "js_service.h"
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
constexpr size_t kAppCommandQueueLength = 16;

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
  QueueHandle_t app_command_queue = nullptr;
  int32_t position = 0;
  int32_t last_delta = 0;
  uint32_t sequence = 0;
  char last_event[48] = "boot";
  char last_ui_message[64] = "-";
  uint64_t last_event_ms = 0;
  AppMode mode = AppMode::kDebug;
  RadioState radio = {};
};

static AppContext* s_app_ctx = nullptr;

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
  display.startWrite();
  display.fillScreen(TFT_BLACK);
  display.setTextColor(TFT_GREEN, TFT_BLACK);
  display.setFont(&fonts::Font0);
  display.setTextSize(1);
  display.setCursor(6, 6);
  display.println(state.title.c_str());
  display.println(state.position_line.c_str());
  display.println(state.wifi_line.c_str());
  display.println(state.ip_line.c_str());
  display.println(state.remote_line.c_str());
  display.println(state.id_line.c_str());
  display.println(state.url_line.c_str());
  display.println(state.event_line.c_str());
  display.println(state.traffic_line.c_str());
  display.println(state.error_line.c_str());
  display.println(state.footer_line.c_str());
  display.endWrite();
}

// ─── Radio mode drawing ────────────────────────────────────

void draw_radio_screen(AppContext* ctx) {
  init_station_trig();

  tutorial_0072::LGFX_M5Dial& display = ctx->board.display();
  display.startWrite();
  display.fillScreen(kRadioBg);

  const int cx = 120;
  const int cy = 120;
  const int32_t fp = ctx->radio.freq_pos;

  // 1. Background frequency arc (dim green, 270 degrees)
  display.fillArc(cx, cy, 118, 112, kArcStartDeg, kArcStartDeg + kArcSpanDeg, kRadioDim);

  // 2. Tuned sweep (bright green from start to current position)
  if (fp > 0) {
    float pos_end = kArcStartDeg + fp * kArcSpanDeg / (kNumStations - 1);
    display.fillArc(cx, cy, 118, 112, kArcStartDeg, pos_end, kRadioAccent);
  }

  // 3. Position indicator (white dot on the arc)
  {
    int hx = cx + static_cast<int>(115.0f * s_station_cos[fp]);
    int hy = cy + static_cast<int>(115.0f * s_station_sin[fp]);
    display.fillCircle(hx, hy, 5, kRadioWhite);
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
    display.fillCircle(dx, dy, radius, color);
  }

  // 5. Band label (top center)
  {
    static const char* band_names[] = {"AM", "FM", "WIRED"};
    const char* bname = band_names[ctx->radio.band % 3];
    display.setFont(&fonts::Font0);
    display.setTextSize(1);
    display.setTextColor(kRadioText, kRadioBg);
    int tw = static_cast<int>(std::strlen(bname)) * 6;
    display.setCursor(cx - tw / 2, 30);
    display.print(bname);
  }

  // 6. Frequency display (center, larger)
  {
    char freq_str[20];
    // Map 0-119 to a frequency range for display flavor
    int mhz = 88 + fp * 30 / 119;
    int frac = (fp * 300 / 119) % 10;
    std::snprintf(freq_str, sizeof(freq_str), "%d.%d MHz", mhz, frac);
    display.setTextSize(2);
    display.setTextColor(kRadioAccent, kRadioBg);
    int tw = static_cast<int>(std::strlen(freq_str)) * 12;
    display.setCursor(cx - tw / 2, cy - 24);
    display.print(freq_str);
  }

  // 7. Station label or reveal text (center)
  {
    display.setTextSize(1);
    bool showing_reveal = ctx->radio.reveal_until_ms > 0 && now_ms() < ctx->radio.reveal_until_ms;

    if (showing_reveal) {
      display.setTextColor(kRadioDanger, kRadioBg);
      int tw = static_cast<int>(std::strlen(ctx->radio.reveal_text)) * 6;
      display.setCursor(cx - tw / 2, cy + 2);
      display.print(ctx->radio.reveal_text);
    } else {
      const char* label = ctx->radio.station_names[fp][0] ? ctx->radio.station_names[fp] : "---";
      StationType st = ctx->radio.stations[fp];
      uint16_t label_color = kRadioText;
      if (st == StationType::kStatic) label_color = kRadioWarn;
      else if (st == StationType::kHidden) label_color = kRadioDanger;
      else if (st == StationType::kDistorted) label_color = kRadioOrange;

      display.setTextColor(label_color, kRadioBg);
      int tw = static_cast<int>(std::strlen(label)) * 6;
      display.setCursor(cx - tw / 2, cy + 2);
      display.print(label);
    }
  }

  // 8. Lock indicator
  if (ctx->radio.locked) {
    display.setTextSize(1);
    display.setTextColor(kRadioAccent, kRadioBg);
    display.setCursor(cx - 18, cy + 18);
    display.print("LOCKED");
  }

  // 9. Bottom hint
  {
    display.setTextSize(1);
    display.setTextColor(kRadioDim, kRadioBg);
    const char* hint = ctx->radio.locked ? "press to unlock" : "twist to tune";
    int tw = static_cast<int>(std::strlen(hint)) * 6;
    display.setCursor(cx - tw / 2, 210);
    display.print(hint);
  }

  display.endWrite();
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

void parse_set_station(AppContext* ctx, const AppCommand& cmd) {
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

void maybe_send_command_ack(AppContext* ctx,
                            const AppCommand& cmd,
                            uint32_t sequence,
                            const char* status,
                            int32_t pos,
                            const char* text,
                            uint64_t ts_ms) {
  if (cmd.source != AppCommandSource::kUi) {
    return;
  }
  (void)remote_client_send_ui_ack(sequence, cmd.request_id, cmd.command, status, pos, text, ts_ms);
}

void handle_app_command(AppContext* ctx, const AppCommand& cmd) {
  const uint64_t ts_ms = now_ms();
  ctx->sequence++;

  switch (cmd.type) {
    case AppCommandType::kShowMessage:
      std::snprintf(ctx->last_ui_message,
                    sizeof(ctx->last_ui_message),
                    "%.*s",
                    static_cast<int>(sizeof(ctx->last_ui_message) - 1),
                    cmd.text[0] ? cmd.text : "-");
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "%s msg", app_command_source_name(cmd.source));
      maybe_send_command_ack(ctx, cmd, ctx->sequence, "applied", ctx->position, ctx->last_ui_message, ts_ms);
      break;

    case AppCommandType::kSetPosition:
      ctx->position = cmd.value;
      ctx->last_delta = 0;
      if (ctx->mode == AppMode::kRadio) {
        ctx->radio.freq_pos = ((cmd.value % kNumStations) + kNumStations) % kNumStations;
        ctx->radio.dirty = true;
      }
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "pos=%" PRId32, cmd.value);
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "%s pos", app_command_source_name(cmd.source));
      maybe_send_command_ack(ctx, cmd, ctx->sequence, "applied", ctx->position, ctx->last_ui_message, ts_ms);
      break;

    case AppCommandType::kSetMode: {
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
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "%s mode", app_command_source_name(cmd.source));
      maybe_send_command_ack(ctx, cmd, ctx->sequence, "applied",
                             ctx->mode == AppMode::kRadio ? ctx->radio.freq_pos : ctx->position,
                             ctx->last_ui_message, ts_ms);
      break;
    }

    case AppCommandType::kSetStation:
      parse_set_station(ctx, cmd);
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "station %" PRId32, cmd.value);
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "%s station", app_command_source_name(cmd.source));
      maybe_send_command_ack(ctx, cmd, ctx->sequence, "applied", ctx->radio.freq_pos, ctx->last_ui_message, ts_ms);
      break;

    case AppCommandType::kSetBand: {
      if (std::strcmp(cmd.text, "AM") == 0) ctx->radio.band = 0;
      else if (std::strcmp(cmd.text, "FM") == 0) ctx->radio.band = 1;
      else ctx->radio.band = 2;
      ctx->radio.dirty = true;
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "band=%.*s",
                    static_cast<int>(sizeof(ctx->last_ui_message) - sizeof("band=")), cmd.text);
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "%s band", app_command_source_name(cmd.source));
      maybe_send_command_ack(ctx, cmd, ctx->sequence, "applied", ctx->radio.freq_pos, ctx->last_ui_message, ts_ms);
      break;
    }

    case AppCommandType::kShowReveal:
      std::snprintf(ctx->radio.reveal_text,
                    sizeof(ctx->radio.reveal_text),
                    "%.*s",
                    static_cast<int>(sizeof(ctx->radio.reveal_text) - 1),
                    cmd.text);
      ctx->radio.reveal_until_ms = ts_ms + 3000;
      ctx->radio.dirty = true;
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "reveal");
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "%s reveal", app_command_source_name(cmd.source));
      maybe_send_command_ack(ctx, cmd, ctx->sequence, "applied", ctx->radio.freq_pos, cmd.text, ts_ms);
      break;

    case AppCommandType::kUnknown:
      std::snprintf(ctx->last_ui_message, sizeof(ctx->last_ui_message), "unknown");
      std::snprintf(ctx->last_event, sizeof(ctx->last_event), "%s ?", app_command_source_name(cmd.source));
      maybe_send_command_ack(ctx, cmd, ctx->sequence, "ignored", ctx->position, ctx->last_ui_message, ts_ms);
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

    AppCommand cmd;
    while (xQueueReceive(ctx->app_command_queue, &cmd, 0) == pdTRUE) {
      handle_app_command(ctx, cmd);
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
  display_console_register_commands();
  js_console_register_commands();
  remote_console_register();
}

}  // namespace

bool app_debug_get_status(AppDebugStatus* out) {
  if (!out || !s_app_ctx) {
    return false;
  }

  *out = {};
  out->ready = true;
  out->radio_mode = s_app_ctx->mode == AppMode::kRadio;
  out->touch_ready = s_app_ctx->board.touch_ready();
  out->position = s_app_ctx->position;
  out->radio_position = s_app_ctx->radio.freq_pos;
  std::snprintf(out->last_event, sizeof(out->last_event), "%s", s_app_ctx->last_event);
  std::snprintf(out->last_message, sizeof(out->last_message), "%s", s_app_ctx->last_ui_message);
  return true;
}

bool app_debug_set_mode(bool radio_mode) {
  if (!s_app_ctx) {
    return false;
  }

  s_app_ctx->mode = radio_mode ? AppMode::kRadio : AppMode::kDebug;
  if (radio_mode) {
    s_app_ctx->radio.dirty = true;
  }
  std::snprintf(s_app_ctx->last_event, sizeof(s_app_ctx->last_event), "display mode");
  std::snprintf(s_app_ctx->last_ui_message, sizeof(s_app_ctx->last_ui_message), "%s", radio_mode ? "radio" : "debug");
  return app_debug_redraw();
}

bool app_debug_redraw() {
  if (!s_app_ctx) {
    return false;
  }

  if (s_app_ctx->mode == AppMode::kRadio) {
    s_app_ctx->radio.dirty = true;
    draw_radio_screen(s_app_ctx);
    s_app_ctx->radio.dirty = false;
  } else {
    const ScreenState next_state = make_screen_state(s_app_ctx);
    draw_status_screen(s_app_ctx, next_state);
  }
  return true;
}

bool app_debug_fill(uint16_t color) {
  if (!s_app_ctx) {
    return false;
  }

  auto& display = s_app_ctx->board.display();
  display.startWrite();
  display.fillScreen(color);
  display.endWrite();
  std::snprintf(s_app_ctx->last_event, sizeof(s_app_ctx->last_event), "display fill");
  std::snprintf(s_app_ctx->last_ui_message, sizeof(s_app_ctx->last_ui_message), "color=%u", static_cast<unsigned>(color));
  return true;
}

bool app_debug_text(const char* text) {
  if (!s_app_ctx || !text) {
    return false;
  }

  auto& display = s_app_ctx->board.display();
  display.startWrite();
  display.fillScreen(TFT_BLACK);
  display.setTextColor(TFT_GREEN, TFT_BLACK);
  display.setFont(&fonts::Font0);
  display.setTextSize(2);
  display.setCursor(8, 20);
  display.printf("DISPLAY TEST\n\n%s", text);
  display.endWrite();
  std::snprintf(s_app_ctx->last_event, sizeof(s_app_ctx->last_event), "display text");
  std::snprintf(s_app_ctx->last_ui_message,
                sizeof(s_app_ctx->last_ui_message),
                "%.*s",
                static_cast<int>(sizeof(s_app_ctx->last_ui_message) - 1),
                text);
  return true;
}

bool app_debug_set_brightness(uint8_t brightness) {
  if (!s_app_ctx) {
    return false;
  }

  s_app_ctx->board.display().setBrightness(brightness);
  std::snprintf(s_app_ctx->last_event, sizeof(s_app_ctx->last_event), "display brightness");
  std::snprintf(s_app_ctx->last_ui_message, sizeof(s_app_ctx->last_ui_message), "brightness=%u", brightness);
  return true;
}

extern "C" void app_main(void) {
  static AppContext app;
  s_app_ctx = &app;

  ESP_LOGI(TAG, "booting M5Dial web remote");
  app.input_queue = xQueueCreate(kInputQueueLength, sizeof(tutorial_0072::InputEvent));
  if (!app.input_queue) {
    ESP_LOGE(TAG, "input queue creation failed");
    return;
  }
  app.app_command_queue = xQueueCreate(kAppCommandQueueLength, sizeof(AppCommand));
  if (!app.app_command_queue) {
    ESP_LOGE(TAG, "app command queue creation failed");
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

  ESP_ERROR_CHECK(js_service_start(app.app_command_queue));
  js_service_set_remote_enabled(cfg.remote_script_enabled);

  ESP_ERROR_CHECK(remote_client_init());
  remote_client_set_app_command_queue(app.app_command_queue);
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
