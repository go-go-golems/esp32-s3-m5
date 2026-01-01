#include "storage/Spiffs.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include "esp_spiffs.h"

namespace {

bool g_spiffs_mounted = false;

}  // namespace

esp_err_t SpiffsEnsureMounted(bool format_if_mount_failed) {
  if (g_spiffs_mounted) {
    return ESP_OK;
  }

  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "storage",
      .max_files = 8,
      .format_if_mount_failed = format_if_mount_failed,
  };

  const esp_err_t err = esp_vfs_spiffs_register(&conf);
  if (err == ESP_OK || err == ESP_ERR_INVALID_STATE) {
    g_spiffs_mounted = true;
    return ESP_OK;
  }
  return err;
}

bool SpiffsIsMounted() {
  return g_spiffs_mounted;
}

esp_err_t SpiffsReadFile(const char* path, uint8_t** out_buf, size_t* out_len) {
  if (!path || !out_buf || !out_len) {
    return ESP_ERR_INVALID_ARG;
  }
  *out_buf = nullptr;
  *out_len = 0;

  FILE* f = fopen(path, "rb");
  if (!f) {
    return ESP_FAIL;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return ESP_FAIL;
  }
  const long size = ftell(f);
  if (size < 0) {
    fclose(f);
    return ESP_FAIL;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return ESP_FAIL;
  }

  uint8_t* buf = static_cast<uint8_t*>(malloc(static_cast<size_t>(size) + 1));
  if (!buf) {
    fclose(f);
    return ESP_ERR_NO_MEM;
  }

  const size_t n = fread(buf, 1, static_cast<size_t>(size), f);
  fclose(f);
  if (n != static_cast<size_t>(size)) {
    free(buf);
    return ESP_FAIL;
  }

  buf[n] = 0;
  *out_buf = buf;
  *out_len = n;
  return ESP_OK;
}

