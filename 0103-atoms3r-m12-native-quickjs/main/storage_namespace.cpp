// storage_namespace.cpp — bounded virtual-rooted FatFs storage for 0103 AtomS3R M12.
#include "storage_namespace.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "wear_levelling.h"

namespace {
constexpr const char *kTag = "0103_storage";
constexpr const char *kMountPoint = "/storage";
constexpr const char *kPartitionLabel = "storage";
constexpr size_t kMaxPathBytes = 127;
constexpr size_t kNativePathBytes = 160;
constexpr size_t kMaxReadBytes = 16 * 1024;
constexpr size_t kMaxWriteBytes = 16 * 1024;
constexpr size_t kMaxListEntries = 64;

wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
bool s_mounted = false;
esp_err_t s_last_mount_err = ESP_ERR_INVALID_STATE;
SemaphoreHandle_t s_lock = nullptr;
StaticSemaphore_t s_lock_storage = {};

SemaphoreHandle_t storage_lock()
{
    if (!s_lock) {
        s_lock = xSemaphoreCreateMutexStatic(&s_lock_storage);
    }
    return s_lock;
}

void lock_storage()
{
    SemaphoreHandle_t lock = storage_lock();
    if (lock) {
        xSemaphoreTake(lock, portMAX_DELAY);
    }
}

void unlock_storage()
{
    if (s_lock) {
        xSemaphoreGive(s_lock);
    }
}

bool is_allowed_root(const char *path)
{
    const char *roots[] = {"/scripts", "/data", "/tmp"};
    for (const char *root : roots) {
        const size_t n = strlen(root);
        if (strncmp(path, root, n) == 0 && (path[n] == '\0' || path[n] == '/')) {
            return true;
        }
    }
    return false;
}

esp_err_t validate_virtual_path(const char *path)
{
    if (!path || path[0] != '/') {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t n = strlen(path);
    if (n == 0 || n > kMaxPathBytes) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (!is_allowed_root(path)) {
        return ESP_ERR_NOT_ALLOWED;
    }
    if (strstr(path, "//") || strstr(path, "/../") || strstr(path, "/./") ||
        strstr(path, "\\") || strstr(path, ":")) {
        return ESP_ERR_INVALID_ARG;
    }
    if (n >= 2 && strcmp(path + n - 2, "/.") == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (n >= 3 && strcmp(path + n - 3, "/..") == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t native_path_for(const char *virtual_path, char *out, size_t out_len)
{
    esp_err_t err = validate_virtual_path(virtual_path);
    if (err != ESP_OK) {
        return err;
    }
    const int n = snprintf(out, out_len, "%s%s", kMountPoint, virtual_path);
    if (n < 0 || (size_t)n >= out_len) {
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

void mkdir_if_needed(const char *path)
{
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        ESP_LOGW(kTag, "mkdir(%s) failed errno=%d", path, errno);
    }
}

esp_err_t ensure_mounted_locked(bool format_if_mount_failed)
{
    if (s_mounted) {
        return ESP_OK;
    }

    esp_vfs_fat_mount_config_t mount_config = VFS_FAT_MOUNT_DEFAULT_CONFIG();
    mount_config.format_if_mount_failed = format_if_mount_failed;
    mount_config.max_files = 6;
    mount_config.allocation_unit_size = 4096;

    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(kMountPoint, kPartitionLabel, &mount_config, &s_wl_handle);
    s_last_mount_err = err;
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "mount %s partition=%s failed: %s%s",
                 kMountPoint,
                 kPartitionLabel,
                 esp_err_to_name(err),
                 format_if_mount_failed ? " (format requested)" : "");
        return err;
    }

    s_mounted = true;
    mkdir_if_needed("/storage/scripts");
    mkdir_if_needed("/storage/data");
    mkdir_if_needed("/storage/tmp");
    ESP_LOGI(kTag, "mounted %s partition=%s", kMountPoint, kPartitionLabel);
    return ESP_OK;
}

esp_err_t storage_read_text_alloc(const char *virtual_path, size_t max_bytes, char **out, size_t *out_len)
{
    if (!out || !out_len || max_bytes == 0 || max_bytes > kMaxReadBytes) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = nullptr;
    *out_len = 0;

    lock_storage();
    if (!s_mounted) {
        unlock_storage();
        return ESP_ERR_INVALID_STATE;
    }

    char path[kNativePathBytes] = {};
    esp_err_t err = native_path_for(virtual_path, path, sizeof(path));
    if (err != ESP_OK) {
        unlock_storage();
        return err;
    }

    struct stat st = {};
    if (stat(path, &st) != 0) {
        unlock_storage();
        return ESP_ERR_NOT_FOUND;
    }
    if (!S_ISREG(st.st_mode) || st.st_size < 0 || (size_t)st.st_size > max_bytes) {
        unlock_storage();
        return ESP_ERR_INVALID_SIZE;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        unlock_storage();
        return ESP_FAIL;
    }

    char *buf = static_cast<char *>(malloc((size_t)st.st_size + 1));
    if (!buf) {
        fclose(f);
        unlock_storage();
        return ESP_ERR_NO_MEM;
    }
    const size_t got = fread(buf, 1, (size_t)st.st_size, f);
    fclose(f);
    if (got != (size_t)st.st_size) {
        free(buf);
        unlock_storage();
        return ESP_FAIL;
    }
    buf[got] = '\0';
    *out = buf;
    *out_len = got;
    unlock_storage();
    return ESP_OK;
}

esp_err_t storage_write_text(const char *virtual_path, const char *text, size_t len)
{
    if (!text || len > kMaxWriteBytes) {
        return ESP_ERR_INVALID_ARG;
    }

    lock_storage();
    if (!s_mounted) {
        unlock_storage();
        return ESP_ERR_INVALID_STATE;
    }

    char path[kNativePathBytes] = {};
    esp_err_t err = native_path_for(virtual_path, path, sizeof(path));
    if (err != ESP_OK) {
        unlock_storage();
        return err;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        unlock_storage();
        return ESP_FAIL;
    }
    const size_t wrote = fwrite(text, 1, len, f);
    const int close_rc = fclose(f);
    unlock_storage();
    return (wrote == len && close_rc == 0) ? ESP_OK : ESP_FAIL;
}

JSValue throw_esp_error(JSContext *ctx, const char *operation, esp_err_t err)
{
    return JS_ThrowInternalError(ctx, "%s: %s", operation, esp_err_to_name(err));
}

JSValue js_storage_status(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    (void)argc;
    (void)argv;
    lock_storage();
    const bool mounted = s_mounted;
    const esp_err_t last_err = s_last_mount_err;
    unlock_storage();

    JSValue obj = JS_NewObject(ctx);
    if (JS_IsException(obj)) {
        return obj;
    }
    JS_DefinePropertyValueStr(ctx, obj, "mounted", JS_NewBool(ctx, mounted), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "mountPoint", JS_NewString(ctx, kMountPoint), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "partition", JS_NewString(ctx, kPartitionLabel), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "lastMountError", JS_NewString(ctx, esp_err_to_name(last_err)), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "maxReadBytes", JS_NewInt32(ctx, (int32_t)kMaxReadBytes), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "maxWriteBytes", JS_NewInt32(ctx, (int32_t)kMaxWriteBytes), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "maxListEntries", JS_NewInt32(ctx, (int32_t)kMaxListEntries), JS_PROP_ENUMERABLE);
    return obj;
}

JSValue js_storage_read_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "storage.readText(path) requires a path");
    }
    const char *path = JS_ToCString(ctx, argv[0]);
    if (!path) {
        return JS_EXCEPTION;
    }
    char *text = nullptr;
    size_t len = 0;
    const esp_err_t err = storage_read_text_alloc(path, kMaxReadBytes, &text, &len);
    JS_FreeCString(ctx, path);
    if (err != ESP_OK) {
        return throw_esp_error(ctx, "storage.readText", err);
    }
    JSValue out = JS_NewStringLen(ctx, text, len);
    free(text);
    return out;
}

JSValue js_storage_write_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "storage.writeText(path, text) requires path and text");
    }
    const char *path = JS_ToCString(ctx, argv[0]);
    if (!path) {
        return JS_EXCEPTION;
    }
    size_t len = 0;
    const char *text = JS_ToCStringLen(ctx, &len, argv[1]);
    if (!text) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }
    esp_err_t err = ESP_OK;
    if (len > kMaxWriteBytes) {
        err = ESP_ERR_INVALID_SIZE;
    } else {
        err = storage_write_text(path, text, len);
    }
    JS_FreeCString(ctx, text);
    JS_FreeCString(ctx, path);
    if (err != ESP_OK) {
        return throw_esp_error(ctx, "storage.writeText", err);
    }
    JSValue obj = JS_NewObject(ctx);
    JS_DefinePropertyValueStr(ctx, obj, "ok", JS_NewBool(ctx, true), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "bytes", JS_NewInt32(ctx, (int32_t)len), JS_PROP_ENUMERABLE);
    return obj;
}

