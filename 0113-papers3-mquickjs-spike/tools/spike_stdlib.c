/* Stdlib definition for the ESP-50 Phase 11 MicroQuickJS spike.
 *
 * Host-only generator input: compiled together with mquickjs_build.c into
 * spike_stdlib_gen, which emits main/spike_stdlib.h (32-bit table) and
 * components/mquickjs/mquickjs_atom.h. Function references here are
 * stringified symbol names — the real implementations live on the device
 * in main/spike_stdlib_runtime.c.
 *
 * Regenerate with tools/gen_spike_stdlib.sh after editing.
 */
#include <stdio.h>
#include <stdint.h>

#include "mquickjs_build.h"

/* Generation-safe opaque widget handle (task dygk): the JS wrapper stores
   a packed (generation << 16 | index) native handle; native teardown bumps
   the generation so stale wrappers throw instead of touching freed state. */
static const JSPropDef js_s3widget_proto[] = {
    JS_CGETSET_DEF("value", js_s3widget_get_value, NULL),
    JS_CFUNC_DEF("bump", 0, js_s3widget_bump),
    JS_PROP_END,
};

static const JSClassDef js_s3widget_class =
    JS_CLASS_DEF("S3Widget", 1, js_s3widget_constructor, JS_CLASS_S3WIDGET,
                 NULL, js_s3widget_proto, NULL, js_s3widget_finalizer);

#define CONFIG_SPIKE 1
#include "mqjs_stdlib_spike.c"
