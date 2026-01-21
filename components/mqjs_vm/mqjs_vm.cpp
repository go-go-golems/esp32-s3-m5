#include "mqjs_vm.h"

#include <cstdio>

#include "esp_timer.h"

MqjsVm::RegistryNode*& MqjsVm::RegistryHead()
{
  static RegistryNode* head = nullptr;
  return head;
}

void MqjsVm::RegistryInsert()
{
  reg_.ctx = ctx_;
  reg_.vm = this;
  reg_.next = RegistryHead();
  RegistryHead() = &reg_;
}

void MqjsVm::RegistryRemove()
{
  RegistryNode** cur = &RegistryHead();
  while (*cur) {
    if (*cur == &reg_) {
      *cur = reg_.next;
      reg_.ctx = nullptr;
      reg_.vm = nullptr;
      reg_.next = nullptr;
      return;
    }
    cur = &((*cur)->next);
  }
}

MqjsVm* MqjsVm::Create(const MqjsVmConfig& cfg)
{
  if (!cfg.arena || cfg.arena_bytes == 0 || !cfg.stdlib) {
    return nullptr;
  }

  JSContext* ctx = JS_NewContext(cfg.arena, cfg.arena_bytes, cfg.stdlib);
  if (!ctx) {
    return nullptr;
  }

  auto* vm = new MqjsVm(ctx);
  vm->RegistryInsert();

  // MicroQuickJS passes ctx->opaque to JSWriteFunc/JSInterruptHandler callbacks.
  // Never change this opaque pointer after init.
  JS_SetContextOpaque(ctx, vm);
  JS_SetLogFunc(ctx, &MqjsVm::WriteFunc);
  JS_SetInterruptHandler(ctx, &MqjsVm::InterruptHandler);

  if (cfg.fix_global_this) {
    vm->FixGlobalThis();
  }

  return vm;
}

MqjsVm* MqjsVm::From(JSContext* ctx)
{
  if (!ctx) return nullptr;
  for (RegistryNode* n = RegistryHead(); n; n = n->next) {
    if (n->ctx == ctx) return n->vm;
  }
  return nullptr;
}

void MqjsVm::DestroyContext(JSContext* ctx)
{
  if (!ctx) return;
  auto* vm = MqjsVm::From(ctx);
  if (!vm) {
    JS_FreeContext(ctx);
    return;
  }
  delete vm;
}

MqjsVm::MqjsVm(JSContext* ctx) : ctx_(ctx) {}

MqjsVm::~MqjsVm()
{
  if (!ctx_) return;
  RegistryRemove();
  JS_SetContextOpaque(ctx_, nullptr);
  JS_FreeContext(ctx_);
  ctx_ = nullptr;
}

MqjsVm::CaptureGuard::CaptureGuard(MqjsVm* vm_, std::string* out) : vm(vm_)
{
  if (!vm) return;
  vm->capture_ = out;
}

MqjsVm::CaptureGuard::~CaptureGuard()
{
  if (!vm) return;
  vm->capture_ = nullptr;
}

int MqjsVm::InterruptHandler(JSContext* ctx, void* opaque)
{
  (void)ctx;
  auto* vm = static_cast<MqjsVm*>(opaque);
  if (!vm || vm->deadline_us_ == 0) return 0;
  return esp_timer_get_time() > vm->deadline_us_;
}

void MqjsVm::WriteFunc(void* opaque, const void* buf, size_t buf_len)
{
  if (!opaque || !buf || buf_len == 0) return;
  auto* vm = static_cast<MqjsVm*>(opaque);
  if (!vm) return;

  if (vm->capture_) {
    vm->capture_->append(static_cast<const char*>(buf), buf_len);
    return;
  }

  (void)fwrite(buf, 1, buf_len, stdout);
}

void MqjsVm::FixGlobalThis()
{
  if (!ctx_) return;
  JSValue glob = JS_GetGlobalObject(ctx_);
  if (JS_IsException(glob)) return;
  (void)JS_SetPropertyStr(ctx_, glob, "globalThis", glob);
}

void MqjsVm::SetDeadlineMs(uint32_t timeout_ms)
{
  if (timeout_ms == 0) {
    deadline_us_ = 0;
    return;
  }
  deadline_us_ = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
}

void MqjsVm::ClearDeadline()
{
  deadline_us_ = 0;
}

std::string MqjsVm::PrintValue(JSValue v, int flags)
{
  std::string out;
  CaptureGuard cap(this, &out);

  JSGCRef v_ref;
  JS_PUSH_VALUE(ctx_, v);
  JS_PrintValueF(ctx_, v, flags);
  JS_POP_VALUE(ctx_, v);

  return out;
}

std::string MqjsVm::GetExceptionString(int flags)
{
  if (!ctx_) return std::string();
  JSValue exc = JS_GetException(ctx_);
  return PrintValue(exc, flags);
}

std::string MqjsVm::DumpMemory(bool is_long)
{
  std::string out;
  CaptureGuard cap(this, &out);
  JS_DumpMemory(ctx_, is_long ? 1 : 0);
  return out;
}

