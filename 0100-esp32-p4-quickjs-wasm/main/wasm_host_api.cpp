/* wasm_host_api.cpp — registers the "env" native symbols the quickjs.wasm
 * module imports (host_print / host_millis / host_gpio_write). Mirrors the
 * Phase-0 host_test.c contract exactly (signatures "($)", "()i", "(ii)"). */
#include "wasm_host_api.h"

#include <cstdio>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "wasm_export.h"

namespace {
constexpr const char *kTag = "0100_host";

void host_print(wasm_exec_env_t, const char *s)
{
    fputs(s, stdout);
    fflush(stdout);
}

int host_millis(wasm_exec_env_t)
{
    return (int)(esp_timer_get_time() / 1000);
}

void host_gpio_write(wasm_exec_env_t, int pin, int val)
{
    gpio_set_level((gpio_num_t)pin, val ? 1 : 0);
}

NativeSymbol kHostSymbols[] = {
    { "host_print",      (void *)host_print,      "($)",  nullptr },
    { "host_millis",     (void *)host_millis,     "()i",  nullptr },
    { "host_gpio_write", (void *)host_gpio_write, "(ii)", nullptr },
};
}

bool init_wasm_host_api(void)
{
    if (!wasm_runtime_register_natives("env", kHostSymbols,
                                       sizeof(kHostSymbols) / sizeof(kHostSymbols[0]))) {
        ESP_LOGE(kTag, "wasm_runtime_register_natives failed");
        return false;
    }
    ESP_LOGI(kTag, "registered %u env native symbols",
             (unsigned)(sizeof(kHostSymbols) / sizeof(kHostSymbols[0])));
    return true;
}
