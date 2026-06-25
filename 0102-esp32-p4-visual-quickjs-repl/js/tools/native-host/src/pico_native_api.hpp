#pragma once

#include <cstdint>
#include <string>

extern "C" {
#include "quickjs.h"
}

namespace pico_native {

struct Runtime;

Runtime *runtime_create(int cols, int rows);
void runtime_destroy(Runtime *rt);
JSContext *runtime_context(Runtime *rt);

bool runtime_load_file(Runtime *rt, const std::string &path, std::string *error = nullptr);
void runtime_run_frame(Runtime *rt, int dt_ms);
void runtime_send_key(Runtime *rt, const std::string &token);
std::string runtime_render_text(Runtime *rt);

uint64_t host_millis();

}  // namespace pico_native
