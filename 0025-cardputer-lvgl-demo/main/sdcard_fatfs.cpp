#include "sdcard_fatfs.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_vfs_fat.h"

#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include "sdkconfig.h"

static const char *TAG = "sdcard";

namespace {

#ifndef CONFIG_0025_SDCARD_MOUNT_PATH
#define CONFIG_0025_SDCARD_MOUNT_PATH "/sd"
#endif

#ifndef CONFIG_0025_SDCARD_PIN_MISO
#define CONFIG_0025_SDCARD_PIN_MISO 39
#endif
#ifndef CONFIG_0025_SDCARD_PIN_MOSI
#define CONFIG_0025_SDCARD_PIN_MOSI 14
#endif
#ifndef CONFIG_0025_SDCARD_PIN_SCK
#define CONFIG_0025_SDCARD_PIN_SCK 40
#endif
#ifndef CONFIG_0025_SDCARD_PIN_CS
#define CONFIG_0025_SDCARD_PIN_CS 12
#endif

static bool s_mounted = false;
static bool s_bus_inited = false;
static sdmmc_card_t *s_card = nullptr;
static sdmmc_host_t s_host = SDSPI_HOST_DEFAULT();

static esp_err_t mount_impl(void) {
    s_host = SDSPI_HOST_DEFAULT();
    // Avoid clobbering any SPI bus already owned by the display (M5GFX/LovyanGFX).
    // SDSPI can use any SPI host; SPI3 is typically free on Cardputer.
    s_host.slot = SPI3_HOST;

    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = CONFIG_0025_SDCARD_PIN_MOSI;
    bus_cfg.miso_io_num = CONFIG_0025_SDCARD_PIN_MISO;
    bus_cfg.sclk_io_num = CONFIG_0025_SDCARD_PIN_SCK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.data4_io_num = -1;
    bus_cfg.data5_io_num = -1;
    bus_cfg.data6_io_num = -1;
    bus_cfg.data7_io_num = -1;
    bus_cfg.max_transfer_sz = 4000;
    bus_cfg.flags = SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MOSI;
    bus_cfg.intr_flags = 0;

    esp_err_t err = spi_bus_initialize((spi_host_device_t)s_host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        return err;
    }
    s_bus_inited = true;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = (gpio_num_t)CONFIG_0025_SDCARD_PIN_CS;
    slot_config.host_id = (spi_host_device_t)s_host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {};
    mount_cfg.format_if_mount_failed = false; // never auto-format MicroSD
    mount_cfg.max_files = 6;
    mount_cfg.allocation_unit_size = 16 * 1024;

    err = esp_vfs_fat_sdspi_mount(CONFIG_0025_SDCARD_MOUNT_PATH, &s_host, &slot_config, &mount_cfg, &s_card);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_vfs_fat_sdspi_mount(%s) failed: %s", CONFIG_0025_SDCARD_MOUNT_PATH, esp_err_to_name(err));
        (void)spi_bus_free((spi_host_device_t)s_host.slot);
        s_bus_inited = false;
        s_card = nullptr;
        return err;
    }

    s_mounted = true;
    sdmmc_card_print_info(stdout, s_card);
    return ESP_OK;
}

} // namespace

esp_err_t sdcard_mount(void) {
    if (s_mounted) return ESP_OK;
    return mount_impl();
}

esp_err_t sdcard_unmount(void) {
    if (!s_mounted) return ESP_OK;
    esp_err_t err = esp_vfs_fat_sdcard_unmount(CONFIG_0025_SDCARD_MOUNT_PATH, s_card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_vfs_fat_sdcard_unmount failed: %s", esp_err_to_name(err));
    }
    s_card = nullptr;
    s_mounted = false;

    if (s_bus_inited) {
        esp_err_t err2 = spi_bus_free((spi_host_device_t)s_host.slot);
        if (err2 != ESP_OK) {
            ESP_LOGW(TAG, "spi_bus_free failed: %s", esp_err_to_name(err2));
        }
        s_bus_inited = false;
    }

    return err;
}

bool sdcard_is_mounted(void) {
    return s_mounted;
}

const char *sdcard_mount_path(void) {
    return CONFIG_0025_SDCARD_MOUNT_PATH;
}

esp_err_t sdcard_list_dir(const char *abs_dir, std::vector<SdDirEntry> *out) {
    if (!abs_dir || !out) return ESP_ERR_INVALID_ARG;
    out->clear();

    DIR *d = opendir(abs_dir);
    if (!d) {
        const int e = errno;
        ESP_LOGW(TAG, "opendir(%s) failed: errno=%d (%s)", abs_dir, e, strerror(e));
        return ESP_FAIL;
    }

    while (true) {
        errno = 0;
        dirent *ent = readdir(d);
        if (!ent) break;

        const char *name = ent->d_name;
        if (!name || name[0] == '\0') continue;
        if (strcmp(name, ".") == 0) continue;

        SdDirEntry e;
        e.name = name;
        e.is_dir = (ent->d_type == DT_DIR);
        out->push_back(std::move(e));
    }

    closedir(d);

    std::stable_sort(out->begin(), out->end(), [](const SdDirEntry &a, const SdDirEntry &b) {
        if (a.is_dir != b.is_dir) return a.is_dir > b.is_dir;
        return a.name < b.name;
    });

    return ESP_OK;
}

esp_err_t sdcard_read_file_preview(const char *abs_path, size_t max_bytes, std::string *out, bool *out_truncated) {
    if (!abs_path || !out) return ESP_ERR_INVALID_ARG;
    out->clear();
    if (out_truncated) *out_truncated = false;

    struct stat st = {};
    if (stat(abs_path, &st) != 0) {
        const int e = errno;
        ESP_LOGW(TAG, "stat(%s) failed: errno=%d (%s)", abs_path, e, strerror(e));
        return ESP_FAIL;
    }

    FILE *f = fopen(abs_path, "rb");
    if (!f) {
        const int e = errno;
        ESP_LOGW(TAG, "fopen(%s) failed: errno=%d (%s)", abs_path, e, strerror(e));
        return ESP_FAIL;
    }

    const size_t target = std::min(max_bytes, (size_t)st.st_size);
    out->reserve(target + 64);

    char buf[512];
    size_t total = 0;
    while (total < target) {
        const size_t want = std::min(sizeof(buf), target - total);
        const size_t n = fread(buf, 1, want, f);
        if (n == 0) break;
        out->append(buf, n);
        total += n;
    }
    fclose(f);

    if ((size_t)st.st_size > max_bytes && out_truncated) {
        *out_truncated = true;
    }

    return ESP_OK;
}
