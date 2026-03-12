#include "display_console.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "esp_console.h"
#include "esp_err.h"

#include "app_debug.h"
#include "m5dial_board.h"

namespace {

void print_usage() {
  std::printf("usage:\n");
  std::printf("  display status\n");
  std::printf("  display mode <debug|radio>\n");
  std::printf("  display redraw\n");
  std::printf("  display fill <black|white|red|green|blue>\n");
  std::printf("  display text <message...>\n");
  std::printf("  display brightness <0-255>\n");
}

bool parse_color(const char* name, uint16_t* out) {
  if (!name || !out) {
    return false;
  }
  if (std::strcmp(name, "black") == 0) {
    *out = TFT_BLACK;
    return true;
  }
  if (std::strcmp(name, "white") == 0) {
    *out = TFT_WHITE;
    return true;
  }
  if (std::strcmp(name, "red") == 0) {
    *out = TFT_RED;
    return true;
  }
  if (std::strcmp(name, "green") == 0) {
    *out = TFT_GREEN;
    return true;
  }
  if (std::strcmp(name, "blue") == 0) {
    *out = TFT_BLUE;
    return true;
  }
  return false;
}

int cmd_display(int argc, char** argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  if (std::strcmp(argv[1], "status") == 0) {
    AppDebugStatus status = {};
    if (!app_debug_get_status(&status)) {
      std::printf("display status: app not ready\n");
      return 1;
    }
    std::printf("ready=%s mode=%s touch=%s pos=%" PRId32 " radio_pos=%" PRId32 "\n",
                status.ready ? "yes" : "no",
                status.radio_mode ? "radio" : "debug",
                status.touch_ready ? "yes" : "no",
                status.position,
                status.radio_position);
    std::printf("last_event=%s\n", status.last_event[0] ? status.last_event : "-");
    std::printf("last_message=%s\n", status.last_message[0] ? status.last_message : "-");
    return 0;
  }

  if (std::strcmp(argv[1], "mode") == 0) {
    if (argc < 3) {
      print_usage();
      return 1;
    }
    const bool radio = std::strcmp(argv[2], "radio") == 0;
    if (!radio && std::strcmp(argv[2], "debug") != 0) {
      print_usage();
      return 1;
    }
    if (!app_debug_set_mode(radio)) {
      std::printf("display mode: app not ready\n");
      return 1;
    }
    std::printf("display mode=%s\n", radio ? "radio" : "debug");
    return 0;
  }

  if (std::strcmp(argv[1], "redraw") == 0) {
    if (!app_debug_redraw()) {
      std::printf("display redraw: app not ready\n");
      return 1;
    }
    std::printf("display redraw requested\n");
    return 0;
  }

  if (std::strcmp(argv[1], "fill") == 0) {
    if (argc < 3) {
      print_usage();
      return 1;
    }
    uint16_t color = 0;
    if (!parse_color(argv[2], &color)) {
      std::printf("display fill: unknown color %s\n", argv[2]);
      return 1;
    }
    if (!app_debug_fill(color)) {
      std::printf("display fill: app not ready\n");
      return 1;
    }
    std::printf("display filled %s\n", argv[2]);
    return 0;
  }

  if (std::strcmp(argv[1], "text") == 0) {
    if (argc < 3) {
      print_usage();
      return 1;
    }
    std::string text;
    for (int i = 2; i < argc; ++i) {
      if (!text.empty()) {
        text.push_back(' ');
      }
      text += argv[i];
    }
    if (!app_debug_text(text.c_str())) {
      std::printf("display text: app not ready\n");
      return 1;
    }
    std::printf("display text rendered\n");
    return 0;
  }

  if (std::strcmp(argv[1], "brightness") == 0) {
    if (argc < 3) {
      print_usage();
      return 1;
    }
    const long value = std::strtol(argv[2], nullptr, 10);
    if (value < 0 || value > 255) {
      std::printf("display brightness: expected 0-255\n");
      return 1;
    }
    if (!app_debug_set_brightness(static_cast<uint8_t>(value))) {
      std::printf("display brightness: app not ready\n");
      return 1;
    }
    std::printf("display brightness=%ld\n", value);
    return 0;
  }

  print_usage();
  return 1;
}

}  // namespace

extern "C" void display_console_register_commands(void) {
  esp_console_cmd_t cmd = {};
  cmd.command = "display";
  cmd.help = "Display diagnostics: status|mode|redraw|fill|text|brightness";
  cmd.func = &cmd_display;
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