JSValue js_storage_stat(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "storage.stat(path) requires a path");
    }
    const char *virtual_path = JS_ToCString(ctx, argv[0]);
    if (!virtual_path) {
        return JS_EXCEPTION;
    }

    lock_storage();
    if (!s_mounted) {
        unlock_storage();
        JS_FreeCString(ctx, virtual_path);
        return throw_esp_error(ctx, "storage.stat", ESP_ERR_INVALID_STATE);
    }
    char path[kNativePathBytes] = {};
    esp_err_t err = native_path_for(virtual_path, path, sizeof(path));
    JS_FreeCString(ctx, virtual_path);
    if (err != ESP_OK) {
        unlock_storage();
        return throw_esp_error(ctx, "storage.stat", err);
    }
    struct stat st = {};
    if (stat(path, &st) != 0) {
        unlock_storage();
        return throw_esp_error(ctx, "storage.stat", ESP_ERR_NOT_FOUND);
    }
    unlock_storage();

    JSValue obj = JS_NewObject(ctx);
    JS_DefinePropertyValueStr(ctx, obj, "type", JS_NewString(ctx, S_ISDIR(st.st_mode) ? "dir" : "file"), JS_PROP_ENUMERABLE);
    JS_DefinePropertyValueStr(ctx, obj, "size", JS_NewInt64(ctx, (int64_t)st.st_size), JS_PROP_ENUMERABLE);
    return obj;
}

