---
Title: MicroQuickJS native extensions on ESP32 (playbook + reference manual)
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T13:56:08.581099937-05:00
WhatFor: ""
WhenToUse: ""
---

# MicroQuickJS native extensions on ESP32 (playbook + reference manual)

## Goal

Provide a **repo-accurate, copy/paste-ready** guide for extending MicroQuickJS (“mquickjs”) with **custom ESP32 / ESP-IDF functionality**: native functions, user objects/classes, callbacks, and safe patterns for FreeRTOS integration.

This is meant to answer: “Where do I put C code so JS can call it?”, “How does MicroQuickJS differ from QuickJS?”, and “What patterns are safe on a FreeRTOS device (Cardputer)?”.

## Context

### MicroQuickJS is not QuickJS (the important differences)

If you come from QuickJS:

- MicroQuickJS is **table-driven**: the JS “standard library” (global object, constructors, builtins, native bindings) is described as a **ROM-style table** (`JSSTDLibraryDef`) with:
  - `stdlib_table` (atoms + object layout),
  - `c_function_table` (native function pointers indexed by integer id),
  - `c_finalizer_table` (user-class finalizers),
  - and offsets (`sorted_atoms_offset`, `global_object_offset`).
- In this API surface, there is **no** `JS_NewCFunction()` / `JS_NewRuntime()` / `JS_NewClass()` flow. You generally expose native callable functions by **generating a stdlib header** that includes them.
- Lifetime is not “dup/free JSValue”: there is **no** `JS_FreeValue()` / `JS_DupValue()`. Long-lived references are kept via **GC reference helpers** (`JSGCRef`, `JS_AddGCRef`, `JS_DeleteGCRef`).

### Where MicroQuickJS lives in *this* repo (key paths)

All paths below are under:

- `imports/esp32-mqjs-repl/mqjs-repl/`

Key files you’ll keep jumping between:

- **Firmware embedding / FreeRTOS usage**: `main/main.c`
- **MicroQuickJS public API**: `components/mquickjs/mquickjs.h`
- **Engine implementation**: `components/mquickjs/mquickjs.c`
- **Stdlib generator (host-side)**:
  - `components/mquickjs/mquickjs_build.h` (declarative macros like `JS_CFUNC_DEF`)
  - `components/mquickjs/mquickjs_build.c` (prints generated header)
- **Generated stdlib header (already in repo)**: `main/esp_stdlib.h`
- **“How to add user classes / closures” example (upstream)**:
  - `components/mquickjs/example.c`
  - `components/mquickjs/example_stdlib.c`

### Our current firmware configuration nuance

The REPL firmware (`main/main.c`) currently uses `minimal_stdlib.h` (an empty stdlib definition) to keep things minimal. That’s fine for a toy REPL, but to expose meaningful native APIs we’ll almost certainly switch to a generated stdlib header (or generate a smaller custom one) so JS can call into C.

## Quick Reference

### 1) MicroQuickJS C API “cheat sheet” (the calls you’ll use 90% of the time)

#### Native function signatures

- **Generic function** (most common):
  - `JSValue my_func(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)`
- **Magic function** (one C body, multiple “variants” via `magic`):
  - `JSValue my_func(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, int magic)`
- **“Params/closure” function** (used with `JS_NewCFunctionParams()`):
  - `JSValue my_func(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, JSValue params)`

#### Converting arguments (JS → C)

- `JS_ToInt32(ctx, &out, argv[i])` → returns non-zero on failure
- `JS_ToUint32(ctx, &out, argv[i])`
- `JS_ToNumber(ctx, &out_double, argv[i])`
- `JS_ToCStringLen(ctx, &len, argv[i], &tmp_buf)` or `JS_ToCString(...)`
- Type checks:
  - `JS_IsString(ctx, v)`
  - `JS_IsFunction(ctx, v)`
  - `JS_IsNumber(ctx, v)` (helper)

#### Creating return values (C → JS)

- `JS_NewInt32(ctx, x)`
- `JS_NewUint32(ctx, x)`
- `JS_NewInt64(ctx, x)`
- `JS_NewFloat64(ctx, x)`
- `JS_NewString(ctx, "…")` / `JS_NewStringLen(ctx, buf, len)`
- `JS_NewObject(ctx)` / `JS_NewArray(ctx, initial_len)`

