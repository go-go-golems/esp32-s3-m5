#include "js_console.h"

#include <cstdio>
#include <cstring>
#include <string>

#include "esp_console.h"
#include "esp_err.h"

#include "js_service.h"

namespace {

void print_usage() {
  std::printf("usage:\n");
  std::printf("  js eval <code...>\n");
  std::printf("examples:\n");
  std::printf("  js eval 1+1\n");
  std::printf("  js eval \"lain.message('hi'); 42\"\n");
}

int cmd_js(int argc, char** argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  if (std::strcmp(argv[1], "eval") != 0) {
    print_usage();
    return 1;
  }

  if (argc < 3) {
    std::printf("js eval: missing code\n");
    return 1;
  }

  std::string code;
  for (int i = 2; i < argc; ++i) {
    if (!code.empty()) {
      code.push_back(' ');
    }
    code += argv[i];
  }

  const std::string result = js_service_eval_to_json(code.c_str(), code.size(), 0, "<console>");
  std::printf("%s\n", result.c_str());
  return 0;
}

}  // namespace

extern "C" void js_console_register_commands(void) {
  esp_console_cmd_t cmd = {};
  cmd.command = "js";
  cmd.help = "JavaScript: js eval <code...>";
  cmd.func = &cmd_js;
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
