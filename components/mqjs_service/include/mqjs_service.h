#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "mquickjs.h"
#ifdef __cplusplus
}  // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mqjs_service mqjs_service_t;

typedef struct {
  const char* task_name;            // default: "mqjs_svc"
  uint32_t task_stack_words;        // default: 6144/4
  uint32_t task_priority;           // default: 8
  int32_t task_core_id;             // default: -1 (no pin)
  uint32_t queue_len;               // default: 16

  size_t arena_bytes;               // required
  const JSSTDLibraryDef* stdlib;    // required
  bool fix_global_this;             // default: true
} mqjs_service_config_t;

typedef struct {
  bool ok;
  bool timed_out;
  // output/error are malloc'ed; caller frees (mqjs_eval_result_free).
  char* output;
  char* error;
} mqjs_eval_result_t;

typedef esp_err_t (*mqjs_job_fn_t)(JSContext* ctx, void* user);

typedef struct {
  mqjs_job_fn_t fn;          // required
  void* user;
  uint32_t timeout_ms;       // 0 => no deadline
} mqjs_job_t;

esp_err_t mqjs_service_start(const mqjs_service_config_t* cfg, mqjs_service_t** out);
void mqjs_service_stop(mqjs_service_t* s);  // best-effort; not required for normal firmware lifetime

esp_err_t mqjs_service_eval(mqjs_service_t* s,
                            const char* code,
                            size_t len,
                            uint32_t timeout_ms,
                            const char* filename,
                            mqjs_eval_result_t* out);

esp_err_t mqjs_service_run(mqjs_service_t* s, const mqjs_job_t* job);
esp_err_t mqjs_service_post(mqjs_service_t* s, const mqjs_job_t* job);  // enqueue only, no wait

void mqjs_eval_result_free(mqjs_eval_result_t* r);

#ifdef __cplusplus
}  // extern "C"
#endif