JSValue js_storage_list(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    (void)this_val;
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "storage.list(path) requires a path");
    }
    const char *virtual_path = JS_ToCString(ctx, argv[0]);
    if (!virtual_path) {
        return JS_EXCEPTION;
    }

    lock_storage();
    if (!s_mounted) {
        unlock_storage();
        JS_FreeCString(ctx, virtual_path);
        return throw_esp_error(ctx, "storage.list", ESP_ERR_INVALID_STATE);
    }

    char path[kNativePathBytes] = {};
    esp_err_t err = native_path_for(virtual_path, path, sizeof(path));
    JS_FreeCString(ctx, virtual_path);
    if (err != ESP_OK) {
        unlock_storage();
        return throw_esp_error(ctx, "storage.list", err);
    }

    DIR *dir = opendir(path);
    if (!dir) {
        unlock_storage();
        return throw_esp_error(ctx, "storage.list", ESP_ERR_NOT_FOUND);
    }

    JSValue arr = JS_NewArray(ctx);
    size_t n = 0;
    while (n < kMaxListEntries) {
        struct dirent *ent = readdir(dir);
        if (!ent) {
            break;
        }
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        char child[kNativePathBytes] = {};
        const int child_len = snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
        struct stat st = {};
        const bool have_stat = child_len > 0 && (size_t)child_len < sizeof(child) && stat(child, &st) == 0;
        JSValue obj = JS_NewObject(ctx);
        JS_DefinePropertyValueStr(ctx, obj, "name", JS_NewString(ctx, ent->d_name), JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, obj, "type", JS_NewString(ctx, (have_stat && S_ISDIR(st.st_mode)) ? "dir" : "file"), JS_PROP_ENUMERABLE);
        JS_DefinePropertyValueStr(ctx, obj, "size", JS_NewInt64(ctx, have_stat ? (int64_t)st.st_size : 0), JS_PROP_ENUMERABLE);
        JS_SetPropertyUint32(ctx, arr, (uint32_t)n++, obj);
    }
    closedir(dir);
    unlock_storage();
    return arr;
}

