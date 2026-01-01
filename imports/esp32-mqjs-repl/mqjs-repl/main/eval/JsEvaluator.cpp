#include "eval/JsEvaluator.h"

#include <dirent.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include "mquickjs.h"
extern const JSSTDLibraryDef js_stdlib;
}

#include "storage/Spiffs.h"

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

bool JsEvaluator::Autoload(bool format_if_mount_failed, std::string* out, std::string* error) {
  if (!out) {
    if (error) {
      *error = "missing output buffer";
    }
    return false;
  }
  out->clear();

  if (!ctx_) {
    if (error) {
      *error = "JS context is not initialized";
    }
    return false;
  }

  const esp_err_t mount_err = SpiffsEnsureMounted(format_if_mount_failed);
  if (mount_err != ESP_OK) {
    if (error) {
      *error = "SPIFFS mount failed (try :autoload --format)";
    }
    return false;
  }

  DIR* dir = opendir("/spiffs/autoload");
  if (!dir) {
    out->append("autoload: no /spiffs/autoload directory (create it and add .js files)\n");
    return true;
  }

  int total = 0;
  int ok = 0;
  int failed = 0;

  for (dirent* ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
    const char* name = ent->d_name;
    if (!name || name[0] == '\0') {
      continue;
    }
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
      continue;
    }

    const size_t len = strlen(name);
    if (len < 3 || strcmp(name + (len - 3), ".js") != 0) {
      continue;
    }

    total++;

    char path[256];
    const int n = snprintf(path, sizeof(path), "/spiffs/autoload/%s", name);
    if (n <= 0 || n >= static_cast<int>(sizeof(path))) {
      failed++;
      out->append("autoload: skip (path too long)\n");
      continue;
    }

    uint8_t* buf = nullptr;
    size_t buf_len = 0;
    const esp_err_t read_err = SpiffsReadFile(path, &buf, &buf_len);
    if (read_err != ESP_OK || !buf) {
      failed++;
      out->append("autoload: failed to read ");
      out->append(path);
      out->push_back('\n');
      continue;
    }

    const int flags = 0;
    JSValue val = JS_Eval(ctx_, reinterpret_cast<const char*>(buf), buf_len, path, flags);
    free(buf);

    if (JS_IsException(val)) {
      failed++;
      JSValue exc = JS_GetException(ctx_);
      out->append("autoload: error in ");
      out->append(path);
      out->append(": ");
      out->append(print_value(ctx_, exc));
      out->push_back('\n');
      continue;
    }

    ok++;
  }

  closedir(dir);

  char summary[96];
  snprintf(summary, sizeof(summary), "autoload: ok=%d failed=%d total=%d\n", ok, failed, total);
  out->append(summary);
  return true;
}
