# mqjs_service (MicroQuickJS Service + VM Host)

This component bundles **two layers** of embedded JS infrastructure:

1) **`MqjsVm`** — a minimal VM host primitive around MicroQuickJS:
   - creates a `JSContext`,
   - installs output/interrupt hooks,
   - provides helpers for exception capture and value printing.

2) **`mqjs_service`** — a FreeRTOS worker task + queue that serializes JS execution and makes it safe for *many* tasks to request work.

If you only need a **single-threaded** JS host (e.g., everything runs on one task), you can use `MqjsVm` directly.  
If your firmware has **multiple producers** (HTTP, UI, timers, hardware events), use `mqjs_service`.

---

## Quick Start (Service)

```c
static mqjs_service_t* s_svc = nullptr;

void js_start(void) {
  mqjs_service_config_t cfg = {};
  cfg.task_name = "js_svc";
  cfg.task_stack_words = 6144;
  cfg.task_priority = 8;
  cfg.task_core_id = -1;
  cfg.queue_len = 16;
  cfg.arena_bytes = CONFIG_MY_JS_MEM_BYTES;
  cfg.stdlib = &js_stdlib;
  cfg.fix_global_this = true;

  mqjs_service_start(&cfg, &s_svc);
}
```

### Evaluate JS

```c
mqjs_eval_result_t r = {};
mqjs_service_eval(s_svc, code, code_len, 250, "<eval>", &r);
// check r.ok, r.output, r.error
mqjs_eval_result_free(&r);
```

### Run a Job on the JS Thread

```c
static esp_err_t job_on_js(JSContext* ctx, void* user) {
  // safe to call JS API here
  return ESP_OK;
}

mqjs_job_t job = {.fn = &job_on_js, .user = nullptr, .timeout_ms = 50};
mqjs_service_post(s_svc, &job);  // async
```

---

## Quick Start (VM only)

```c
MqjsVmConfig cfg = {};
cfg.arena = arena;
cfg.arena_bytes = arena_bytes;
cfg.stdlib = &js_stdlib;
cfg.fix_global_this = true;

MqjsVm* vm = MqjsVm::Create(cfg);
JSContext* ctx = vm->ctx();
```

---

## Safety Notes

- `timeout_ms` is a **best‑effort VM deadline**, not a caller wait timeout.
- `mqjs_service_eval` / `mqjs_service_run` block until the job finishes.
- Never call `mqjs_service_eval/run` *from within a job* (deadlock risk).

---

## Files

- `include/mqjs_service.h` — public service API
- `include/mqjs_vm.h` — VM primitive API
- `mqjs_service.cpp` — service implementation
- `mqjs_vm.cpp` — VM primitive implementation
