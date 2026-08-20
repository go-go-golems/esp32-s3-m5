// files singleton bindings (ESP-53 P2). Async verbs register the module
// completion callback, run the bounded op synchronously on the owner, and
// let the posted ModuleDone event deliver fn(kind, value, err) on a later
// loop pass. Accessors read the mailboxes.
//
// GC-safety note (write/append): the body can be 16 KiB, far beyond any
// stack buffer, so the C string pointer from JS_ToCStringLen is passed
// straight into the op. That is safe ONLY because the callback was
// registered (the last allocating call) BEFORE the string was fetched and
// the op itself never calls into JS. Keep that order.
#include "app_files.h"
#include "app_js_internal.h"

namespace pulp {

using namespace jsi;

namespace {


// Shared shape for list/read/remove: (path, fn).
JSValue PathOp(JSContext *ctx, int argc, JSValue *argv,
               FilesStatusCode (*op)(const char *)) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "(path, fn) expected");
    }
    char path[kFilesMaxPath];
    JSValue err;
    if (!ArgString(ctx, argv[0], path, sizeof(path), &err)) {
        return err;
    }
    if (!RegisterModuleCb(ctx, ModuleId::Files, argv[1], &err)) {
        return err;
    }
    const FilesStatusCode status = op(path);
    if (status != FilesStatusCode::Ok) {
        CancelModuleCb(ModuleId::Files);
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(status));
}

// Shared shape for write/append: (path, body, fn).
JSValue BodyOp(JSContext *ctx, int argc, JSValue *argv,
               FilesStatusCode (*op)(const char *, const char *,
                                     uint32_t)) {
    if (argc < 3) {
        return JS_ThrowTypeError(ctx, "(path, body, fn) expected");
    }
    char path[kFilesMaxPath];
    JSValue err;
    if (!ArgString(ctx, argv[0], path, sizeof(path), &err)) {
        return err;
    }
    if (!RegisterModuleCb(ctx, ModuleId::Files, argv[2], &err)) {
        return err;
    }
    // No allocating JS call between here and op() — see header note.
    JSCStringBuf buf;
    size_t len;
    const char *body = JS_ToCStringLen(ctx, &len, argv[1], &buf);
    if (body == nullptr) {
        CancelModuleCb(ModuleId::Files);
        return JS_EXCEPTION;
    }
    const FilesStatusCode status =
        op(path, body, static_cast<uint32_t>(len));
    if (status != FilesStatusCode::Ok) {
        CancelModuleCb(ModuleId::Files);
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(status));
}

}  // namespace

extern "C" {

JSValue js_files_exists(JSContext *ctx, JSValue *, int argc,
                        JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "exists(path)");
    }
    char path[kFilesMaxPath];
    JSValue err;
    if (!ArgString(ctx, argv[0], path, sizeof(path), &err)) {
        return err;
    }
    return JS_NewInt32(ctx, FilesExists(path));
}

JSValue js_files_list(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    return PathOp(ctx, argc, argv, &FilesList);
}

JSValue js_files_read(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    return PathOp(ctx, argc, argv, &FilesRead);
}

JSValue js_files_remove(JSContext *ctx, JSValue *, int argc,
                        JSValue *argv) {
    return PathOp(ctx, argc, argv, &FilesRemove);
}

JSValue js_files_write(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    return BodyOp(ctx, argc, argv, &FilesWrite);
}

JSValue js_files_append(JSContext *ctx, JSValue *, int argc,
                        JSValue *argv) {
    return BodyOp(ctx, argc, argv, &FilesAppend);
}

JSValue js_files_name(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    const char *name = FilesListName(static_cast<uint32_t>(index));
    if (index < 0 || name == nullptr) {
        return JS_ThrowTypeError(ctx, "list index out of range");
    }
    return JS_NewString(ctx, name);
}

JSValue js_files_size(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    return JS_NewInt32(ctx,
                       FilesListSize(static_cast<uint32_t>(index)));
}

JSValue js_files_is_dir(JSContext *ctx, JSValue *, int argc,
                        JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    return JS_NewInt32(ctx,
                       FilesListIsDir(static_cast<uint32_t>(index)));
}

JSValue js_files_line_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(ctx, static_cast<int32_t>(FilesLineCount()));
}

JSValue js_files_line(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    uint32_t len = 0;
    const char *line =
        index >= 0 ? FilesLine(static_cast<uint32_t>(index), &len)
                   : nullptr;
    if (line == nullptr) {
        return JS_ThrowTypeError(ctx, "line index out of range");
    }
    return JS_NewStringLen(ctx, line, len);
}

}  // extern "C"

}  // namespace pulp
