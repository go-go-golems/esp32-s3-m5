/* Minimal stdlib for ESP32 - just enough to initialize the engine */
#include "mquickjs_priv.h"

static const uint32_t __attribute((aligned(4))) js_stdlib_table[] = {
    0  /* Empty table */
};

static const JSCFunctionDef js_c_function_table[] = {
};

static const JSCFinalizer js_c_finalizer_table[] = {
};

const JSSTDLibraryDef js_stdlib = {
    (const JSWord *)js_stdlib_table,
    js_c_function_table,
    js_c_finalizer_table,
    1,      /* stdlib_table_len */
    4,      /* stdlib_table_align */
    0,      /* sorted_atoms_offset */
    0,      /* global_object_offset */
    JS_CLASS_USER,  /* class_count */
};
