// USB Serial/JTAG console (Phase 1).
//
// Console handlers run in the esp_console REPL task. They are pure producers:
// every command posts an AppEvent and waits for a bounded reply. Nothing in
// this module reads or writes owner state directly.
#pragma once

namespace reader {

void ConsoleStart();

}  // namespace reader
