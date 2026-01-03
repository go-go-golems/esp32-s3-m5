#pragma once

#include <string>
#include <vector>

#include "esp_err.h"

struct SdDirEntry {
    std::string name;
    bool is_dir = false;
};

// MicroSD mount helpers (FATFS over SDSPI).
// Mount path is typically "/sd".
//
// Cardputer wiring reference (vendor demo): SDSPI pins
// - MISO=GPIO39, MOSI=GPIO14, SCK=GPIO40, CS=GPIO12
// See: ../M5Cardputer-UserDemo/main/hal/sdcard/sdcard.cpp

// Mounts the SD card at sdcard_mount_path(). Safe to call multiple times.
esp_err_t sdcard_mount(void);

// Unmounts the SD card and frees the SPI bus (best-effort).
esp_err_t sdcard_unmount(void);

bool sdcard_is_mounted(void);
const char *sdcard_mount_path(void);

// Lists entries in `abs_dir` (absolute path, e.g. "/sd" or "/sd/logs").
// Output is sorted: dirs first, then files; both lexicographically.
esp_err_t sdcard_list_dir(const char *abs_dir, std::vector<SdDirEntry> *out);

// Read up to `max_bytes` from a file into `out`. Returns ESP_OK even when the
// file is larger than max_bytes; sets `out_truncated=true` in that case.
esp_err_t sdcard_read_file_preview(const char *abs_path, size_t max_bytes, std::string *out, bool *out_truncated);
