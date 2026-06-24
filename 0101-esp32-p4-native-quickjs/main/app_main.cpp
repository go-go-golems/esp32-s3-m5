// 0101 — ESP32-P4 native QuickJS service smoke firmware.
// Starts the reusable qjs_service owner task and evaluates small JS snippets.
#include <cstdio>
#include <cstring>

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "qjs_service.h"

namespace {
constexpr const char *kTag = "0101_qjs";
constexpr size_t kQuickJsMemoryLimit = 2 * 1024 * 1024;
constexpr size_t kQuickJsStackLimit = 64 * 1024;

void log_heap(const char *label)
{
    ESP_LOGI(kTag,
             "heap %s: internal=%u 8bit=%u psram=%u",
             label,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void log_status(qjs_service_t *svc, const char *label)
{
    qjs_service_status_t st = {};
    esp_err_t err = qjs_service_get_status(svc, &st, 1000);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "status %s failed: %s", label, esp_err_to_name(err));
        return;
    }
    ESP_LOGI(kTag,
             "status %s: ready=%d busy=%d evals=%u resets=%u last=%ums qjs_used=%u qjs_malloc=%u atoms=%u heap8=%u psram=%u",
             label,
             st.ready,
             st.busy,
             (unsigned)st.eval_count,
             (unsigned)st.reset_count,
             (unsigned)st.last_eval_ms,
             (unsigned)st.memory_used_size,
             (unsigned)st.malloc_size,
             (unsigned)st.atom_count,
             (unsigned)st.esp_heap_8bit_free,
             (unsigned)st.esp_heap_psram_free);
}

void log_eval(qjs_service_t *svc, const char *label, const char *src, uint32_t timeout_ms)
{
    ESP_LOGI(kTag, "eval %s: %s", label, src);
    qjs_eval_result_t result = {};
    esp_err_t err = qjs_service_eval(svc, src, std::strlen(src), timeout_ms, label, &result);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "eval %s service error: %s", label, esp_err_to_name(err));
        qjs_eval_result_free(&result);
        return;
    }

    ESP_LOGI(kTag,
             "eval %s result: ok=%d timed_out=%d elapsed=%ums",
             label,
             result.ok,
             result.timed_out,
             (unsigned)result.elapsed_ms);
    if (result.output && result.output[0]) {
        std::printf("%s", result.output);
        std::fflush(stdout);
    }
    if (result.error && result.error[0]) {
        ESP_LOGE(kTag, "eval %s exception: %s", label, result.error);
    }
    qjs_eval_result_free(&result);
}

void run_service_smoke()
{
    log_heap("before");

    qjs_service_config_t cfg = {};
    cfg.task_name = "qjs0101";
    cfg.task_stack_words = 12288;
    cfg.task_priority = 8;
    cfg.task_core_id = -1;
    cfg.queue_len = 8;
    cfg.memory_limit_bytes = kQuickJsMemoryLimit;
    cfg.stack_limit_bytes = kQuickJsStackLimit;
    cfg.can_block = false;

    qjs_service_t *svc = nullptr;
    esp_err_t err = qjs_service_start(&cfg, &svc);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "qjs_service_start failed: %s", esp_err_to_name(err));
        return;
    }

    log_status(svc, "after-start");
    log_eval(svc, "boot-smoke", "print(1+2)", 1000);
    log_eval(svc,
             "sum10k",
             "let t=millis(); let s=0; for(let i=0;i<10000;i++) s+=i; print('sum10k='+String(millis()-t)+',s='+String(s));",
             1000);
    log_eval(svc, "exception", "throw new Error('boom')", 1000);
    log_status(svc, "after-evals");

    err = qjs_service_reset(svc, 1000);
    ESP_LOGI(kTag, "reset: %s", esp_err_to_name(err));
    log_status(svc, "after-reset");

    qjs_service_stop(svc);
    log_heap("after-stop");
}
}  // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "0101 ESP32-P4 native QuickJS service smoke (ticket ESP32-P4-NATIVE-QUICKJS)");
    run_service_smoke();
}
