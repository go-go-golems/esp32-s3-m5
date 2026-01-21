#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

extern "C" {
#include "mquickjs.h"
}

struct MqjsVmConfig {
  void* arena = nullptr;
  size_t arena_bytes = 0;
  const JSSTDLibraryDef* stdlib = nullptr;
  bool fix_global_this = true;
};

// Minimal MicroQuickJS "VM host primitive".
//
// Key invariant: ctx->opaque is always a stable pointer (the owning MqjsVm),
// because MicroQuickJS passes ctx->opaque to JSWriteFunc/JSInterruptHandler.
class MqjsVm {
 public:
  static MqjsVm* Create(const MqjsVmConfig& cfg);
  static MqjsVm* From(JSContext* ctx);
  static void DestroyContext(JSContext* ctx);

  JSContext* ctx() const { return ctx_; }

  void SetDeadlineMs(uint32_t timeout_ms);
  void ClearDeadline();

  std::string PrintValue(JSValue v, int flags = JS_DUMP_LONG);
  std::string GetExceptionString(int flags = JS_DUMP_LONG);
  std::string DumpMemory(bool is_long = false);

 private:
  explicit MqjsVm(JSContext* ctx);
  ~MqjsVm();

  MqjsVm(const MqjsVm&) = delete;
  MqjsVm& operator=(const MqjsVm&) = delete;

  struct CaptureGuard {
    explicit CaptureGuard(MqjsVm* vm, std::string* out);
    ~CaptureGuard();
    CaptureGuard(const CaptureGuard&) = delete;
    CaptureGuard& operator=(const CaptureGuard&) = delete;
    MqjsVm* vm = nullptr;
  };

  static int InterruptHandler(JSContext* ctx, void* opaque);
  static void WriteFunc(void* opaque, const void* buf, size_t buf_len);

  void FixGlobalThis();

  struct RegistryNode {
    JSContext* ctx = nullptr;
    MqjsVm* vm = nullptr;
    RegistryNode* next = nullptr;
  };
  static RegistryNode*& RegistryHead();
  void RegistryInsert();
  void RegistryRemove();

  JSContext* ctx_ = nullptr;
  int64_t deadline_us_ = 0;
  std::string* capture_ = nullptr;
  RegistryNode reg_ = {};
};