bool set_function(JSContext *ctx, JSValueConst obj, const char *name, JSCFunction *fn, int argc)
{
    JSValue f = JS_NewCFunction(ctx, fn, name, argc);
    if (JS_IsException(f)) {
        return false;
    }
    return JS_DefinePropertyValueStr(ctx, obj, name, f, JS_PROP_ENUMERABLE) >= 0;
}

esp_err_t install_storage_namespace_job(JSContext *ctx, void *user)
{
    (void)user;
    if (!ctx) {
        return ESP_ERR_INVALID_ARG;
    }
    JSValue storage = JS_NewObject(ctx);
    if (JS_IsException(storage)) {
        return ESP_ERR_NO_MEM;
    }

    bool ok = set_function(ctx, storage, "status", js_storage_status, 0) &&
              set_function(ctx, storage, "list", js_storage_list, 1) &&
              set_function(ctx, storage, "stat", js_storage_stat, 1) &&
              set_function(ctx, storage, "readText", js_storage_read_text, 1) &&
              set_function(ctx, storage, "writeText", js_storage_write_text, 2);

    if (ok && JS_PreventExtensions(ctx, storage) < 0) {
        ok = false;
    }

    JSValue global = JS_GetGlobalObject(ctx);
    if (JS_IsException(global)) {
        JS_FreeValue(ctx, storage);
        return ESP_FAIL;
    }
    if (ok) {
        const int rc = JS_DefinePropertyValueStr(ctx, global, "storage", storage, JS_PROP_ENUMERABLE);
        storage = JS_UNDEFINED;
        ok = rc >= 0;
    }
    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, storage);
    return ok ? ESP_OK : ESP_FAIL;
}

void print_storage_usage()
{
    printf("usage:\n");
    printf("  storage status\n");
    printf("  storage mount [format]\n");
    printf("  storage list <virtual-path>\n");
    printf("  storage read <virtual-path>\n");
    printf("  storage write <virtual-path> <text>\n");
}

int cmd_storage(int argc, char **argv)
{
    if (argc < 2) {
        print_storage_usage();
        return 1;
    }
    if (strcmp(argv[1], "status") == 0) {
        lock_storage();
        const bool mounted = s_mounted;
        const esp_err_t last_err = s_last_mount_err;
        unlock_storage();
        printf("mounted=%d mount=%s partition=%s last_mount=%s max_read=%u max_write=%u\n",
               mounted,
               kMountPoint,
               kPartitionLabel,
               esp_err_to_name(last_err),
               (unsigned)kMaxReadBytes,
               (unsigned)kMaxWriteBytes);
        return 0;
    }
    if (strcmp(argv[1], "mount") == 0) {
        const bool format = argc >= 3 && (strcmp(argv[2], "format") == 0 || strcmp(argv[2], "--format") == 0);
        esp_err_t err = storage_namespace_start(format);
        printf("mount: %s%s\n", esp_err_to_name(err), format ? " (format allowed)" : "");
        return err == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "list") == 0) {
        if (argc < 3) {
            print_storage_usage();
            return 1;
        }
        lock_storage();
        if (!s_mounted) {
            unlock_storage();
            printf("storage not mounted\n");
            return 1;
        }
        char path[kNativePathBytes] = {};
        esp_err_t err = native_path_for(argv[2], path, sizeof(path));
        if (err != ESP_OK) {
            unlock_storage();
            printf("path error: %s\n", esp_err_to_name(err));
            return 1;
        }
        DIR *dir = opendir(path);
        if (!dir) {
            unlock_storage();
            printf("opendir failed\n");
            return 1;
        }
        size_t n = 0;
        while (n < kMaxListEntries) {
            struct dirent *ent = readdir(dir);
            if (!ent) break;
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            printf("%s\n", ent->d_name);
            n++;
        }
        closedir(dir);
        unlock_storage();
        return 0;
    }
    if (strcmp(argv[1], "read") == 0) {
        if (argc < 3) {
            print_storage_usage();
            return 1;
        }
        char *text = nullptr;
        size_t len = 0;
        esp_err_t err = storage_read_text_alloc(argv[2], kMaxReadBytes, &text, &len);
        if (err != ESP_OK) {
            printf("read: %s\n", esp_err_to_name(err));
            return 1;
        }
        fwrite(text, 1, len, stdout);
        if (len == 0 || text[len - 1] != '\n') {
            printf("\n");
        }
        free(text);
        return 0;
    }
    if (strcmp(argv[1], "write") == 0) {
        if (argc < 4) {
            print_storage_usage();
            return 1;
        }
        esp_err_t err = storage_write_text(argv[2], argv[3], strlen(argv[3]));
        printf("write: %s\n", esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }

    print_storage_usage();
    return 1;
}
} // namespace

