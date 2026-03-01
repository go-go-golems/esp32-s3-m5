#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <string>
#include <vector>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include <M5Unified.hpp>

#include "app_state.h"
#include "chain_encoder_uart.h"
#include "http_server.h"
#include "lvgl_port_m5gfx.h"
#include "preset_store.h"
#include "screenshot_qoi.h"

#if CONFIG_PHOTO_TIMER_WIFI_ENABLE
#include "esp_netif_ip_addr.h"
#include "lwip/inet.h"
#include "wifi_console.h"
#include "wifi_mgr.h"
#endif

namespace {

static const char* TAG = "photo_timer_0071";

static ChainEncoderUart* s_enc = nullptr;
static lv_indev_t* s_enc_indev = nullptr;
static lv_group_t* s_group = nullptr;
static lv_obj_t* s_list = nullptr;
static lv_obj_t* s_header = nullptr;
static lv_obj_t* s_footer = nullptr;
static uint32_t s_seen_revision = 0;
static std::string s_seen_settings_ip_line;

enum class UiView {
  kTimer,
  kSettings,
};

static UiView s_view = UiView::kTimer;

enum class UiCommandType {
  kScreenshot,
};

struct UiCommand {
  UiCommandType type = UiCommandType::kScreenshot;
  TaskHandle_t reply_task = nullptr;
};

static QueueHandle_t s_ui_command_q = nullptr;

enum class UiActionType {
  kToggle,
  kNext,
  kReset,
  kSelectPreset,
  kOpenSettings,
  kBackToTimer,
};

struct UiActionBinding {
  lv_obj_t* btn = nullptr;
  UiActionType type = UiActionType::kToggle;
  std::string preset_id;
};

static std::vector<UiActionBinding> s_bindings;

static lv_style_t s_btn_base;
static lv_style_t s_btn_focused;
static bool s_styles_inited = false;

void attach_btn_callbacks();
void switch_view(UiView view);

const char* state_to_str(TimerRunState state) {
  switch (state) {
    case TimerRunState::kIdle:
      return "IDLE";
    case TimerRunState::kRunning:
      return "RUNNING";
    case TimerRunState::kPaused:
      return "PAUSED";
    case TimerRunState::kComplete:
      return "COMPLETE";
  }
  return "?";
}

void apply_styles(lv_obj_t* btn) {
  if (!btn) return;
  if (!s_styles_inited) {
    s_styles_inited = true;

    lv_style_init(&s_btn_base);
    lv_style_set_bg_opa(&s_btn_base, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_btn_base, 0);
    lv_style_set_radius(&s_btn_base, 6);
    lv_style_set_pad_all(&s_btn_base, 6);
    lv_style_set_text_color(&s_btn_base, lv_palette_main(LV_PALETTE_GREEN));

    lv_style_init(&s_btn_focused);
    lv_style_set_bg_opa(&s_btn_focused, LV_OPA_COVER);
    lv_style_set_bg_color(&s_btn_focused, lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_text_color(&s_btn_focused, lv_color_black());
  }

  lv_obj_add_style(btn,
                   &s_btn_base,
                   (lv_style_selector_t)((lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT));
  lv_obj_add_style(btn,
                   &s_btn_focused,
                   (lv_style_selector_t)((lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_FOCUSED));
}

void add_binding(lv_obj_t* btn, UiActionType type, const std::string& preset_id = {}) {
  UiActionBinding b;
  b.btn = btn;
  b.type = type;
  b.preset_id = preset_id;
  s_bindings.push_back(std::move(b));
}

std::string settings_ip_line() {
#if CONFIG_PHOTO_TIMER_WIFI_ENABLE
  wifi_mgr_status_t st = {};
  if (wifi_mgr_get_status(&st) != ESP_OK) {
    return "Wi-Fi IP: unavailable";
  }
  if (st.state != WIFI_MGR_STATE_CONNECTED || st.ip4 == 0) {
    return "Wi-Fi IP: disconnected";
  }
  ip4_addr_t ip = {.addr = htonl(st.ip4)};
  char line[64];
  snprintf(line, sizeof(line), "Wi-Fi IP: " IPSTR, IP2STR(&ip));
  return std::string(line);
#else
  return "Wi-Fi disabled";
#endif
}

void update_label_for_width(lv_obj_t* obj) {
  if (!obj) return;
  lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(obj, lv_pct(100));
}

void rebuild_preset_list() {
  if (!s_list || !s_group) return;

  lv_obj_clean(s_list);
  s_bindings.clear();

  const TimerConfig cfg = app_state_config_copy();

  lv_obj_t* first_focus = nullptr;

  auto add_button = [&](const char* label, UiActionType type, const std::string& preset_id) {
    lv_obj_t* btn = lv_list_add_btn(s_list, nullptr, label);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_t* label_obj = lv_obj_get_child(btn, 0);
    if (label_obj) {
      update_label_for_width(label_obj);
    }
    apply_styles(btn);
    lv_group_add_obj(s_group, btn);
    add_binding(btn, type, preset_id);
    if (!first_focus) first_focus = btn;
  };

  if (s_view == UiView::kSettings) {
    const std::string ip_line = settings_ip_line();
    s_seen_settings_ip_line = ip_line;
    lv_obj_t* line = lv_list_add_text(s_list, ip_line.c_str());
    update_label_for_width(line);
    add_button("Back", UiActionType::kBackToTimer, "");
  } else {
    add_button("Start/Pause", UiActionType::kToggle, "");
    add_button("Next Step", UiActionType::kNext, "");
    add_button("Reset", UiActionType::kReset, "");
    add_button("Settings", UiActionType::kOpenSettings, "");

    for (const auto& preset : cfg.presets) {
      std::string label = (preset.id == cfg.active_preset_id) ? "* " : "  ";
      label += preset.name;
      add_button(label.c_str(), UiActionType::kSelectPreset, preset.id);
    }
  }

  if (first_focus) {
    lv_group_focus_obj(first_focus);
  }
}

void on_action(UiActionType type, const char* preset_id) {
  switch (type) {
    case UiActionType::kToggle:
      (void)app_state_timer_action("toggle", nullptr);
      break;
    case UiActionType::kNext:
      (void)app_state_timer_action("next", nullptr);
      break;
    case UiActionType::kReset:
      (void)app_state_timer_action("reset", nullptr);
      break;
    case UiActionType::kSelectPreset:
      if (preset_id) {
        (void)app_state_timer_action("select", preset_id);
      }
      break;
    case UiActionType::kOpenSettings:
      switch_view(UiView::kSettings);
      break;
    case UiActionType::kBackToTimer:
      switch_view(UiView::kTimer);
      break;
  }
}

void switch_view(UiView view) {
  if (s_view == view) return;
  s_view = view;
  rebuild_preset_list();
  attach_btn_callbacks();
}

void list_btn_cb(lv_event_t* e) {
  lv_obj_t* btn = lv_event_get_target(e);
  for (const auto& b : s_bindings) {
    if (b.btn == btn) {
      on_action(b.type, b.preset_id.empty() ? nullptr : b.preset_id.c_str());
      break;
    }
  }
}

void attach_btn_callbacks() {
  for (const auto& b : s_bindings) {
    lv_obj_add_event_cb(b.btn, list_btn_cb, LV_EVENT_CLICKED, nullptr);
  }
}

void refresh_status() {
  const TimerSnapshot snap = app_state_snapshot();

  char header[256];
  if (s_view == UiView::kSettings) {
    snprintf(header, sizeof(header), "Settings");
  } else {
    snprintf(header,
             sizeof(header),
             "Preset: %s  State: %s  Step: %d/%d",
             snap.preset_name.empty() ? "(none)" : snap.preset_name.c_str(),
             state_to_str(snap.state),
             (snap.step_index >= 0) ? (snap.step_index + 1) : 0,
             std::max(0, snap.step_count));
  }

  char footer[256];
  const uint32_t remain = snap.step_remaining_sec;
  const uint32_t mm = remain / 60U;
  const uint32_t ss = remain % 60U;
  if (s_view == UiView::kSettings) {
    snprintf(footer, sizeof(footer), "Use encoder to return");
  } else {
    snprintf(footer,
             sizeof(footer),
             "%s  remaining %02" PRIu32 ":%02" PRIu32,
             snap.step_name.empty() ? "(no step)" : snap.step_name.c_str(),
             mm,
             ss);
  }

  if (s_header) {
    lv_label_set_text(s_header, header);
  }
  if (s_footer) {
    lv_label_set_text(s_footer, footer);
  }

  const uint32_t revision = app_state_config_revision();
  if (revision != s_seen_revision) {
    s_seen_revision = revision;
    rebuild_preset_list();
    attach_btn_callbacks();
  }

  if (s_view == UiView::kSettings) {
    const std::string ip_line = settings_ip_line();
    if (ip_line != s_seen_settings_ip_line) {
      s_seen_settings_ip_line = ip_line;
      rebuild_preset_list();
      attach_btn_callbacks();
    }
  }
}

void status_timer_cb(lv_timer_t* t) {
  (void)t;
  refresh_status();
}

void enc_read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  (void)drv;

  int32_t d = 0;
  if (s_enc) {
    d = s_enc->take_delta();
  }

  const int clamp = CONFIG_PHOTO_TIMER_ENCODER_DELTA_CLAMP;
  if (clamp > 0) {
    d = std::clamp(d, (int32_t)-clamp, (int32_t)clamp);
  }
  data->enc_diff = (int16_t)std::clamp(d, (int32_t)-32768, (int32_t)32767);

  static bool pulse_pressed = false;
  if (s_enc && s_enc->has_click_pending()) {
    if (!pulse_pressed) {
      pulse_pressed = true;
      data->state = LV_INDEV_STATE_PR;
      return;
    }

    pulse_pressed = false;
    s_enc->clear_click_pending();
    data->state = LV_INDEV_STATE_REL;
    return;
  }

  data->state = LV_INDEV_STATE_REL;
}

lv_indev_t* lvgl_port_chain_encoder_init() {
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_ENCODER;
  indev_drv.read_cb = enc_read_cb;
  return lv_indev_drv_register(&indev_drv);
}

void create_ui() {
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_text_color(scr, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

  s_header = lv_label_create(scr);
  lv_label_set_text(s_header, "Photo Timer 0071");
  lv_obj_set_style_text_font(s_header, &lv_font_montserrat_14, 0);
  update_label_for_width(s_header);
  lv_obj_align(s_header, LV_ALIGN_TOP_LEFT, 4, 2);

  s_list = lv_list_create(scr);
  lv_obj_set_size(s_list, 240 - 12, 135 - 42);
  lv_obj_align(s_list, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_set_style_bg_opa(s_list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(s_list, 0, 0);
  lv_obj_set_scroll_dir(s_list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(s_list, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_pad_right(s_list, 2, LV_PART_MAIN);
  lv_obj_set_style_pad_right(s_list, 2, LV_PART_SCROLLBAR);

  s_group = lv_group_create();
  lv_group_set_wrap(s_group, true);
  if (s_enc_indev) {
    lv_indev_set_group(s_enc_indev, s_group);
  }

  s_footer = lv_label_create(scr);
  lv_obj_set_style_text_font(s_footer, &lv_font_montserrat_14, 0);
  lv_label_set_text(s_footer, "loading...");
  update_label_for_width(s_footer);
  lv_obj_align(s_footer, LV_ALIGN_BOTTOM_LEFT, 4, -2);

  s_seen_revision = app_state_config_revision();
  rebuild_preset_list();
  attach_btn_callbacks();
  refresh_status();

  const uint32_t period_ms = (uint32_t)CONFIG_PHOTO_TIMER_STATUS_REFRESH_MS;
  lv_timer_create(status_timer_cb, period_ms > 0 ? period_ms : 200, nullptr);
}

#if CONFIG_PHOTO_TIMER_WIFI_ENABLE
void on_wifi_got_ip(uint32_t ip4_host_order, void* ctx) {
  (void)ip4_host_order;
  (void)ctx;
  (void)photo_http_server_start();
}
#endif

int cmd_screenshot(int argc, char** argv) {
  (void)argc;
  (void)argv;
  if (!s_ui_command_q) {
    printf("ERR: screenshot queue unavailable\n");
    return 1;
  }

  UiCommand cmd = {};
  cmd.type = UiCommandType::kScreenshot;
  cmd.reply_task = xTaskGetCurrentTaskHandle();
  if (xQueueSend(s_ui_command_q, &cmd, pdMS_TO_TICKS(200)) != pdTRUE) {
    printf("ERR: screenshot busy (queue full)\n");
    return 1;
  }

  uint32_t result = 0;
  if (xTaskNotifyWait(0, UINT32_MAX, &result, pdMS_TO_TICKS(20000)) != pdTRUE) {
    printf("ERR: screenshot timeout\n");
    return 1;
  }
  if (result == 0) {
    printf("ERR: screenshot failed\n");
    return 1;
  }

  printf("OK: screenshot %u bytes\n", (unsigned)result);
  return 0;
}

int cmd_ui(int argc, char** argv) {
  if (argc < 2 || !argv[1]) {
    printf("usage: ui <timer|settings>\n");
    return 1;
  }
  if (strcmp(argv[1], "timer") == 0) {
    switch_view(UiView::kTimer);
    printf("OK: ui timer\n");
    return 0;
  }
  if (strcmp(argv[1], "settings") == 0) {
    switch_view(UiView::kSettings);
    printf("OK: ui settings\n");
    return 0;
  }
  printf("ERR: unknown ui view '%s'\n", argv[1]);
  return 1;
}

void register_console_commands() {
  static bool s_registered = false;
  if (s_registered) return;
  s_registered = true;

  esp_console_cmd_t cmd = {};
  cmd.command = "screenshot";
  cmd.help = "Capture current UI and stream QOI over USB-Serial/JTAG";
  cmd.hint = nullptr;
  cmd.func = &cmd_screenshot;
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

  cmd = {};
  cmd.command = "ui";
  cmd.help = "Switch UI view (ui timer|settings)";
  cmd.hint = nullptr;
  cmd.func = &cmd_ui;
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void drain_ui_commands(m5gfx::M5GFX& display) {
  if (!s_ui_command_q) return;

  UiCommand cmd = {};
  while (xQueueReceive(s_ui_command_q, &cmd, 0) == pdTRUE) {
    uint32_t notify_value = 0;
    switch (cmd.type) {
      case UiCommandType::kScreenshot: {
        lv_timer_handler();
        size_t qoi_len = 0;
        const bool ok = screenshot_qoi_to_usb_serial_jtag_ex(display, &qoi_len);
        notify_value = ok ? (uint32_t)qoi_len : 0U;
      } break;
    }
    if (cmd.reply_task) {
      (void)xTaskNotify(cmd.reply_task, notify_value, eSetValueWithOverwrite);
    }
  }
}

}  // namespace

extern "C" void app_main(void) {
  ESP_LOGI(TAG,
           "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32,
           esp_get_free_heap_size(),
           (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

  M5.begin();
  M5.Display.setBrightness(255);
  M5.Display.setColorDepth(16);
  auto& display = M5.Display;

  LvglM5gfxConfig lv_cfg{};
  lv_cfg.buffer_lines = 40;
  lv_cfg.double_buffer = true;
  lv_cfg.swap_bytes = false;
  lv_cfg.tick_ms = 2;
  if (!lvgl_port_m5gfx_init(display, lv_cfg)) {
    ESP_LOGE(TAG, "lvgl_port_m5gfx_init failed");
    return;
  }

  const esp_err_t ps_init = preset_store_init((bool)CONFIG_PHOTO_TIMER_SPIFFS_FORMAT_IF_MOUNT_FAILED);
  if (ps_init != ESP_OK) {
    ESP_LOGE(TAG, "preset_store_init failed: %s", esp_err_to_name(ps_init));
    return;
  }

  TimerConfig cfg;
  const esp_err_t ps_load = preset_store_load_or_seed(&cfg);
  if (ps_load != ESP_OK) {
    ESP_LOGE(TAG, "preset_store_load_or_seed failed: %s", esp_err_to_name(ps_load));
    return;
  }

  if (app_state_init(&cfg) != ESP_OK) {
    ESP_LOGE(TAG, "app_state_init failed");
    return;
  }

  ChainEncoderUart::Config enc_cfg;
  enc_cfg.uart_num = CONFIG_PHOTO_TIMER_ENCODER_UART_NUM;
  enc_cfg.baud = CONFIG_PHOTO_TIMER_ENCODER_BAUD;
  enc_cfg.tx_gpio = CONFIG_PHOTO_TIMER_ENCODER_TX_GPIO;
  enc_cfg.rx_gpio = CONFIG_PHOTO_TIMER_ENCODER_RX_GPIO;
  enc_cfg.index_id = (uint8_t)CONFIG_PHOTO_TIMER_ENCODER_INDEX_ID;
  enc_cfg.poll_ms = CONFIG_PHOTO_TIMER_ENCODER_POLL_MS;

  static ChainEncoderUart enc(enc_cfg);
  s_enc = &enc;
  if (!enc.init()) {
    ESP_LOGW(TAG, "chain encoder init failed; UI renders but input is unavailable");
  }

  s_enc_indev = lvgl_port_chain_encoder_init();
  create_ui();
  s_ui_command_q = xQueueCreate(4, sizeof(UiCommand));
  if (!s_ui_command_q) {
    ESP_LOGE(TAG, "failed to create UI command queue");
    return;
  }

#if CONFIG_PHOTO_TIMER_WIFI_ENABLE
  wifi_mgr_set_on_got_ip_cb(on_wifi_got_ip, nullptr);
  ESP_ERROR_CHECK(wifi_mgr_start());

  wifi_console_config_t wifi_console_cfg = {};
  wifi_console_cfg.prompt = "timer> ";
  wifi_console_cfg.register_extra = register_console_commands;
  wifi_console_start(&wifi_console_cfg);

  ESP_LOGI(TAG, "Wi-Fi console started; connect then browse / to open the preset UI");
#endif

  while (true) {
    drain_ui_commands(display);
    app_state_tick();
    lv_timer_handler();
    vTaskDelay(1);
  }
}
