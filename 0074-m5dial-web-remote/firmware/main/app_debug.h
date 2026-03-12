#pragma once

#include <stdint.h>

struct AppDebugStatus {
  bool ready = false;
  bool radio_mode = false;
  bool touch_ready = false;
  int32_t position = 0;
  int32_t radio_position = 0;
  char last_event[48] = {0};
  char last_message[64] = {0};
};

bool app_debug_get_status(AppDebugStatus* out);
bool app_debug_set_mode(bool radio_mode);
bool app_debug_redraw();
bool app_debug_fill(uint16_t color);
bool app_debug_text(const char* text);
bool app_debug_set_brightness(uint8_t brightness);