esp_err_t storage_namespace_start(bool format_if_mount_failed)
{
    lock_storage();
    esp_err_t err = ensure_mounted_locked(format_if_mount_failed);
    unlock_storage();
    return err;
}

esp_err_t storage_namespace_validate_virtual_path(const char *virtual_path)
{
    return validate_virtual_path(virtual_path);
}

esp_err_t storage_namespace_read_text_alloc(const char *virtual_path,
                                           size_t max_bytes,
                                           char **out,
                                           size_t *out_len)
{
    return storage_read_text_alloc(virtual_path, max_bytes, out, out_len);
}

esp_err_t storage_namespace_stream_file(const char *virtual_path,
                                        size_t max_bytes,
                                        storage_stream_writer_t writer,
                                        void *user,
                                        size_t *out_len)
{
    if (!writer || max_bytes == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (out_len) {
        *out_len = 0;
    }

    lock_storage();
    if (!s_mounted) {
        unlock_storage();
        return ESP_ERR_INVALID_STATE;
    }

    char path[kNativePathBytes] = {};
    esp_err_t err = native_path_for(virtual_path, path, sizeof(path));
    if (err != ESP_OK) {
        unlock_storage();
        return err;
    }

    struct stat st = {};
    if (stat(path, &st) != 0) {
        unlock_storage();
        return ESP_ERR_NOT_FOUND;
    }
    if (!S_ISREG(st.st_mode) || st.st_size < 0 || (size_t)st.st_size > max_bytes) {
        unlock_storage();
        return ESP_ERR_INVALID_SIZE;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        unlock_storage();
        return ESP_FAIL;
    }

    char chunk[1024];
    size_t total = 0;
    while (true) {
        const size_t got = fread(chunk, 1, sizeof(chunk), f);
        if (got > 0) {
            err = writer(chunk, got, user);
            if (err != ESP_OK) {
                fclose(f);
                unlock_storage();
                return err;
            }
            total += got;
        }
        if (got < sizeof(chunk)) {
            if (ferror(f)) {
                fclose(f);
                unlock_storage();
                return ESP_FAIL;
            }
            break;
        }
    }
    fclose(f);
    if (out_len) {
        *out_len = total;
    }
    unlock_storage();
    return total == (size_t)st.st_size ? ESP_OK : ESP_FAIL;
}

esp_err_t install_storage_namespace(qjs_service_t *svc)
{
    if (!svc) {
        return ESP_ERR_INVALID_ARG;
    }
    qjs_job_t job = {};
    job.fn = install_storage_namespace_job;
    job.timeout_ms = 1000;
    return qjs_service_run(svc, &job);
}

void register_storage_commands(void)
{
    esp_console_cmd_t cmd = {};
    cmd.command = "storage";
    cmd.help = "FatFs storage: storage status|mount [format]|list|read|write";
    cmd.func = &cmd_storage;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(kTag, "registered storage console commands");
}
