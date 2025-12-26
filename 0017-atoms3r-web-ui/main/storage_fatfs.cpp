/*
 * Tutorial 0017: FATFS RW storage mount for uploads.
 */

#include "storage_fatfs.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "wear_levelling.h"

static const char *TAG = "atoms3r_web_ui_0017";

static bool s_mounted = false;
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

esp_err_t storage_fatfs_mount(void) {
    if (s_mounted) {
        return ESP_OK;
    }

    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 8,
        .allocation_unit_size = 4096,
        .disk_status_check_enable = false,
        .use_one_fat = false,
    };

    ESP_LOGI(TAG, "mounting FATFS RW (WL): partition=%s at %s",
             CONFIG_TUTORIAL_0017_STORAGE_PARTITION_LABEL,
             CONFIG_TUTORIAL_0017_STORAGE_MOUNT_PATH);

    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(
        CONFIG_TUTORIAL_0017_STORAGE_MOUNT_PATH,
        CONFIG_TUTORIAL_0017_STORAGE_PARTITION_LABEL,
        &mount_config,
        &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mount failed: %s", esp_err_to_name(err));
        return err;
    }

    s_mounted = true;
    ESP_LOGI(TAG, "mount ok");
    return ESP_OK;
}

static esp_err_t ensure_dir(const char *path) {
    if (!path || !path[0]) return ESP_ERR_INVALID_ARG;
    if (mkdir(path, 0775) == 0) {
        return ESP_OK;
    }
    if (errno == EEXIST) {
        return ESP_OK;
    }
    ESP_LOGE(TAG, "mkdir(%s) failed: errno=%d", path, errno);
    return ESP_FAIL;
}

esp_err_t storage_fatfs_ensure_graphics_dir(void) {
    esp_err_t err = storage_fatfs_mount();
    if (err != ESP_OK) return err;

    // Create mount root dir is handled by VFS; ensure subdir exists.
    return ensure_dir(CONFIG_TUTORIAL_0017_STORAGE_GRAPHICS_DIR);
}

bool storage_fatfs_is_valid_filename(const char *name, size_t max_len) {
    if (!name) return false;
    const size_t n = strnlen(name, max_len + 1);
    if (n == 0 || n > max_len) return false;

    // Reject path traversal / separators.
    for (size_t i = 0; i < n; i++) {
        const char c = name[i];
        if (c == '/' || c == '\\') return false;
    }
    if (strstr(name, "..") != nullptr) return false;

    return true;
}


