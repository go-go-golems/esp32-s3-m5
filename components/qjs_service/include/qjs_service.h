#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs.h"
#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qjs_service qjs_service_t;

typedef struct {
  const char* task_name;      // default: "qjs_svc"
  uint32_t task_stack_words;  // default: 8192 words
  uint32_t task_priority;     // default: 8
  int32_t task_core_id;       // default: -1 (no pin)
  uint32_t queue_len;         // default: 16

  size_t memory_limit_bytes;  // default: 2 MiB
  size_t stack_limit_bytes;   // default: 64 KiB
  bool can_block;             // default: false
} qjs_service_config_t;

typedef struct {
  bool ok;
  bool timed_out;
  uint32_t elapsed_ms;
  // output/error are malloc'ed; caller frees with qjs_eval_result_free().
  char* output;
  char* error;
} qjs_eval_result_t;

typedef struct {
  bool ready;
  bool busy;
  uint32_t eval_count;
  uint32_t reset_count;
  uint32_t last_eval_ms;
  size_t memory_limit_bytes;
  size_t stack_limit_bytes;
  size_t malloc_size;
  size_t memory_used_size;
  size_t atom_count;
  size_t esp_heap_internal_free;
  size_t esp_heap_8bit_free;
  size_t esp_heap_psram_free;
} qjs_service_status_t;

typedef esp_err_t (*qjs_job_fn_t)(JSContext* ctx, void* user);

typedef struct {
  qjs_job_fn_t fn;     // required
  void* user;
  uint32_t timeout_ms; // 0 => no JS interrupt deadline for the job
} qjs_job_t;

esp_err_t qjs_service_start(const qjs_service_config_t* cfg, qjs_service_t** out);
void qjs_service_stop(qjs_service_t* s);

esp_err_t qjs_service_eval(qjs_service_t* s,
                           const char* code,
                           size_t len,
                           uint32_t timeout_ms,
                           const char* filename,
                           qjs_eval_result_t* out);

esp_err_t qjs_service_run(qjs_service_t* s, const qjs_job_t* job);
esp_err_t qjs_service_post(qjs_service_t* s, const qjs_job_t* job);
esp_err_t qjs_service_reset(qjs_service_t* s, uint32_t timeout_ms);
esp_err_t qjs_service_get_status(qjs_service_t* s, qjs_service_status_t* out, uint32_t timeout_ms);

void qjs_eval_result_free(qjs_eval_result_t* r);

#ifdef __cplusplus
}  // extern "C"
#endif
