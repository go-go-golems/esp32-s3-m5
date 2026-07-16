// buzzer singleton bindings (ESP-53 P1). Thin owner-task wrappers over
// app_buzzer; all argument bounds live in the module, not here.
#include "app_buzzer.h"
#include "app_js_internal.h"

namespace pulp {

using namespace jsi;

extern "C" {

JSValue js_buzzer_tone(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int freq = 0;
    int ms = 0;
    if (argc < 1 || JS_ToInt32(ctx, &freq, argv[0])) {
        return JS_EXCEPTION;
    }
    if (argc >= 2 && JS_ToInt32(ctx, &ms, argv[1])) {
        return JS_EXCEPTION;
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(BuzzerTone(freq, ms)));
}

JSValue js_buzzer_beep(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(BuzzerBeep()));
}

JSValue js_buzzer_stop(JSContext *ctx, JSValue *, int, JSValue *) {
    BuzzerStop();
    return JS_UNDEFINED;
}

JSValue js_buzzer_melody(JSContext *ctx, JSValue *, int argc,
                         JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "melody(spec)");
    }
    char spec[160];  // 16 notes of "12000:10000," fit
    JSValue err;
    if (!ArgString(ctx, argv[0], spec, sizeof(spec), &err)) {
        return err;
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(BuzzerMelody(spec)));
}

}  // extern "C"

}  // namespace pulp
