// system_namespace.cpp — read-only JavaScript system metadata for 0103 AtomS3R M12.
#include "system_namespace.h"

#include <stdint.h>

#include "esp_flash.h"
#include "esp_psram.h"

namespace {
constexpr const char *kFirmwareName = "0103-atoms3r-m12-native-quickjs";
constexpr const char *kBoardName = "AtomS3R M12";
constexpr const char *kTargetName = "esp32s3";
constexpr const char *kTicketName = "ATOMS3R-M12-NATIVE-QUICKJS";
constexpr int64_t kQuickJsMemoryLimitBytes = 1 * 1024 * 1024;
constexpr int64_t kQuickJsStackLimitBytes = 64 * 1024;

bool define_string(JSContext *ctx, JSValueConst obj, const char *name, const char *value)
{
    JSValue v = JS_NewString(ctx, value);
    if (JS_IsException(v)) {
        return false;
    }
    return JS_DefinePropertyValueStr(ctx, obj, name, v, JS_PROP_ENUMERABLE) >= 0;
}

bool define_i64(JSContext *ctx, JSValueConst obj, const char *name, int64_t value)
{
    JSValue v = JS_NewInt64(ctx, value);
    if (JS_IsException(v)) {
        return false;
    }
    return JS_DefinePropertyValueStr(ctx, obj, name, v, JS_PROP_ENUMERABLE) >= 0;
}

bool define_bool(JSContext *ctx, JSValueConst obj, const char *name, bool value)
{
    JSValue v = JS_NewBool(ctx, value ? 1 : 0);
    return JS_DefinePropertyValueStr(ctx, obj, name, v, JS_PROP_ENUMERABLE) >= 0;
}

esp_err_t install_system_namespace_job(JSContext *ctx, void *user)
{
    (void)user;
    if (!ctx) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t flash_size = 0;
    if (esp_flash_get_size(nullptr, &flash_size) != ESP_OK) {
        flash_size = 0;
    }

    JSValue system = JS_NewObject(ctx);
    if (JS_IsException(system)) {
        return ESP_ERR_NO_MEM;
    }

    bool ok = define_string(ctx, system, "firmware", kFirmwareName) &&
              define_string(ctx, system, "board", kBoardName) &&
              define_string(ctx, system, "target", kTargetName) &&
              define_string(ctx, system, "ticket", kTicketName) &&
              define_bool(ctx, system, "psramInitialized", esp_psram_is_initialized()) &&
              define_i64(ctx, system, "psramBytes", (int64_t)esp_psram_get_size()) &&
              define_i64(ctx, system, "flashBytes", (int64_t)flash_size) &&
              define_i64(ctx, system, "quickjsMemoryLimitBytes", kQuickJsMemoryLimitBytes) &&
              define_i64(ctx, system, "quickjsStackLimitBytes", kQuickJsStackLimitBytes);

    if (ok && JS_PreventExtensions(ctx, system) < 0) {
        ok = false;
    }

    JSValue global = JS_GetGlobalObject(ctx);
    if (JS_IsException(global)) {
        JS_FreeValue(ctx, system);
        return ESP_FAIL;
    }

    if (ok) {
        const int rc = JS_DefinePropertyValueStr(ctx, global, "system", system, JS_PROP_ENUMERABLE);
        // JS_DefinePropertyValueStr consumes system on both success and failure.
        system = JS_UNDEFINED;
        ok = rc >= 0;
    }

    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, system);
    return ok ? ESP_OK : ESP_FAIL;
}
}  // namespace

esp_err_t install_system_namespace(qjs_service_t *svc)
{
    if (!svc) {
        return ESP_ERR_INVALID_ARG;
    }
    qjs_job_t job = {};
    job.fn = install_system_namespace_job;
    job.timeout_ms = 1000;
    return qjs_service_run(svc, &job);
}
