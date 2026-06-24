/* wasm_runner.cpp — own the QuickJS-WASM session on one pthread.
 *
 * WAMR's ESP-IDF thread-manager path calls os_self_thread() from
 * wasm_runtime_call_wasm(). In the registry espressif/wasm-micro-runtime
 * component used by this project, os_self_thread() reaches pthread_self(),
 * which asserts when called from app_main's plain FreeRTOS main_task. Keep all
 * calls into wasm_runtime_call_wasm() on this long-lived pthread instead.
 */
#include "wasm_runner.h"

#include "quickjs_embed.h"

#include <cstdio>
#include <cstring>
#include <pthread.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "wasm_export.h"

namespace {
constexpr const char *kTag = "0100_run";
constexpr uint32_t kGuestStack = 32 * 1024;
constexpr uint32_t kGuestHeap  = 512 * 1024;
constexpr size_t kWorkerThreadStackBytes = 64 * 1024;
constexpr UBaseType_t kEvalQueueDepth = 4;

wasm_module_t      g_mod  = nullptr;
wasm_module_inst_t g_inst = nullptr;
wasm_exec_env_t    g_env  = nullptr;

/* WAMR's loader writes into the module buffer (e.g. the reference-types /
   fast-interp load path). The embedded quickjs.wasm lives in read-only
   flash, so it must be copied into a writable buffer (PSRAM) before load.
   On the host this was a malloc'd buffer and worked; on-device it crashed
   with a Store access fault in b_memmove_s writing to the flash blob. */
uint8_t *g_wasm_copy = nullptr;

struct EvalRequest {
    const char *src;
    size_t len;
    int result;
    SemaphoreHandle_t done;
};

QueueHandle_t g_eval_queue = nullptr;
SemaphoreHandle_t g_init_done = nullptr;
pthread_t g_worker = {};
bool g_worker_started = false;
bool g_ready = false;
char g_init_error[160] = "not initialized";

void SetInitError(const char *msg)
{
    std::snprintf(g_init_error, sizeof(g_init_error), "%s", msg ? msg : "unknown error");
}

bool CopyWasmToWritableMemory(void)
{
    if (g_wasm_copy != nullptr) {
        return true;
    }

    const size_t sz = quickjs_wasm_size();
    g_wasm_copy = static_cast<uint8_t *>(heap_caps_malloc(sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!g_wasm_copy) {
        g_wasm_copy = static_cast<uint8_t *>(heap_caps_malloc(sz, MALLOC_CAP_8BIT));
    }
    if (!g_wasm_copy) {
        char msg[96];
        std::snprintf(msg, sizeof(msg), "failed to allocate writable wasm copy (%zu bytes)", sz);
        SetInitError(msg);
        ESP_LOGE(kTag, "%s", msg);
        return false;
    }

    std::memcpy(g_wasm_copy, quickjs_wasm_data(), sz);
    ESP_LOGI(kTag, "copied quickjs.wasm (%zu bytes) to writable buffer %p", sz, g_wasm_copy);
    return true;
}

bool InitSessionOnWorkerThread(void)
{
    char err[256] = {0};

    if (!CopyWasmToWritableMemory()) {
        return false;
    }

    g_mod = wasm_runtime_load(g_wasm_copy, quickjs_wasm_size(), err, sizeof(err));
    if (!g_mod) {
        char msg[sizeof(err) + 40];
        std::snprintf(msg, sizeof(msg), "wasm_runtime_load failed: %s", err);
        SetInitError(msg);
        ESP_LOGE(kTag, "%s", msg);
        return false;
    }

    g_inst = wasm_runtime_instantiate(g_mod, kGuestStack, kGuestHeap, err, sizeof(err));
    if (!g_inst) {
        char msg[sizeof(err) + 48];
        std::snprintf(msg, sizeof(msg), "wasm_runtime_instantiate failed: %s", err);
        SetInitError(msg);
        ESP_LOGE(kTag, "%s", msg);
        wasm_runtime_unload(g_mod);
        g_mod = nullptr;
        return false;
    }

    g_env = wasm_runtime_create_exec_env(g_inst, kGuestStack);
    if (!g_env) {
        SetInitError("wasm_runtime_create_exec_env failed");
        ESP_LOGE(kTag, "%s", g_init_error);
        return false;
    }

    wasm_function_inst_t finit = wasm_runtime_lookup_function(g_inst, "qjs_init");
    if (!finit) {
        SetInitError("qjs_init export not found");
        ESP_LOGE(kTag, "%s", g_init_error);
        return false;
    }
    if (!wasm_runtime_call_wasm(g_env, finit, 0, nullptr)) {
        const char *exception = wasm_runtime_get_exception(g_inst);
        char msg[160];
        std::snprintf(msg, sizeof(msg), "qjs_init call failed: %s", exception ? exception : "<no exception>");
        SetInitError(msg);
        ESP_LOGE(kTag, "%s", msg);
        return false;
    }

    ESP_LOGI(kTag, "QuickJS ready (qjs_init ok on worker pthread)");
    SetInitError("ok");
    return true;
}

int EvalOnWorkerThread(const char *src, size_t len)
{
    if (!g_inst) {
        std::printf("wasm session not initialized\n");
        return -10;
    }
    wasm_function_inst_t feval = wasm_runtime_lookup_function(g_inst, "qjs_eval");
    if (!feval) {
        std::printf("qjs_eval export not found\n");
        return -11;
    }

    /* Copy the JS source into the guest's linear memory (+1 for NUL so the
       guest can echo it if needed). Pass the guest-space address + length. */
    uint64_t wptr = wasm_runtime_module_dup_data(g_inst, src, len + 1);
    if (!wptr) {
        std::printf("wasm_runtime_module_dup_data failed\n");
        return -12;
    }

    uint32_t argv[2] = { static_cast<uint32_t>(wptr), static_cast<uint32_t>(len) };
    const bool ok = wasm_runtime_call_wasm(g_env, feval, 2, argv);
    wasm_runtime_module_free(g_inst, wptr);

    if (!ok) {
        std::printf("eval exception: %s\n", wasm_runtime_get_exception(g_inst));
        return -1;
    }
    return static_cast<int>(argv[0]);  // qjs_eval returns 0 on success, -1 on JS exception
}

void *WorkerThreadEntry(void *)
{
    if (!wasm_runtime_init_thread_env()) {
        SetInitError("wasm_runtime_init_thread_env failed");
        ESP_LOGE(kTag, "%s", g_init_error);
        g_ready = false;
        xSemaphoreGive(g_init_done);
        return nullptr;
    }

    g_ready = InitSessionOnWorkerThread();
    xSemaphoreGive(g_init_done);

    if (!g_ready) {
        wasm_runtime_destroy_thread_env();
        return nullptr;
    }

    while (true) {
        EvalRequest *request = nullptr;
        if (xQueueReceive(g_eval_queue, &request, portMAX_DELAY) != pdTRUE || request == nullptr) {
            continue;
        }
        request->result = EvalOnWorkerThread(request->src, request->len);
        xSemaphoreGive(request->done);
    }

    // Unreachable in the current firmware lifetime model.
    wasm_runtime_destroy_thread_env();
    return nullptr;
}
}  // namespace

bool wasm_runner_init(void)
{
    if (g_ready) {
        return true;
    }
    if (g_worker_started) {
        ESP_LOGE(kTag, "QuickJS worker already started but not ready: %s", g_init_error);
        return false;
    }

    g_eval_queue = xQueueCreate(kEvalQueueDepth, sizeof(EvalRequest *));
    if (g_eval_queue == nullptr) {
        ESP_LOGE(kTag, "xQueueCreate failed");
        return false;
    }
    g_init_done = xSemaphoreCreateBinary();
    if (g_init_done == nullptr) {
        ESP_LOGE(kTag, "xSemaphoreCreateBinary(init) failed");
        return false;
    }

    pthread_attr_t attr;
    int ret = pthread_attr_init(&attr);
    if (ret != 0) {
        ESP_LOGE(kTag, "pthread_attr_init failed: %d", ret);
        return false;
    }
    ret = pthread_attr_setstacksize(&attr, kWorkerThreadStackBytes);
    if (ret != 0) {
        pthread_attr_destroy(&attr);
        ESP_LOGE(kTag, "pthread_attr_setstacksize(%zu) failed: %d", kWorkerThreadStackBytes, ret);
        return false;
    }

    ret = pthread_create(&g_worker, &attr, WorkerThreadEntry, nullptr);
    pthread_attr_destroy(&attr);
    if (ret != 0) {
        ESP_LOGE(kTag, "pthread_create failed: %d", ret);
        return false;
    }
    pthread_detach(g_worker);
    g_worker_started = true;

    xSemaphoreTake(g_init_done, portMAX_DELAY);
    if (!g_ready) {
        ESP_LOGE(kTag, "QuickJS worker init failed: %s", g_init_error);
    }
    return g_ready;
}

int wasm_runner_eval(const char *src, size_t len)
{
    if (!g_ready || g_eval_queue == nullptr) {
        std::printf("wasm session not initialized: %s\n", g_init_error);
        return -10;
    }

    EvalRequest request = {};
    request.src = src;
    request.len = len;
    request.result = -99;
    request.done = xSemaphoreCreateBinary();
    if (request.done == nullptr) {
        std::printf("xSemaphoreCreateBinary(eval) failed\n");
        return -13;
    }

    EvalRequest *request_ptr = &request;
    if (xQueueSend(g_eval_queue, &request_ptr, portMAX_DELAY) != pdTRUE) {
        vSemaphoreDelete(request.done);
        std::printf("xQueueSend(eval) failed\n");
        return -14;
    }

    xSemaphoreTake(request.done, portMAX_DELAY);
    vSemaphoreDelete(request.done);
    return request.result;
}
