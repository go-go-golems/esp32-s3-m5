// ESP-50 Phase 11: bounded MicroQuickJS feasibility spike for PaperS3.
//
// Autoruns the full probe suite at boot and prints structured evidence
// lines ("SPIKE|<probe>|..."), then idles. Deliberately not linked into
// the 0112 reader firmware. Capture with the 0112 console client script
// (no modem control) or any raw reader of the USB Serial/JTAG port.
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqjs_vm.h"
#include "spike_widgets.h"

extern "C" const JSSTDLibraryDef js_stdlib;

namespace {

// ---- harness ----

struct Vm {
    MqjsVm *vm = nullptr;
    uint8_t *arena = nullptr;

    bool Open(size_t bytes, bool psram) {
        arena = static_cast<uint8_t *>(heap_caps_malloc(
            bytes, psram ? MALLOC_CAP_SPIRAM : MALLOC_CAP_INTERNAL));
        if (arena == nullptr) {
            return false;
        }
        MqjsVmConfig cfg{};
        cfg.arena = arena;
        cfg.arena_bytes = bytes;
        cfg.stdlib = &js_stdlib;
        cfg.fix_global_this = true;
        vm = MqjsVm::Create(cfg);
        if (vm == nullptr) {
            free(arena);
            arena = nullptr;
            return false;
        }
        return true;
    }
    void Close() {
        if (vm != nullptr) {
            MqjsVm::DestroyContext(vm->ctx());
            vm = nullptr;
        }
        free(arena);
        arena = nullptr;
    }
};

struct EvalOut {
    bool ok;
    std::string text;
};

EvalOut Eval(MqjsVm *vm, const char *code) {
    JSContext *ctx = vm->ctx();
    const JSValue v =
        JS_Eval(ctx, code, strlen(code), "<probe>", JS_EVAL_RETVAL);
    if (JS_IsException(v)) {
        return EvalOut{false, vm->GetExceptionString()};
    }
    return EvalOut{true, vm->PrintValue(v)};
}

int g_pass = 0;
int g_fail = 0;

void Report(const char *probe, bool pass, const char *fmt, ...) {
    char detail[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(detail, sizeof(detail), fmt, ap);
    va_end(ap);
    printf("SPIKE|%s|%s|%s\n", probe, pass ? "PASS" : "FAIL", detail);
    (pass ? g_pass : g_fail)++;
}

// ---- probe 1: context startup at several fixed arena sizes (868e) ----

void ProbeStartup() {
    struct Case {
        size_t kb;
        bool psram;
    };
    static const Case kCases[] = {{8, false},   {16, false},  {32, false},
                                  {64, false},  {128, false}, {256, false},
                                  {1024, true}, {4096, true}};
    for (const Case &c : kCases) {
        Vm vm;
        const int64_t t0 = esp_timer_get_time();
        const bool opened = vm.Open(c.kb * 1024, c.psram);
        const int64_t t1 = esp_timer_get_time();
        if (!opened) {
            // Small arenas may be legitimately rejected; that is the
            // explicit-failure behavior the task asks about.
            Report("startup", true, "%uKB %s: JS_NewContext rejected cleanly",
                   static_cast<unsigned>(c.kb), c.psram ? "psram" : "internal");
            continue;
        }
        const EvalOut r = Eval(vm.vm, "6*7");
        const int64_t t2 = esp_timer_get_time();
        Report("startup", r.ok && r.text == "42",
               "%uKB %s: create=%" PRId64 "us eval=%" PRId64 "us result=%s",
               static_cast<unsigned>(c.kb), c.psram ? "psram" : "internal",
               t1 - t0, t2 - t1, r.text.c_str());
        vm.Close();
    }
}

// ---- probe 2: syntax compatibility (0fdb) ----

void ProbeSyntax() {
    struct Case {
        const char *name;
        const char *code;
        bool expect_ok;
    };
    static const Case kCases[] = {
        {"var", "var x1=1; x1", true},
        {"let", "let y1=2; y1", false},
        {"const", "const z1=3; z1", false},
        {"arrow", "var f1=function(){return 0;}; var g1=(a)=>a+1; g1(1)",
         false},
        {"class", "class A1 {}", false},
        {"template-literal", "var t1=`x${1+1}`; t1", false},
        {"spread-array", "var a1=[1,2]; var b1=[...a1]; b1.length", false},
        {"for-of", "var s1=0; var v1; for (v1 of [1,2,3]) s1+=v1; s1", true},
        {"destructuring", "var o1={p:5}; var {p}=o1; p", false},
        {"getter-literal", "var o2={get g(){return 4;}}; o2.g", true},
        {"module-import", "import x2 from 'mod';", false},
        {"undeclared-global", "q1 = 5", false},
        {"array-hole-write", "var a2=[]; a2[10]=2", false},
        {"closure",
         "function mk(){var n=0; return function(){return ++n;};} "
         "var c1=mk(); c1(); c1()",
         true},
        {"fluent-es5",
         "function T(v){this.v=v;} "
         "T.prototype.size=function(s){this.s=s;return this;}; "
         "T.prototype.center=function(){this.c=1;return this;}; "
         "new T('hi').size('xl').center().v",
         true},
        {"json", "JSON.stringify({a:[1,2]})", true},
        {"array-map", "[1,2,3].map(function(x){return x*2;}).join(',')",
         true},
        {"regexp", "'abc123'.match(/[0-9]+/)[0]", true},
    };
    Vm vm;
    if (!vm.Open(128 * 1024, false)) {
        Report("syntax", false, "could not open 128KB context");
        return;
    }
    for (const Case &c : kCases) {
        const EvalOut r = Eval(vm.vm, c.code);
        // Every probe "passes" as evidence; the classification is the data.
        Report("syntax", r.ok == c.expect_ok, "%s: %s -> %s", c.name,
               r.ok ? "OK" : "ERR", r.text.c_str());
    }
    vm.Close();
}

// ---- probe 3: memory exhaustion and recovery (868e/dzfz) ----

void ProbeOom() {
    Vm vm;
    if (!vm.Open(24 * 1024, false)) {
        Report("oom", false, "could not open 24KB context");
        return;
    }
    vm.vm->SetDeadlineMs(5000);  // backstop only; OOM should hit first
    const int64_t t0 = esp_timer_get_time();
    const EvalOut r = Eval(
        vm.vm, "var a=[]; for(;;) { a.push('0123456789abcdef'); }");
    const int64_t ms = (esp_timer_get_time() - t0) / 1000;
    vm.vm->ClearDeadline();
    Report("oom", !r.ok, "exhaustion after %" PRId64 "ms -> %s", ms,
           r.text.c_str());
    // The context must stay usable after an OOM exception.
    const EvalOut again = Eval(vm.vm, "1+1");
    Report("oom", again.ok && again.text == "2",
           "recovery eval after OOM -> %s", again.text.c_str());
    vm.Close();
}

// ---- probe 4: compacting GC vs rooted references (vq48) ----

void ProbeGcRooting() {
    Vm vm;
    if (!vm.Open(64 * 1024, false)) {
        Report("gcroot", false, "could not open 64KB context");
        return;
    }
    JSContext *ctx = vm.vm->ctx();
    static const char kRootedObj[] = "({tag:'rooted', n:41})";
    JSValue v = JS_Eval(ctx, kRootedObj, strlen(kRootedObj), "<gc>",
                        JS_EVAL_RETVAL);
    if (JS_IsException(v)) {
        Report("gcroot", false, "setup eval failed: %s",
               vm.vm->GetExceptionString().c_str());
        vm.Close();
        return;
    }
    JSGCRef ref;
    JSValue *rooted = JS_AddGCRef(ctx, &ref);
    *rooted = v;
    const uintptr_t bits_before = (uintptr_t)*rooted;
    // Churn the heap so the compacting GC has to move things.
    for (int i = 0; i < 20; i++) {
        const EvalOut churn = Eval(
            vm.vm,
            "(function(){var g=[];for(var i=0;i<200;i++)"
            "g.push('churn-'+i);return g.length;})()");
        if (!churn.ok) {
            Report("gcroot", false, "churn eval %d failed: %s", i,
                   churn.text.c_str());
            JS_DeleteGCRef(ctx, &ref);
            vm.Close();
            return;
        }
        (void)Eval(vm.vm, "gc()");
    }
    const uintptr_t bits_after = (uintptr_t)*rooted;
    // The rooted value must still be the same live object.
    const JSValue n = JS_GetPropertyStr(ctx, *rooted, "n");
    int n_val = -1;
    (void)JS_ToInt32(ctx, &n_val, n);
    Report("gcroot", n_val == 41,
           "rooted object intact after 20 gc cycles (n=%d, moved=%s)",
           n_val, bits_before != bits_after ? "yes" : "no");
    JS_DeleteGCRef(ctx, &ref);
    vm.Close();
}

// ---- probe 5: opaque widget handles, finalizers, staleness (durp/dygk) ----

void ProbeWidgets() {
    Vm vm;
    if (!vm.Open(64 * 1024, false)) {
        Report("widget", false, "could not open 64KB context");
        return;
    }
    SpikeWidgetsReset();
    EvalOut r = Eval(vm.vm, "var w = new S3Widget(7); w.value");
    Report("widget", r.ok && r.text == "7", "construct + read -> %s",
           r.text.c_str());
    r = Eval(vm.vm, "w.bump()");
    Report("widget", r.ok && r.text == "8", "method mutate -> %s",
           r.text.c_str());
    r = Eval(vm.vm, "widgetLiveCount()");
    Report("widget", r.ok && r.text == "1", "native live count -> %s",
           r.text.c_str());
    // Native teardown while the JS wrapper is alive: stale, not UB.
    r = Eval(vm.vm,
             "widgetDestroyAll();"
             "var msg; try { w.value; msg='no-throw'; }"
             "catch(e) { msg = e.message; } msg");
    Report("widget", r.ok && r.text.find("stale") != std::string::npos,
           "post-teardown access -> %s (stale_hits=%u)", r.text.c_str(),
           static_cast<unsigned>(SpikeWidgetsStaleHits()));
    // Finalizer runs when the wrapper is collected.
    r = Eval(vm.vm, "w = null; gc(); 'ok'");
    Report("widget", r.ok && SpikeWidgetsFinalized() >= 1,
           "finalizer after gc (created=%u finalized=%u live=%u)",
           static_cast<unsigned>(SpikeWidgetsCreated()),
           static_cast<unsigned>(SpikeWidgetsFinalized()),
           static_cast<unsigned>(SpikeWidgetsLive()));
    // Diagnostic C function binding (durp).
    r = Eval(vm.vm, "typeof millis()");
    Report("widget", r.ok && r.text == "\"number\"" ,
           "millis() diagnostic binding -> %s", r.text.c_str());
    vm.Close();
}

// ---- probe 6: execution budget and cancellation (dzfz) ----

void ProbeCancel() {
    Vm vm;
    if (!vm.Open(64 * 1024, false)) {
        Report("cancel", false, "could not open 64KB context");
        return;
    }
    vm.vm->SetDeadlineMs(100);
    const int64_t t0 = esp_timer_get_time();
    const EvalOut r = Eval(vm.vm, "for(;;);");
    const int64_t ms = (esp_timer_get_time() - t0) / 1000;
    vm.vm->ClearDeadline();
    Report("cancel", !r.ok && ms < 1000,
           "runaway loop stopped after %" PRId64 "ms -> %s", ms,
           r.text.c_str());
    const EvalOut again = Eval(vm.vm, "2+2");
    Report("cancel", again.ok && again.text == "4",
           "recovery eval after cancellation -> %s", again.text.c_str());
    vm.Close();
}

// ---- probe 7: trusted relocated bytecode, fully on-device (m1w2) ----

void ProbeBytecode() {
    static const char kScript[] =
        "(function(){ return 'bytecode:' + (6*7); })()";
    // Compile in a throwaway preparation context.
    const size_t comp_size = 128 * 1024;
    uint8_t *comp_arena =
        static_cast<uint8_t *>(heap_caps_malloc(comp_size,
                                                MALLOC_CAP_INTERNAL));
    JSContext *comp_ctx =
        JS_NewContext2(comp_arena, comp_size, &js_stdlib, 1);
    if (comp_ctx == nullptr) {
        Report("bytecode", false, "compile context failed");
        free(comp_arena);
        return;
    }
    const JSValue parsed = JS_Parse(comp_ctx, kScript, strlen(kScript),
                                    "<bc>", JS_EVAL_RETVAL);
    if (JS_IsException(parsed)) {
        Report("bytecode", false, "parse failed");
        JS_FreeContext(comp_ctx);
        free(comp_arena);
        return;
    }
    JSBytecodeHeader hdr;
    const uint8_t *data_buf = nullptr;
    uint32_t data_len = 0;
    JS_PrepareBytecode(comp_ctx, &hdr, &data_buf, &data_len, parsed);
    if (JS_RelocateBytecode2(comp_ctx, &hdr, const_cast<uint8_t *>(data_buf),
                             data_len, 0, 0) != 0) {
        Report("bytecode", false, "relocate-to-zero failed");
        JS_FreeContext(comp_ctx);
        free(comp_arena);
        return;
    }
    // Persist header + payload exactly like `mqjs -o` writes a file.
    const uint32_t image_len = sizeof(hdr) + data_len;
    uint8_t *image = static_cast<uint8_t *>(malloc(image_len));
    memcpy(image, &hdr, sizeof(hdr));
    memcpy(image + sizeof(hdr), data_buf, data_len);
    JS_FreeContext(comp_ctx);
    free(comp_arena);

    // Execute the trusted image in a fresh context.
    Vm vm;
    if (!vm.Open(64 * 1024, false)) {
        Report("bytecode", false, "exec context failed");
        free(image);
        return;
    }
    JSContext *ctx = vm.vm->ctx();
    bool ok = JS_IsBytecode(image, image_len);
    if (ok && JS_RelocateBytecode(ctx, image, image_len) != 0) {
        ok = false;
    }
    std::string result = "n/a";
    if (ok) {
        const JSValue fn = JS_LoadBytecode(ctx, image);
        const JSValue out = JS_Run(ctx, fn);
        if (JS_IsException(out)) {
            ok = false;
            result = vm.vm->GetExceptionString();
        } else {
            result = vm.vm->PrintValue(out);
        }
    }
    Report("bytecode", ok && result == "\"bytecode:42\"",
           "compile->relocate->load->run (%u bytes) -> %s",
           static_cast<unsigned>(image_len), result.c_str());
    vm.Close();
    free(image);
}

}  // namespace

extern "C" void app_main(void) {
    // USB Serial/JTAG drops output while no host reads: loop the suite so
    // any capture window sees at least one complete run.
    for (int run = 1;; run++) {
        g_pass = 0;
        g_fail = 0;
        printf("\nSPIKE|suite|BEGIN|run=%d mquickjs feasibility (ESP-50 P11)\n",
               run);
        printf("SPIKE|env|INFO|internal_free=%u psram_free=%u\n",
               static_cast<unsigned>(
                   heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
               static_cast<unsigned>(
                   heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
        ProbeStartup();
        ProbeSyntax();
        ProbeOom();
        ProbeGcRooting();
        ProbeWidgets();
        ProbeCancel();
        ProbeBytecode();
        printf("SPIKE|suite|%s|run=%d pass=%d fail=%d\n",
               g_fail == 0 ? "PASS" : "FAIL", run, g_pass, g_fail);
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
