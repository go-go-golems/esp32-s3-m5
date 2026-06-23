// 0100 — ESP32-P4 QuickJS-WASM scaffold app_main.
// Buildable now: console + banner + a `js status` stub.
// The intern replaces this with the full WAMR wiring per design §7.
#include <cstdio>
#include "esp_console.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"

static const char *kTag = "0100_qjs";

static int cmd_js(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: js <eval|repl|reset|status>\n");
        return 0;
    }
    if (strcmp(argv[1], "status") == 0) {
        printf("runtime=stub-not-wired-yet\n");
        printf("hint: port wasm_runtime_service + wasm_host_api from 0079, see design §7\n");
        return 0;
    }
    printf("js %s: not implemented yet (scaffold). See design doc §5.8.\n", argv[1]);
    return 1;
}

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "0100 ESP32-P4 QuickJS-WASM scaffold (ticket ESP32-P4-QUICKJS-WASM)");
    ESP_LOGI(kTag, "Read the design doc: design/01-quickjs-wasm-esp32p4-...-guide.md");

    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    cfg.prompt = "0100>";
    esp_console_new_repl_uart(&cfg, &repl);

    esp_console_cmd_t js_cmd = {};
    js_cmd.command = "js";
    js_cmd.help = "js eval|repl|reset|status (QuickJS-WASM)";
    js_cmd.func = cmd_js;
    esp_console_cmd_register(&js_cmd);

    esp_console_start_repl(repl);
}
