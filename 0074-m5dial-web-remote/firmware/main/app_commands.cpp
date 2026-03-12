#include "app_commands.h"

#include <cstdio>
#include <cstring>

namespace {

AppCommandType command_type_from_name(const char* command) {
  if (!command || command[0] == '\0') {
    return AppCommandType::kUnknown;
  }
  if (std::strcmp(command, "show_message") == 0) {
    return AppCommandType::kShowMessage;
  }
  if (std::strcmp(command, "set_position") == 0) {
    return AppCommandType::kSetPosition;
  }
  if (std::strcmp(command, "set_mode") == 0) {
    return AppCommandType::kSetMode;
  }
  if (std::strcmp(command, "set_station") == 0) {
    return AppCommandType::kSetStation;
  }
  if (std::strcmp(command, "set_band") == 0) {
    return AppCommandType::kSetBand;
  }
  if (std::strcmp(command, "show_reveal") == 0) {
    return AppCommandType::kShowReveal;
  }
  return AppCommandType::kUnknown;
}

}  // namespace

const char* app_command_type_name(AppCommandType type) {
  switch (type) {
    case AppCommandType::kShowMessage:
      return "show_message";
    case AppCommandType::kSetPosition:
      return "set_position";
    case AppCommandType::kSetMode:
      return "set_mode";
    case AppCommandType::kSetStation:
      return "set_station";
    case AppCommandType::kSetBand:
      return "set_band";
    case AppCommandType::kShowReveal:
      return "show_reveal";
    case AppCommandType::kUnknown:
      break;
  }
  return "unknown";
}

const char* app_command_source_name(AppCommandSource source) {
  switch (source) {
    case AppCommandSource::kUi:
      return "ui";
    case AppCommandSource::kJs:
      return "js";
    case AppCommandSource::kUnknown:
      break;
  }
  return "unknown";
}

void app_command_init(AppCommand* out,
                      AppCommandType type,
                      AppCommandSource source,
                      uint32_t request_id,
                      const char* command,
                      int32_t value,
                      const char* text) {
  if (!out) {
    return;
  }
  *out = {};
  out->type = type;
  out->source = source;
  out->request_id = request_id;
  out->value = value;
  std::snprintf(out->command, sizeof(out->command), "%s", command ? command : app_command_type_name(type));
  if (text) {
    std::snprintf(out->text, sizeof(out->text), "%s", text);
  }
}

bool app_command_parse_ui_message(const cJSON* root, AppCommand* out) {
  if (!root || !out) {
    return false;
  }

  const cJSON* type = cJSON_GetObjectItemCaseSensitive(root, "type");
  if (!cJSON_IsString(type) || std::strcmp(type->valuestring, "ui_command") != 0) {
    return false;
  }

  AppCommand cmd = {};
  cmd.source = AppCommandSource::kUi;

  const cJSON* request_id = cJSON_GetObjectItemCaseSensitive(root, "request_id");
  if (cJSON_IsNumber(request_id) && request_id->valuedouble >= 0) {
    cmd.request_id = static_cast<uint32_t>(request_id->valuedouble);
  }

  const cJSON* command = cJSON_GetObjectItemCaseSensitive(root, "command");
  if (cJSON_IsString(command)) {
    std::snprintf(cmd.command, sizeof(cmd.command), "%s", command->valuestring);
    cmd.type = command_type_from_name(command->valuestring);
  }

  const cJSON* text = cJSON_GetObjectItemCaseSensitive(root, "text");
  if (cJSON_IsString(text)) {
    std::snprintf(cmd.text, sizeof(cmd.text), "%s", text->valuestring);
  }

  const cJSON* value = cJSON_GetObjectItemCaseSensitive(root, "value");
  if (cJSON_IsNumber(value)) {
    cmd.value = value->valueint;
  }

  if (cmd.type == AppCommandType::kUnknown) {
    return false;
  }

  *out = cmd;
  return true;
}

bool app_command_parse_js_batch_item(const cJSON* root, uint32_t request_id, AppCommand* out) {
  if (!root || !out) {
    return false;
  }

  const cJSON* command = cJSON_GetObjectItemCaseSensitive(root, "command");
  if (!cJSON_IsString(command)) {
    return false;
  }

  AppCommand cmd = {};
  cmd.source = AppCommandSource::kJs;
  cmd.request_id = request_id;
  std::snprintf(cmd.command, sizeof(cmd.command), "%s", command->valuestring);
  cmd.type = command_type_from_name(command->valuestring);
  if (cmd.type == AppCommandType::kUnknown) {
    return false;
  }

  const cJSON* text = cJSON_GetObjectItemCaseSensitive(root, "text");
  if (cJSON_IsString(text)) {
    std::snprintf(cmd.text, sizeof(cmd.text), "%s", text->valuestring);
  }

  const cJSON* value = cJSON_GetObjectItemCaseSensitive(root, "value");
  if (cJSON_IsNumber(value)) {
    cmd.value = value->valueint;
  }

  *out = cmd;
  return true;
}