#### Error handling

- If you build a value and it’s an exception, check with `JS_IsException(val)`.
- Get the pending exception object with `JS_GetException(ctx)` and print it with `JS_PrintValueF(ctx, obj, JS_DUMP_LONG)`.
- Throw errors from C:
  - `return JS_ThrowTypeError(ctx, "message")`
  - `return JS_ThrowInternalError(ctx, "message")`
  - `return JS_EXCEPTION` when a helper already set the exception (e.g., `JS_ToInt32()` failing).

#### Calling JS functions from C (callback invocation)

MicroQuickJS uses an internal arg stack:

- Check capacity: `if (JS_StackCheck(ctx, N)) return JS_EXCEPTION;`
- Push args, then function + this, then call:
  - `JS_PushArg(ctx, arg0);`
  - `JS_PushArg(ctx, func);`
  - `JS_PushArg(ctx, JS_NULL); /* this */`
  - `return JS_Call(ctx, argc);`

### 2) Keeping JS values alive across time (GC refs)

If C stores a JS function/value for later (callbacks, timers, message handlers), you must keep it reachable:

- **Store a long-lived reference**: `JS_AddGCRef(ctx, &my_ref)` gives you a `JSValue*` slot to assign.
- **Release it**: `JS_DeleteGCRef(ctx, &my_ref)`

Example pattern (simplified from upstream timer logic):

```c
typedef struct {
    bool allocated;
    JSGCRef cb;
} Foo;

static Foo foo;

static JSValue js_register_cb(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (!JS_IsFunction(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "not a function");

    JSValue *slot = JS_AddGCRef(ctx, &foo.cb);
    *slot = argv[0];
    foo.allocated = true;
    return JS_UNDEFINED;
}
```

### 3) User objects/classes (ESP32 drivers as JS objects)

For “driver objects” (e.g. `GPIOPin`, `I2CDevice`, `WiFiStation`), the common pattern is:

- Allocate native state (`malloc`), attach it to a JS object with `JS_SetOpaque()`.
- Provide getters/setters/methods that retrieve that state with `JS_GetOpaque()`.
- Provide a finalizer in `c_finalizer_table` to `free()` the state.

Upstream example reference: `components/mquickjs/example.c`.

Constructor rules:

- Constructors receive `argc` with a constructor bit:
  - if `!(argc & FRAME_CF_CTOR)`: throw “must be called with new”
  - `argc &= ~FRAME_CF_CTOR;` before reading argv normally.

### 4) The stdlib generator pipeline (how “native bindings” actually become callable)

#### What gets generated

The stdlib generator emits a C header that contains:

- `static const JSWord js_stdlib_table[] = { ... }`
- `static const JSCFunctionDef js_c_function_table[] = { ... }`
- `static const JSCFinalizer js_c_finalizer_table[] = { ... }`
- `const JSSTDLibraryDef js_stdlib = { ... }`

You can see a real generated artifact in:

- `main/esp_stdlib.h` (search for `const JSSTDLibraryDef js_stdlib`)

#### How it’s generated

At a high level:

- `mquickjs_build.h` provides declarative macros (`JS_CFUNC_DEF`, `JS_CLASS_DEF`, `JS_OBJECT_DEF`, …).
- You write a “stdlib definition C file” that declares:
  - global object properties (what exists on `globalThis`)
  - classes and their methods/getters
  - any extra “C-closure only” functions
- `mquickjs_build.c` walks those declarations and prints the generated tables.

In this repo the generator inputs already exist (but aren’t built for ESP-IDF):

- `main/esp_stdlib.c` (adds a few extra functions, then includes `mqjs_stdlib.c`)
- `main/esp_stdlib_simple.c` (minimal-ish variant)
- `main/mqjs_stdlib.c` (declarative global object/classes for generation)

### 5) Step-by-step: adding a new ESP32-native API (recommended workflow)

This is the workflow that matches MicroQuickJS’ design:

1. **Pick the JS surface**
   - Prefer a single namespace object, e.g. `ESP.*` (avoids polluting globals).
2. **Implement the native functions/classes in C**
   - Pure functions: `JSValue js_esp_heapFree(...)`
   - Stateful drivers: user classes with `JS_NewObjectClassUser()` and a finalizer.
