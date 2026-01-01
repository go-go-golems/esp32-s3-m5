#include "eval/JsEvaluator.h"

#include <string>

extern "C" {
#include "mquickjs.h"
extern const JSSTDLibraryDef js_stdlib;
}

uint8_t JsEvaluator::js_mem_buf_[JsEvaluator::kJsMemSize];

namespace {

void write_to_string(void* opaque, const void* buf, size_t buf_len) {
  if (!opaque || !buf || buf_len == 0) {
    return;
  }
  auto* out = static_cast<std::string*>(opaque);
  out->append(static_cast<const char*>(buf), buf_len);
}

std::string print_value(JSContext* ctx, JSValue v) {
  std::string out;
  JS_SetContextOpaque(ctx, &out);
  JS_SetLogFunc(ctx, write_to_string);
  JS_PrintValueF(ctx, v, JS_DUMP_LONG);
  return out;
}

std::string dump_memory(JSContext* ctx) {
  std::string out;
  JS_SetContextOpaque(ctx, &out);
  JS_SetLogFunc(ctx, write_to_string);
  JS_DumpMemory(ctx, 0);
  return out;
}

}  // namespace

JsEvaluator::JsEvaluator() {
  ctx_ = JS_NewContext(js_mem_buf_, kJsMemSize, &js_stdlib);
}

JsEvaluator::~JsEvaluator() {
  if (ctx_) {
    JS_FreeContext(ctx_);
    ctx_ = nullptr;
  }
}

EvalResult JsEvaluator::EvalLine(std::string_view line) {
  EvalResult result;
  if (!ctx_) {
    result.ok = false;
    result.output = "error: JS context is not initialized\n";
    return result;
  }

  const int flags = JS_EVAL_REPL | JS_EVAL_RETVAL;
  JSValue val = JS_Eval(ctx_, line.data(), line.size(), "<repl>", flags);

  if (JS_IsException(val)) {
    result.ok = false;
    JSValue exc = JS_GetException(ctx_);
    result.output = "error: ";
    result.output += print_value(ctx_, exc);
    result.output.push_back('\n');
    return result;
  }

  result.ok = true;
  if (!JS_IsUndefined(val)) {
    result.output = print_value(ctx_, val);
    result.output.push_back('\n');
  }

  return result;
}

bool JsEvaluator::Reset(std::string* error) {
  if (ctx_) {
    JS_FreeContext(ctx_);
    ctx_ = nullptr;
  }

  ctx_ = JS_NewContext(js_mem_buf_, kJsMemSize, &js_stdlib);
  if (!ctx_) {
    if (error) {
      *error = "failed to recreate JS context";
    }
    return false;
  }

  if (error) {
    error->clear();
  }
  return true;
}

bool JsEvaluator::GetStats(std::string* out) {
  if (!out) {
    return false;
  }
  if (!ctx_) {
    *out = "js: <no context>\n";
    return true;
  }

  *out = dump_memory(ctx_);
  return true;
}
