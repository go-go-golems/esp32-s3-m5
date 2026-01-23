#include "mqjs_engine.h"

#include <cstring>

extern "C" {
#include "mquickjs.h"
}

// Provided by `mqjs/esp32_stdlib_runtime.c` (which also defines `js_stdlib`).
extern "C" void mqjs_sim_set_engine(sim_engine_t *engine);
extern "C" const JSSTDLibraryDef js_stdlib;

uint8_t MqjsEngine::js_mem_buf_[MqjsEngine::kJsMemSize];

namespace {

void write_to_string(void *opaque, const void *buf, size_t buf_len) {
  if (!opaque || !buf || buf_len == 0) return;
  auto *out = static_cast<std::string *>(opaque);
  out->append(static_cast<const char *>(buf), buf_len);
}

std::string print_value(JSContext *ctx, JSValue v, int flags) {
  std::string out;
  JS_SetContextOpaque(ctx, &out);
  JS_SetLogFunc(ctx, &write_to_string);
  JS_PrintValueF(ctx, v, flags);
  return out;
}

std::string dump_memory(JSContext *ctx, bool is_long) {
  std::string out;
  JS_SetContextOpaque(ctx, &out);
  JS_SetLogFunc(ctx, &write_to_string);
  JS_DumpMemory(ctx, is_long ? 1 : 0);
  return out;
}

}  // namespace

bool MqjsEngine::EnsureContext(std::string *error) {
  if (ctx_) return true;

  ctx_ = JS_NewContext(js_mem_buf_, kJsMemSize, &js_stdlib);
  if (!ctx_) {
    if (error) *error = "JS_NewContext failed";
    return false;
  }
  if (error) error->clear();
  return true;
}

bool MqjsEngine::Init(sim_engine_t *engine, std::string *error) {
  mqjs_sim_set_engine(engine);
  return EnsureContext(error);
}

bool MqjsEngine::Reset(std::string *error) {
  if (ctx_) {
    JS_FreeContext(ctx_);
    ctx_ = nullptr;
  }
  return EnsureContext(error);
}

bool MqjsEngine::Eval(const std::string &code, std::string *out, std::string *error) {
  if (out) out->clear();
  if (error) error->clear();
  if (!EnsureContext(error)) return false;

  const int flags = JS_EVAL_REPL | JS_EVAL_RETVAL;
  JSValue val = JS_Eval(ctx_, code.data(), code.size(), "<repl>", flags);
  if (JS_IsException(val)) {
    if (error) {
      JSValue exc = JS_GetException(ctx_);
      *error = print_value(ctx_, exc, JS_DUMP_LONG);
      if (!error->empty() && (*error)[error->size() - 1] != '\n') error->push_back('\n');
    }
    return true;
  }

  if (out && !JS_IsUndefined(val)) {
    *out = print_value(ctx_, val, JS_DUMP_LONG);
    if (!out->empty() && (*out)[out->size() - 1] != '\n') out->push_back('\n');
  }

  return true;
}

bool MqjsEngine::DumpMemory(std::string *out) {
  if (!out) return false;
  out->clear();
  if (!EnsureContext(nullptr)) return false;
  *out = dump_memory(ctx_, false);
  if (!out->empty() && (*out)[out->size() - 1] != '\n') out->push_back('\n');
  return true;
}

