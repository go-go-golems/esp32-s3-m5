// Copied from imports MicroQuickJS REPL as the initial stdlib wiring strategy.
// Source: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c
//
// NOTE: This file includes the generated stdlib (`esp32_stdlib.h`) at the end,
// which defines `const JSSTDLibraryDef js_stdlib`.
//
// For now we accept the broader stdlib surface; we can replace with a minimal
// stdlib later if bundle size or dependencies become problematic.

#include "../../imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c"

