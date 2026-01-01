#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"

esp_err_t SpiffsEnsureMounted(bool format_if_mount_failed);
bool SpiffsIsMounted();

esp_err_t SpiffsReadFile(const char* path, uint8_t** out_buf, size_t* out_len);
