#include "gif_storage.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "wear_levelling.h"

static const char *TAG = "gif_storage";

static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
static bool s_mounted = false;

static constexpr const char *kMountPath = "/storage";
static constexpr const char *kPartitionLabel = "storage";
static constexpr const char *kGifDirPreferred = "/storage/gifs";
static constexpr const char *kGifDirFallback = "/storage";

esp_err_t gif_storage_mount(void) {
    if (s_mounted) {
        return ESP_OK;
    }

    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 0,
        .disk_status_check_enable = false,
        .use_one_fat = false,
    };

    ESP_LOGI(TAG, "mounting FATFS: partition=%s at %s", kPartitionLabel, kMountPath);
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(
        kMountPath,
        kPartitionLabel,
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

static bool has_gif_ext(const char *name) {
    if (!name) return false;
    const size_t n = strlen(name);
    if (n < 4) return false;
    const char *ext = name + (n - 4);
    // Case-insensitive ".gif"
    return (ext[0] == '.' &&
            (ext[1] == 'g' || ext[1] == 'G') &&
            (ext[2] == 'i' || ext[2] == 'I') &&
            (ext[3] == 'f' || ext[3] == 'F'));
}

int gif_storage_list_gifs(char *out_paths, int max_paths, int max_path_len) {
    if (!out_paths || max_paths <= 0 || max_path_len <= 0) {
        return 0;
    }

    if (!s_mounted) {
        const esp_err_t err = gif_storage_mount();
        if (err != ESP_OK) {
            return 0;
        }
    }

    const char *base_dir = kGifDirPreferred;
    DIR *dir = opendir(base_dir);
    if (!dir) {
        // Allow "flat" FATFS images where the GIFs are placed in the root of the storage partition.
        base_dir = kGifDirFallback;
        dir = opendir(base_dir);
        if (!dir) {
            ESP_LOGW(TAG, "opendir(%s) and fallback opendir(%s) failed: errno=%d",
                     kGifDirPreferred, kGifDirFallback, errno);
            return 0;
        }
        ESP_LOGW(TAG, "GIF dir %s not found; falling back to %s", kGifDirPreferred, base_dir);
    }

    int count = 0;
    while (count < max_paths) {
        struct dirent *ent = readdir(dir);
        if (!ent) break;
        if (ent->d_name[0] == '.') continue;
        if (!has_gif_ext(ent->d_name)) continue;

        char *dst = out_paths + (count * max_path_len);
        const int wrote = snprintf(dst, (size_t)max_path_len, "%s/%s", base_dir, ent->d_name);
        if (wrote <= 0 || wrote >= max_path_len) {
            ESP_LOGW(TAG, "path too long, skipping: %s", ent->d_name);
            continue;
        }

        count++;
    }

    closedir(dir);
    return count;
}

esp_err_t gif_storage_read_file(const char *path, uint8_t **out_buf, size_t *out_len) {
    if (!path || !out_buf || !out_len) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_buf = nullptr;
    *out_len = 0;

    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "fopen failed: %s errno=%d", path, errno);
        return ESP_FAIL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return ESP_FAIL;
    }
    const long size = ftell(f);
    if (size <= 0) {
        fclose(f);
        return ESP_FAIL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return ESP_FAIL;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)size);
    if (!buf) {
        fclose(f);
        ESP_LOGE(TAG, "malloc failed for %ld bytes", size);
        return ESP_ERR_NO_MEM;
    }

    const size_t nread = fread(buf, 1, (size_t)size, f);
    fclose(f);
    if (nread != (size_t)size) {
        free(buf);
        ESP_LOGE(TAG, "short read: want=%ld got=%u", size, (unsigned)nread);
        return ESP_FAIL;
    }

    *out_buf = buf;
    *out_len = nread;
    return ESP_OK;
}

void gif_storage_free(uint8_t *buf) {
    free(buf);
}


