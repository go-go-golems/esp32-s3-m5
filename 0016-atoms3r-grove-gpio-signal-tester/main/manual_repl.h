// Tutorial 0016: minimal manual REPL (no esp_console) over USB Serial/JTAG.
#pragma once

// Starts a background task that reads lines and maps commands to CtrlEvent (via ctrl_send()).
void manual_repl_start(void);


