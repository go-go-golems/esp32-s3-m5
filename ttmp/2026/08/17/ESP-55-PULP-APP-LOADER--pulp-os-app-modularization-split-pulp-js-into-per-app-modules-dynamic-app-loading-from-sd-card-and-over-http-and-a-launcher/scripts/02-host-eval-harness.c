/* ESP-55 experiment 2: host harness measuring what a dynamically loaded app
 * costs inside a MicroQuickJS arena, source-eval vs precompiled bytecode.
 *
 * Reuses pulpjsc.c (stubbed stdlib + host stdlib table) by including it
 * with main renamed. Build/run via 02-host-eval-harness.sh.
 *
 * modes:
 *   eval  <arena_kb> <file.js>...   JS_Eval each file in order into ONE
 *                                    context (kernel first); prints heap
 *                                    used after each file (+ after GC) and
 *                                    the parse+run wall time.
 *   bc    <arena_kb> <file.js>...   compile each file to (host, 64-bit)
 *                                    bytecode in a scratch context, then
 *                                    load ALL images into one fresh context
 *                                    (before the kernel eval, as the device
 *                                    does) and JS_Run each; prints heap
 *                                    used after each run.
 * Caveat: host JSValue is 8 bytes (device 4) so absolute heap numbers are
 * inflated ~1.5-2x; use the ratios.
 */
#define main pulpjsc_main
#include "pulpjsc.c"
#undef main
#include <time.h>

static const char kKernel[] =
    "var __cbs = [null];\n"
    "var G = {TAP:0, LONG:1, LEFT:2, RIGHT:3, UP:4, DOWN:5, TICK:100};\n";

static char g_dump[8192];
static size_t g_dump_len;
static void capture(void *o, const void *b, size_t n) {
    (void)o;
    if (g_dump_len + n < sizeof(g_dump)) {
        memcpy(g_dump + g_dump_len, b, n);
        g_dump_len += n;
    }
}
/* returns heap bytes used, parsed from JS_DumpMemory's summary line */
static unsigned heap_used(JSContext *ctx) {
    g_dump_len = 0;
    JS_DumpMemory(ctx, 0);
    g_dump[g_dump_len] = 0;
    const char *p = strstr(g_dump, "heap size=");
    return p ? (unsigned)strtoul(p + 10, NULL, 10) : 0;
}
static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}
static char *slurp(const char *path, long *len) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    fseek(f, 0, SEEK_END); *len = ftell(f); fseek(f, 0, SEEK_SET);
    char *s = malloc(*len + 1);
    if (fread(s, 1, *len, f) != (size_t)*len) exit(1);
    s[*len] = 0; fclose(f);
    return s;
}
static void report_exc(JSContext *ctx, const char *what) {
    JSValue e = JS_GetException(ctx);
    fprintf(stderr, "%s: exception: ", what);
    JS_SetLogFunc(ctx, log_to_stderr);
    JS_PrintValueF(ctx, e, JS_DUMP_LONG);
    fprintf(stderr, "\n");
    JS_SetLogFunc(ctx, capture);
}

static int mode_eval(int argc, char **argv) {
    size_t arena = strtoul(argv[0], NULL, 10) * 1024;
    void *mem = malloc(arena);
    JSContext *ctx = JS_NewContext(mem, arena, &js_stdlib);
    JS_SetLogFunc(ctx, capture);
    JS_GC(ctx); unsigned base = heap_used(ctx);
    printf("| step | source bytes | parse+run ms | heap delta | heap after GC delta |\n|---|---:|---:|---:|---:|\n");
    JSValue v = JS_Eval(ctx, kKernel, strlen(kKernel), "<kernel>", 0);
    if (JS_IsException(v)) { report_exc(ctx, "kernel"); return 1; }
    JS_GC(ctx);
    unsigned prev = heap_used(ctx);
    printf("| kernel | %zu | - | %u | - |\n", strlen(kKernel), prev - base);
    for (int i = 1; i < argc; i++) {
        long len; char *src = slurp(argv[i], &len);
        double t0 = now_ms();
        v = JS_Eval(ctx, src, len, argv[i], JS_EVAL_STRIP_COL);
        double dt = now_ms() - t0;
        if (JS_IsException(v)) { report_exc(ctx, argv[i]); continue; }
        unsigned h1 = heap_used(ctx);
        JS_GC(ctx);
        unsigned h2 = heap_used(ctx);
        const char *nm = strrchr(argv[i], '/'); nm = nm ? nm + 1 : argv[i];
        printf("| %s | %ld | %.2f | %u | %u |\n", nm, len, dt, h1 - prev, h2 - prev);
        prev = h2;
    }
    printf("\ntotal heap used: %u of %zu\n", heap_used(ctx) - base, arena);
    return 0;
}

