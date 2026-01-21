#include "js_console.h"

#include <stdio.h>
#include <string.h>

#include <string>

#include "esp_console.h"
#include "esp_err.h"

#include "js_runner.h"

namespace {

static void print_usage(void) {
  printf("usage:\n");
  printf("  js eval <code...>\n");
  printf("examples:\n");
  printf("  js eval 1+1\n");
  printf("  js eval \"let x=1; x+2\"\n");
}

static int cmd_js(int argc, char** argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  if (strcmp(argv[1], "eval") == 0) {
    if (argc < 3) {
      printf("js eval: missing code\n");
      return 1;
    }

    std::string code;
    for (int i = 2; i < argc; i++) {
      if (!code.empty()) code.push_back(' ');
      code += argv[i];
    }

    const std::string json = js_runner_eval_to_json(code.c_str(), code.size());
    printf("%s\n", json.c_str());
    return 0;
  }

  print_usage();
  return 1;
}

} // namespace

extern "C" void js_console_register_commands(void) {
  esp_console_cmd_t cmd = {};
  cmd.command = "js";
  cmd.help = "JavaScript: js eval <code...>";
  cmd.func = &cmd_js;
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

