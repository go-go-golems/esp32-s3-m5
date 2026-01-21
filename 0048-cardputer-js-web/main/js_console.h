#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Registers the `js` esp_console command (no REPL init; called by wifi_console_start()).
void js_console_register_commands(void);

#ifdef __cplusplus
}
#endif