typedef struct { uint8_t *buf; uint32_t len; const char *name; long src_len; } Img;

static int mode_bc(int argc, char **argv) {
    size_t arena = strtoul(argv[0], NULL, 10) * 1024;
    int n = argc - 1;
    Img *imgs = calloc(n, sizeof(Img));
    /* compile each file in its own scratch context (host 64-bit image) */
    for (int i = 0; i < n; i++) {
        long len; char *src = slurp(argv[i + 1], &len);
        size_t cm = 4u << 20; void *cmem = malloc(cm);
        JSContext *cc = JS_NewContext2(cmem, cm, &js_stdlib, 1);
        JS_SetLogFunc(cc, log_to_stderr);
        JSValue parsed = JS_Parse(cc, src, len, argv[i + 1], JS_EVAL_STRIP_COL);
        if (JS_IsException(parsed)) { report_exc(cc, argv[i + 1]); return 1; }
        JSBytecodeHeader hdr; const uint8_t *data; uint32_t dlen;
        JS_PrepareBytecode(cc, &hdr, &data, &dlen, parsed);
        imgs[i].len = sizeof(hdr) + dlen;
        imgs[i].buf = malloc(imgs[i].len);
        memcpy(imgs[i].buf, &hdr, sizeof(hdr));
        memcpy(imgs[i].buf + sizeof(hdr), data, dlen);
        imgs[i].name = argv[i + 1]; imgs[i].src_len = len;
        JS_FreeContext(cc); free(cmem); free(src);
    }
    void *mem = malloc(arena);
    JSContext *ctx = JS_NewContext(mem, arena, &js_stdlib);
    JS_SetLogFunc(ctx, capture);
    JS_GC(ctx); unsigned base = heap_used(ctx);
    JSValue *mains = calloc(n, sizeof(JSValue));
    /* load ALL images before the kernel eval (zero-RAM-atom rule) */
    for (int i = 0; i < n; i++) {
        if (JS_RelocateBytecode(ctx, imgs[i].buf, imgs[i].len) != 0) {
            fprintf(stderr, "%s: relocate failed\n", imgs[i].name); return 1;
        }
        mains[i] = JS_LoadBytecode(ctx, imgs[i].buf);
        if (JS_IsException(mains[i])) { report_exc(ctx, imgs[i].name); return 1; }
    }
    JSValue v = JS_Eval(ctx, kKernel, strlen(kKernel), "<kernel>", 0);
    if (JS_IsException(v)) { report_exc(ctx, "kernel"); return 1; }
    JS_GC(ctx);
    unsigned prev = heap_used(ctx);
    printf("| step | source bytes | image bytes (host64) | run ms | heap delta | heap after GC delta |\n|---|---:|---:|---:|---:|---:|\n");
    printf("| kernel (after %d images loaded) | %zu | - | - | %u | - |\n", n, strlen(kKernel), prev - base);
    for (int i = 0; i < n; i++) {
        double t0 = now_ms();
        v = JS_Run(ctx, mains[i]);
        double dt = now_ms() - t0;
        if (JS_IsException(v)) { report_exc(ctx, imgs[i].name); continue; }
        unsigned h1 = heap_used(ctx); JS_GC(ctx); unsigned h2 = heap_used(ctx);
        const char *nm = strrchr(imgs[i].name, '/'); nm = nm ? nm + 1 : imgs[i].name;
        printf("| %s | %ld | %u | %.2f | %u | %u |\n", nm, imgs[i].src_len, imgs[i].len, dt, h1 - prev, h2 - prev);
        prev = h2;
    }
    printf("\ntotal heap used: %u of %zu (images live OUTSIDE the arena)\n", heap_used(ctx) - base, arena);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: %s eval|bc <arena_kb> <file.js>...\n", argv[0]); return 2; }
    if (!strcmp(argv[1], "eval")) return mode_eval(argc - 2, argv + 2);
    if (!strcmp(argv[1], "bc")) return mode_bc(argc - 2, argv + 2);
    return 2;
}
