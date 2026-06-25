/* js_command.cpp — native QuickJS console commands for 0103 AtomS3R M12.
 *
 * Supported commands:
 *   js status
 *   js eval <source>
 *   js reset
 *   js gc
 *   js bench
 */
#include "js_command.h"

#include <cstdio>
#include <cstring>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "system_namespace.h"

namespace {
constexpr const char *kTag = "0103_js";
constexpr size_t kMaxSrc = 2048;
constexpr uint32_t kDefaultEvalTimeoutMs = 1000;
constexpr uint32_t kResetTimeoutMs = 2000;

qjs_service_t *g_svc = nullptr;

bool join_source(int argc, char **argv, int first, char *buf, size_t buf_len)
{
    size_t n = 0;
    for (int i = first; i < argc; i++) {
        size_t l = std::strlen(argv[i]);
        if (n + l + (i > first ? 1 : 0) + 1 > buf_len) {
            std::printf("source too long (max %zu)\n", buf_len - 1);
            return false;
        }
        if (i > first) {
            buf[n++] = ' ';
        }
        std::memcpy(buf + n, argv[i], l);
        n += l;
    }
    buf[n] = '\0';
    return true;
}

void print_eval_result(const char *label, const qjs_eval_result_t &r)
{
    std::printf("[%s] ok=%d timed_out=%d elapsed=%ums\n",
                label,
                r.ok,
                r.timed_out,
                (unsigned)r.elapsed_ms);
    if (r.output && r.output[0]) {
        std::printf("%s", r.output);
        if (r.output[std::strlen(r.output) - 1] != '\n') {
            std::printf("\n");
        }
    }
    if (r.error && r.error[0]) {
        std::printf("error: %s\n", r.error);
    }
}

int run_eval(const char *label, const char *src, uint32_t timeout_ms)
{
    if (!g_svc) {
        std::printf("QuickJS service is not started\n");
        return 1;
    }

    qjs_eval_result_t r = {};
    esp_err_t err = qjs_service_eval(g_svc, src, std::strlen(src), timeout_ms, label, &r);
    if (err != ESP_OK) {
        std::printf("service error: %s\n", esp_err_to_name(err));
        qjs_eval_result_free(&r);
        return 1;
    }
    print_eval_result(label, r);
    const bool ok = r.ok && !r.timed_out;
    qjs_eval_result_free(&r);
    return ok ? 0 : 1;
}

int cmd_status()
{
    if (!g_svc) {
        std::printf("QuickJS service is not started\n");
        return 1;
    }

    qjs_service_status_t st = {};
    esp_err_t err = qjs_service_get_status(g_svc, &st, 1000);
    if (err != ESP_OK) {
        std::printf("status error: %s\n", esp_err_to_name(err));
        return 1;
    }

    std::printf("ready=%d busy=%d evals=%u resets=%u last_eval_ms=%u\n",
                st.ready,
                st.busy,
                (unsigned)st.eval_count,
                (unsigned)st.reset_count,
                (unsigned)st.last_eval_ms);
    std::printf("limits: memory=%u stack=%u\n",
                (unsigned)st.memory_limit_bytes,
                (unsigned)st.stack_limit_bytes);
    std::printf("quickjs: used=%u malloc=%u atoms=%u\n",
                (unsigned)st.memory_used_size,
                (unsigned)st.malloc_size,
                (unsigned)st.atom_count);
    std::printf("esp_heap: internal=%u 8bit=%u psram=%u\n",
                (unsigned)st.esp_heap_internal_free,
                (unsigned)st.esp_heap_8bit_free,
                (unsigned)st.esp_heap_psram_free);
    return 0;
}

int cmd_reset()
{
    if (!g_svc) {
        std::printf("QuickJS service is not started\n");
        return 1;
    }
    esp_err_t err = qjs_service_reset(g_svc, kResetTimeoutMs);
    if (err == ESP_OK) {
        esp_err_t sys_err = install_system_namespace(g_svc);
        if (sys_err != ESP_OK) {
            std::printf("reset: ESP_OK, system namespace: %s\n", esp_err_to_name(sys_err));
            return 1;
        }
    }
    std::printf("reset: %s\n", esp_err_to_name(err));
    return err == ESP_OK ? 0 : 1;
}

int cmd_bench()
{
    int rc = 0;
    rc |= run_eval("bench-10k",
                   "(()=>{let t=millis(); let s=0; for(let i=0;i<10000;i++) s+=i; print('sum10k='+String(millis()-t)+',s='+String(s));})()",
                   kDefaultEvalTimeoutMs);
    rc |= run_eval("bench-100k",
                   "(()=>{let t=millis(); let s=0; for(let i=0;i<100000;i++) s+=i; print('sum100k='+String(millis()-t)+',s='+String(s));})()",
                   kDefaultEvalTimeoutMs);
    rc |= run_eval("bench-fib20",
                   "(()=>{function fib(n){return n<2?n:fib(n-1)+fib(n-2)}; let t=millis(); print('fib20='+String(fib(20))+',ms='+String(millis()-t));})()",
                   kDefaultEvalTimeoutMs);
    return rc == 0 ? 0 : 1;
}

int cmd_js(int argc, char **argv)
{
    if (argc < 2) {
        std::printf("usage: js status | eval <source> | reset | gc | bench\n");
        return 0;
    }

    if (std::strcmp(argv[1], "status") == 0) {
        return cmd_status();
    }
    if (std::strcmp(argv[1], "reset") == 0) {
        return cmd_reset();
    }
    if (std::strcmp(argv[1], "gc") == 0) {
        return run_eval("gc", "gc()", kDefaultEvalTimeoutMs);
    }
    if (std::strcmp(argv[1], "bench") == 0) {
        return cmd_bench();
    }
    if (std::strcmp(argv[1], "eval") == 0) {
        if (argc < 3) {
            std::printf("usage: js eval <source>\n");
            return 1;
        }
        char buf[kMaxSrc];
        if (!join_source(argc, argv, 2, buf, sizeof(buf))) {
            return 1;
        }
        return run_eval("atoms3r-eval", buf, kDefaultEvalTimeoutMs);
    }

    std::printf("unknown subcommand: %s\n", argv[1]);
    std::printf("usage: js status | eval <source> | reset | gc | bench\n");
    return 1;
}
}  // namespace

void register_js_commands(qjs_service_t *svc)
{
    g_svc = svc;
    esp_console_cmd_t js_cmd = {};
    js_cmd.command = "js";
    js_cmd.help = "Native QuickJS: js status | eval <source> | reset | gc | bench";
    js_cmd.func = &cmd_js;
    ESP_ERROR_CHECK(esp_console_cmd_register(&js_cmd));
    ESP_LOGI(kTag, "registered AtomS3R native QuickJS console commands");
}
