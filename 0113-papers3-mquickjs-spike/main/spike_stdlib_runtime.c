/* Device implementations for the spike stdlib (ESP-50 Phase 11).
 *
 * The generated table (spike_stdlib.h, included at the END of this file)
 * references these functions by name; the pairing contract is that this
 * translation unit defines every app-level symbol the stdlib definition
 * (tools/spike_stdlib.c) names.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "esp_timer.h"

#include "mquickjs.h"
#include "spike_widgets.h"

#define JS_CLASS_S3WIDGET (JS_CLASS_USER + 0)
#define JS_CLASS_COUNT (JS_CLASS_USER + 1)

/* ---- diagnostic functions (task durp) ---- */

static int64_t get_time_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_millis(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_NewInt64(ctx, get_time_ms());
}

static JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_ThrowTypeError(ctx, "load() not supported");
}

static JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_ThrowTypeError(ctx, "setTimeout() not supported");
}

static JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_ThrowTypeError(ctx, "clearTimeout() not supported");
}

static JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    int i;
    JSValue v;

    for (i = 0; i < argc; i++) {
        if (i != 0)
            putchar(' ');
        v = argv[i];
        if (JS_IsString(ctx, v)) {
            JSCStringBuf buf;
            const char *str;
            size_t len;
            str = JS_ToCStringLen(ctx, &len, v, &buf);
            fwrite(str, 1, len, stdout);
        } else {
            JS_PrintValueF(ctx, argv[i], JS_DUMP_LONG);
        }
    }
    putchar('\n');
    return JS_UNDEFINED;
}

static JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    JS_GC(ctx);
    return JS_UNDEFINED;
}

/* ---- generation-safe widget handles (task dygk) ---- */

#define SPIKE_MAX_WIDGETS 16

typedef struct {
    uint16_t generation;
    uint8_t in_use;
    int32_t value;
} SpikeWidgetSlot;

static SpikeWidgetSlot s_widgets[SPIKE_MAX_WIDGETS];
static uint32_t s_widgets_created;
static uint32_t s_widgets_finalized;
static uint32_t s_widgets_stale_hits;

void SpikeWidgetsReset(void)
{
    /* Generations survive reset so handles from before stay stale. */
    for (int i = 0; i < SPIKE_MAX_WIDGETS; i++) {
        s_widgets[i].in_use = 0;
    }
    s_widgets_created = 0;
    s_widgets_finalized = 0;
    s_widgets_stale_hits = 0;
}

uint32_t SpikeWidgetsLive(void)
{
    uint32_t n = 0;
    for (int i = 0; i < SPIKE_MAX_WIDGETS; i++) {
        n += s_widgets[i].in_use ? 1 : 0;
    }
    return n;
}

uint32_t SpikeWidgetsCreated(void) { return s_widgets_created; }
uint32_t SpikeWidgetsFinalized(void) { return s_widgets_finalized; }
uint32_t SpikeWidgetsStaleHits(void) { return s_widgets_stale_hits; }

/* Handle encoding: (generation << 16) | index, biased by 1 so a valid
   handle is never the NULL opaque. */
static void *widget_handle_pack(uint32_t index, uint16_t generation)
{
    return (void *)(uintptr_t)(((uint32_t)generation << 16 | index) + 1);
}

static SpikeWidgetSlot *widget_handle_resolve(void *opaque)
{
    if (opaque == NULL) {
        return NULL;
    }
    const uint32_t packed = (uint32_t)(uintptr_t)opaque - 1;
    const uint32_t index = packed & 0xFFFF;
    const uint16_t generation = (uint16_t)(packed >> 16);
    if (index >= SPIKE_MAX_WIDGETS || !s_widgets[index].in_use ||
        s_widgets[index].generation != generation) {
        return NULL;
    }
    return &s_widgets[index];
}

static JSValue js_s3widget_constructor(JSContext *ctx, JSValue *this_val, int argc,
                                       JSValue *argv)
{
    JSGCRef obj_ref;
    JSValue *obj;
    int value;
    int index;

    if (!(argc & FRAME_CF_CTOR))
        return JS_ThrowTypeError(ctx, "must be called with new");
    argc &= ~FRAME_CF_CTOR;
    for (index = 0; index < SPIKE_MAX_WIDGETS; index++) {
        if (!s_widgets[index].in_use)
            break;
    }
    if (index >= SPIKE_MAX_WIDGETS)
        return JS_ThrowTypeError(ctx, "widget arena full");

    /* Root the wrapper across the allocating JS_ToInt32 call (task vq48:
       the compacting GC may move it). */
    obj = JS_PushGCRef(ctx, &obj_ref);
    *obj = JS_NewObjectClassUser(ctx, JS_CLASS_S3WIDGET);
    if (JS_ToInt32(ctx, &value, argv[0]))
        return JS_EXCEPTION;
    s_widgets[index].in_use = 1;
    s_widgets[index].value = value;
    s_widgets_created++;
    JS_SetOpaque(ctx, *obj, widget_handle_pack(index, s_widgets[index].generation));
    JS_PopGCRef(ctx, &obj_ref);
    return *obj;
}

static void js_s3widget_finalizer(JSContext *ctx, void *opaque)
{
    SpikeWidgetSlot *slot = widget_handle_resolve(opaque);
    s_widgets_finalized++;
    if (slot != NULL) {
        slot->in_use = 0;  /* wrapper collected -> release native slot */
    }
    /* stale handle: native side already torn down; nothing to touch. */
}

static JSValue js_s3widget_get_value(JSContext *ctx, JSValue *this_val, int argc,
                                     JSValue *argv)
{
    if (JS_GetClassID(ctx, *this_val) != JS_CLASS_S3WIDGET)
        return JS_ThrowTypeError(ctx, "expecting S3Widget");
    SpikeWidgetSlot *slot = widget_handle_resolve(JS_GetOpaque(ctx, *this_val));
    if (slot == NULL) {
        s_widgets_stale_hits++;
        return JS_ThrowTypeError(ctx, "stale widget handle");
    }
    return JS_NewInt32(ctx, slot->value);
}

static JSValue js_s3widget_bump(JSContext *ctx, JSValue *this_val, int argc,
                                JSValue *argv)
{
    if (JS_GetClassID(ctx, *this_val) != JS_CLASS_S3WIDGET)
        return JS_ThrowTypeError(ctx, "expecting S3Widget");
    SpikeWidgetSlot *slot = widget_handle_resolve(JS_GetOpaque(ctx, *this_val));
    if (slot == NULL) {
        s_widgets_stale_hits++;
        return JS_ThrowTypeError(ctx, "stale widget handle");
    }
    slot->value++;
    return JS_NewInt32(ctx, slot->value);
}

/* Simulates a native page teardown: every live slot's generation advances,
   so existing JS wrappers become stale without any JS-side bookkeeping. */
static JSValue js_widget_destroy_all(JSContext *ctx, JSValue *this_val, int argc,
                                     JSValue *argv)
{
    for (int i = 0; i < SPIKE_MAX_WIDGETS; i++) {
        if (s_widgets[i].in_use) {
            s_widgets[i].in_use = 0;
            s_widgets[i].generation++;
        }
    }
    return JS_UNDEFINED;
}

static JSValue js_widget_live_count(JSContext *ctx, JSValue *this_val, int argc,
                                    JSValue *argv)
{
    return JS_NewInt32(ctx, (int)SpikeWidgetsLive());
}

/* The generated stdlib table references the functions above by name. */
#include "spike_stdlib.h"
