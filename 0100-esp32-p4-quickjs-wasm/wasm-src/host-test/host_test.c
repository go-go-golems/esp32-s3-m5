/*
 * host_test.c — Phase 0 host smoke test for quickjs.wasm.
 *
 * Embeds WAMR (libvmlib), registers the "env" native symbols the wasm module
 * imports (host_print / host_millis / host_gpio_write), loads quickjs.wasm,
 * calls qjs_init, then qjs_eval("<src>"). This is the exact code path firmware
 * 0100 will use on the ESP32-P4, so a pass here means the wasm + host API
 * contract is correct before flashing.
 *
 * Build: see CMakeLists.txt. Run:
 *   ./host_test <quickjs.wasm> "<js source>"
 *   ./host_test ../../wasm-build/quickjs.wasm "print(1+2)"
 * Expect console output: 3
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "wasm_export.h"

/* ---- Boundary A: native symbols exported to the wasm guest (module "env") ---- */
static void host_print(wasm_exec_env_t env, const char *s)
{
    (void)env;
    fputs(s, stdout);
    fflush(stdout);
}
static int host_millis(wasm_exec_env_t env)
{
    (void)env;
    return 0; /* unused in the smoke test */
}
static void host_gpio_write(wasm_exec_env_t env, int pin, int val)
{
    (void)env; (void)pin; (void)val;
}

static NativeSymbol native_symbols[] = {
    { "host_print",      (void *)host_print,      "($)",  NULL },
    { "host_millis",     (void *)host_millis,     "()i",  NULL },
    { "host_gpio_write", (void *)host_gpio_write, "(ii)", NULL },
};

static char *read_file(const char *path, uint32_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, sz, f) != (size_t)sz) { perror("fread"); free(buf); fclose(f); return NULL; }
    fclose(f);
    *out_size = (uint32_t)sz;
    return buf;
}

int main(int argc, char **argv)
{
    const char *wasm_path = argc > 1 ? argv[1] : "quickjs.wasm";
    const char *src       = argc > 2 ? argv[2] : "print(1+2)";
    printf("[host] wasm=%s  js=%s\n", wasm_path, src);

    /* 8 MB host pool (device uses PSRAM; host has plenty) */
    static char heap[8 * 1024 * 1024];
    RuntimeInitArgs args;
    memset(&args, 0, sizeof(args));
    args.mem_alloc_type = Alloc_With_Pool;
    args.mem_alloc_option.pool.heap_buf  = heap;
    args.mem_alloc_option.pool.heap_size = sizeof(heap);
    args.native_module_name = "env";
    args.n_native_symbols  = sizeof(native_symbols) / sizeof(native_symbols[0]);
    args.native_symbols     = native_symbols;
    if (!wasm_runtime_full_init(&args)) {
        printf("[host] wasm_runtime_full_init FAILED\n");
        return 1;
    }
    wasm_runtime_set_log_level(WASM_LOG_LEVEL_WARNING);

    uint32_t size = 0;
    char *buf = read_file(wasm_path, &size);
    if (!buf) return 1;

    char err[256] = {0};
    wasm_module_t mod = wasm_runtime_load((uint8_t *)buf, size, err, sizeof(err));
    if (!mod) { printf("[host] load FAILED: %s\n", err); return 1; }

    wasm_module_inst_t inst = wasm_runtime_instantiate(mod, 32 * 1024, 512 * 1024, err, sizeof(err));
    if (!inst) { printf("[host] instantiate FAILED: %s\n", err); return 1; }

    wasm_exec_env_t env = wasm_runtime_create_exec_env(inst, 32 * 1024);

    /* qjs_init() — zero args */
    wasm_function_inst_t finit = wasm_runtime_lookup_function(inst, "qjs_init");
    if (!finit) { printf("[host] qjs_init export not found\n"); return 1; }
    if (!wasm_runtime_call_wasm(env, finit, 0, NULL)) {
        printf("[host] qjs_init call FAILED: %s\n", wasm_runtime_get_exception(inst));
        return 1;
    }
    printf("[host] qjs_init OK\n");

    /* qjs_eval(ptr, len) — copy JS source into guest linear memory */
    wasm_function_inst_t feval = wasm_runtime_lookup_function(inst, "qjs_eval");
    if (!feval) { printf("[host] qjs_eval export not found\n"); return 1; }
    size_t slen = strlen(src);
    uint64_t wptr = wasm_runtime_module_dup_data(inst, src, slen + 1); /* +1: NUL so qjs_eval can echo it */
    if (!wptr) { printf("[host] module_dup_data FAILED\n"); return 1; }
    uint32_t av[2] = { (uint32_t)wptr, (uint32_t)slen };
    bool ok = wasm_runtime_call_wasm(env, feval, 2, av);
    if (!ok)
        printf("[host] qjs_eval FAILED: %s\n", wasm_runtime_get_exception(inst));
    else
        printf("[host] qjs_eval returned %d\n", (int)av[0]);

    wasm_runtime_module_free(inst, wptr);
    wasm_runtime_destroy_exec_env(env);
    wasm_runtime_deinstantiate(inst);
    wasm_runtime_unload(mod);
    free(buf);
    return ok ? 0 : 1;
}