3. **Declare them in the stdlib definition**
   - Add a `JS_OBJECT_DEF("ESP", ...)` and attach it in the global object with `JS_PROP_CLASS_DEF("ESP", &js_esp_obj)`.
4. **Regenerate the stdlib header**
   - Build/run the generator so it prints a new header (replace `main/esp_stdlib.h` or generate a new `cardputer_stdlib.h`).
5. **Use the new stdlib on device**
   - In firmware, include the generated header and pass its `&js_stdlib` into `JS_NewContext()`.
6. **Add an integration test surface**
   - At minimum: a tiny JS snippet that calls the binding and prints output (REPL or scripted autoload).

### 6) FreeRTOS safety rules (for native bindings)

These rules keep you out of deadlocks and heisenbugs:

- **One `JSContext` must be owned by exactly one task**.
  - Other tasks must not call MicroQuickJS APIs directly; they should enqueue work/messages.
- **Never call JS from a finalizer** (explicitly warned in the API: “no JS function call be called from a C finalizer”).
- **Bound execution time**:
  - Use `JS_SetInterruptHandler()` + a watchdog/timeout check for long-running scripts.
- **Avoid dynamic allocations in hot paths**:
  - Prefer fixed-size buffers or preallocated pools for message passing and IO.

## Usage Examples

### Example A: Expose a simple “ESP.heapFree()” function

#### C function implementation (device-side)

```c
static JSValue js_esp_heapFree(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val; (void)argc; (void)argv;
    return JS_NewUint32(ctx, (uint32_t)esp_get_free_heap_size());
}
```

#### Stdlib declaration snippet (host-side generator input)

```c
static const JSPropDef js_esp[] = {
    JS_CFUNC_DEF("heapFree", 0, js_esp_heapFree),
    JS_PROP_END,
};

static const JSClassDef js_esp_obj = JS_OBJECT_DEF("ESP", js_esp);

// In the global object list:
// JS_PROP_CLASS_DEF("ESP", &js_esp_obj),
```

#### JS usage

```js
ESP.heapFree()
```

### Example B: A user object for a peripheral handle (“GPIOPin”)

Use this when you need state + cleanup:

```c
#define JS_CLASS_GPIO_PIN (JS_CLASS_USER + 0)
#define JS_CLASS_COUNT    (JS_CLASS_USER + 1)

typedef struct {
    int pin;
} GPIOPin;

static JSValue js_gpioPin_ctor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (!(argc & FRAME_CF_CTOR))
        return JS_ThrowTypeError(ctx, "must be called with new");
    argc &= ~FRAME_CF_CTOR;

    int pin;
    if (JS_ToInt32(ctx, &pin, argv[0]))
        return JS_EXCEPTION;

    JSValue obj = JS_NewObjectClassUser(ctx, JS_CLASS_GPIO_PIN);
    GPIOPin *p = malloc(sizeof(*p));
    p->pin = pin;
    JS_SetOpaque(ctx, obj, p);
    return obj;
}

static void js_gpioPin_finalizer(JSContext *ctx, void *opaque) {
    (void)ctx;
    free((GPIOPin *)opaque);
}
```

This pattern (constructor + finalizer) is shown end-to-end in `components/mquickjs/example.c`.

### Example C: Cross-task message passing (design sketch)

This is the safe pattern for FreeRTOS:

- C tasks produce messages → enqueue to the owning JS task
- JS task drains queue → calls a stored JS callback (kept alive via `JSGCRef`)

This design is expanded in:

- `design-doc/01-microquickjs-freertos-multi-vm-multi-task-architecture-communication-brainstorm.md`

## Related

- Ticket design brainstorm: `../design-doc/01-microquickjs-freertos-multi-vm-multi-task-architecture-communication-brainstorm.md`
- Existing imported docs (broad background, not repo-specific):
  - `imports/ESP32_MicroQuickJS_Playbook.md`
  - `imports/MicroQuickJS_ESP32_Complete_Guide.md`
- Code entry points:
  - Firmware: `imports/esp32-mqjs-repl/mqjs-repl/main/main.c`
  - Engine API: `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`
  - Stdlib generator: `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.{h,c}`
