#pragma once

#include <stddef.h>
#include <string>

#include "sim_engine.h"

struct JSContext;

class MqjsEngine {
 public:
  bool Init(sim_engine_t *engine, std::string *error);
  bool Reset(std::string *error);

  bool Eval(const std::string &code, std::string *out, std::string *error);
  bool DumpMemory(std::string *out);

  const char *Prompt() const { return "js> "; }

 private:
  bool EnsureContext(std::string *error);

  JSContext *ctx_ = nullptr;
  static constexpr size_t kJsMemSize = 64 * 1024;
  static uint8_t js_mem_buf_[kJsMemSize];
};

