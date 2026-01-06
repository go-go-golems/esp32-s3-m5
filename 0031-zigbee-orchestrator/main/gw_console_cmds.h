#pragma once

// Registers app-specific console commands. Must be called after esp_console init
// but before starting the REPL.
void gw_console_cmds_init(void);

