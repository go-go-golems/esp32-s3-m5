/*
 * Ticket CLINTS-MEMO-WEBSITE: SPIFFS storage mount for recordings.
 */

#include "storage_spiffs.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_spiffs.h"
#include "sdkconfig.h"

static const char *TAG = "atoms3_memo_storage";

static bool s_mounted = false;
static char s_recordings_dir[128] = {0};

static esp_err_t ensure_dir_exists(const char *path) {
    struct stat st = {};
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) return ESP_OK;
        ESP_LOGE(TAG, "path exists but is not a directory: %s", path);
        return ESP_FAIL;
    }
    if (mkdir(path, 0755) != 0) {
        ESP_LOGE(TAG, "mkdir failed: %s", path);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t storage_spiffs_init(void) {
    if (s_mounted) return ESP_OK;

    const char *base_path = CONFIG_CLINTS_MEMO_STORAGE_BASE_PATH;
    const char *label = CONFIG_CLINTS_MEMO_STORAGE_PARTITION_LABEL;

    esp_vfs_spiffs_conf_t conf = {};
    conf.base_path = base_path;
    conf.partition_label = label;
    conf.max_files = CONFIG_CLINTS_MEMO_STORAGE_MAX_OPEN_FILES;
    conf.format_if_mount_failed = CONFIG_CLINTS_MEMO_STORAGE_FORMAT_IF_MOUNT_FAILED;

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_vfs_spiffs_register failed: %s", esp_err_to_name(err));
        return err;
    }

    size_t total = 0;
    size_t used = 0;
    err = esp_spiffs_info(label, &total, &used);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted: label=%s base=%s used=%u total=%u",
                 label,
                 base_path,
                 (unsigned)used,
                 (unsigned)total);
    }

    const char *rel = CONFIG_CLINTS_MEMO_RECORDINGS_DIR;
    if (snprintf(s_recordings_dir, sizeof(s_recordings_dir), "%s/%s", base_path, rel) <= 0) {
        ESP_LOGE(TAG, "failed building recordings dir path");
        return ESP_FAIL;
    }
    err = ensure_dir_exists(s_recordings_dir);
    if (err != ESP_OK) {
        // SPIFFS is often configured without directory support. If mkdir fails, fall back to
        // the mount base and treat recordings as flat files under base_path.
        ESP_LOGW(TAG, "recordings dir unavailable (%s); using base path: %s", s_recordings_dir, base_path);
        snprintf(s_recordings_dir, sizeof(s_recordings_dir), "%s", base_path);
    }

    s_mounted = true;
    return ESP_OK;
}

const char *storage_recordings_dir(void) {
    return s_recordings_dir[0] ? s_recordings_dir : "/spiffs/rec";
}
