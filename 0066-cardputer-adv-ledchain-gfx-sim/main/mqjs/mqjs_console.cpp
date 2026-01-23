#include "mqjs_console.h"

#include <stdio.h>
#include <string.h>

#include <string>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"

#include "mqjs_engine.h"

static const char *TAG = "0066_mqjs_console";

static MqjsEngine s_js;

static void print_usage(void) {
  printf("js: MicroQuickJS REPL + sim control API\n");
  printf("\n");
  printf("usage:\n");
  printf("  js help\n");
  printf("  js eval <code...>\n");
  printf("  js repl\n");
  printf("  js reset\n");
  printf("  js stats\n");
  printf("\n");
  printf("sim API (in JS):\n");
  printf("  sim.status()\n");
  printf("  sim.setFrameMs(ms)\n");
  printf("  sim.setBrightness(pct)\n");
  printf("  sim.setPattern('rainbow'|'chase'|'breathing'|'sparkle'|'off')\n");
  printf("  sim.setRainbow(speed, sat, spread)\n");
  printf("  sim.setChase(speed, tail, gap, trains, '#RRGGBB', '#RRGGBB', dir, fade, briPct)\n");
  printf("  sim.setBreathing(speed, '#RRGGBB', min, max, curve)\n");
  printf("  sim.setSparkle(speed, '#RRGGBB', density, fade, mode, '#RRGGBB')\n");
  printf("\n");
  printf("REPL commands inside `js repl`:\n");
  printf("  .exit   leave JS REPL\n");
  printf("\n");
}

static std::string join_args(int start, int argc, char **argv) {
  std::string out;
  for (int i = start; i < argc; i++) {
    if (i != start) out.push_back(' ');
    out.append(argv[i] ? argv[i] : "");
  }
  return out;
}

static int cmd_js(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  if (strcmp(argv[1], "help") == 0) {
    print_usage();
    return 0;
  }

  if (strcmp(argv[1], "reset") == 0) {
    std::string error;
    if (!s_js.Reset(&error)) {
      printf("error: %s\n", error.c_str());
      return 1;
    }
    printf("ok\n");
    return 0;
  }

  if (strcmp(argv[1], "stats") == 0) {
    std::string out;
    if (!s_js.DumpMemory(&out)) {
      printf("error: stats failed\n");
      return 1;
    }
    printf("%s", out.c_str());
    return 0;
  }

  if (strcmp(argv[1], "eval") == 0) {
    if (argc < 3) {
      printf("js eval: missing code\n");
      return 1;
    }
    const std::string code = join_args(2, argc, argv);
    std::string out;
    std::string err;
    (void)s_js.Eval(code, &out, &err);
    if (!err.empty()) {
      printf("error: %s", err.c_str());
      return 1;
    }
    if (!out.empty()) {
      printf("%s", out.c_str());
    }
    return 0;
  }

  if (strcmp(argv[1], "repl") == 0) {
    printf("entering JS REPL; type .exit to return to sim> prompt\n");

    char line[512];
    for (;;) {
      printf("%s", s_js.Prompt());
      fflush(stdout);

      if (!fgets(line, sizeof(line), stdin)) {
        printf("\n");
        return 0;
      }

      // Trim \r\n.
      size_t n = strlen(line);
      while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) {
        line[--n] = '\0';
      }

      if (strcmp(line, ".exit") == 0 || strcmp(line, ":exit") == 0 || strcmp(line, ":quit") == 0) {
        return 0;
      }
      if (n == 0) continue;

      std::string out;
      std::string err;
      (void)s_js.Eval(std::string(line, n), &out, &err);
      if (!err.empty()) {
        printf("error: %s", err.c_str());
        continue;
      }
      if (!out.empty()) {
        printf("%s", out.c_str());
      }
    }
  }

  print_usage();
  return 1;
}

void mqjs_console_register_commands(sim_engine_t *engine) {
  std::string error;
  if (!s_js.Init(engine, &error)) {
    ESP_LOGW(TAG, "JS engine not started: %s", error.c_str());
    return;
  }

  esp_console_cmd_t cmd = {};
  cmd.command = "js";
  cmd.help = "MicroQuickJS REPL + simulator API (try: js repl, js eval 'sim.status()')";
  cmd.func = &cmd_js;
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
