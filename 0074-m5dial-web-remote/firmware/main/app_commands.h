#pragma once

#include <stdint.h>

#include "cJSON.h"

enum class AppCommandType : uint8_t {
  kUnknown = 0,
  kShowMessage,
  kSetPosition,
  kSetMode,
  kSetStation,
  kSetBand,
  kShowReveal,
};

enum class AppCommandSource : uint8_t {
  kUnknown = 0,
  kUi,
  kJs,
};

struct AppCommand {
  AppCommandType type = AppCommandType::kUnknown;
  AppCommandSource source = AppCommandSource::kUnknown;
  uint32_t request_id = 0;
  char command[24] = {0};
  char text[96] = {0};
  int32_t value = 0;
};

const char* app_command_type_name(AppCommandType type);
const char* app_command_source_name(AppCommandSource source);
bool app_command_parse_ui_message(const cJSON* root, AppCommand* out);
bool app_command_parse_js_batch_item(const cJSON* root, uint32_t request_id, AppCommand* out);
void app_command_init(AppCommand* out,
                      AppCommandType type,
                      AppCommandSource source,
                      uint32_t request_id,
                      const char* command,
                      int32_t value,
                      const char* text);
