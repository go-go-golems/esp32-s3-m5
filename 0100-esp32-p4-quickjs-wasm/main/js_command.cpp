/* js_command.cpp — `js eval <source>` and `js status` console commands.
 * `js eval` joins the remaining args with spaces so both
 *   js eval "print(1+2)"   and   js eval print(1 + 2)
 * work. */
#include "js_command.h"

#include <cstdio>
#include <cstring>

#include "esp_console.h"
#include "esp_log.h"

#include "wasm_runner.h"
#include "wasm_runtime_service.h"

namespace {
constexpr const char *kTag = "0100_js";
constexpr size_t kMaxSrc = 1024;

int cmd_js(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: js eval <source> | js status\n");
        return 0;
    }

    if (strcmp(argv[1], "status") == 0) {
        print_wasm_runtime_status();
        return 0;
    }

    if (strcmp(argv[1], "eval") != 0) {
        printf("unknown subcommand: %s (use 'eval' or 'status')\n", argv[1]);
        return 1;
    }

    if (argc < 3) {
        printf("usage: js eval <source>\n");
        return 1;
    }

    char buf[kMaxSrc];
    size_t n = 0;
    for (int i = 2; i < argc; i++) {
        size_t l = strlen(argv[i]);
        if (n + l + (i > 2 ? 1 : 0) + 1 > sizeof(buf)) {
            printf("source too long (max %zu)\n", sizeof(buf) - 1);
            return 1;
        }
        if (i > 2) buf[n++] = ' ';
        memcpy(buf + n, argv[i], l);
        n += l;
    }
    buf[n] = '\0';

    int r = wasm_runner_eval(buf, n);
    return (r == 0) ? 0 : 1;
}
}  // namespace

void register_js_commands(void)
{
    esp_console_cmd_t js_cmd = {};
    js_cmd.command = "js";
    js_cmd.help = "QuickJS-WASM: js eval <source> | js status";
    js_cmd.func = &cmd_js;
    ESP_ERROR_CHECK(esp_console_cmd_register(&js_cmd));
}
