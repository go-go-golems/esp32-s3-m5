#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

// Mount the FATFS storage partition (label: "storage") at /storage.
esp_err_t gif_storage_mount(void);

// Scan /storage/gifs and return up to max_paths full paths (e.g. "/storage/gifs/foo.gif").
// Returns the number of entries written to out_paths.
int gif_storage_list_gifs(char *out_paths, int max_paths, int max_path_len);

// Read a file fully into RAM. Caller must free with gif_storage_free().
esp_err_t gif_storage_read_file(const char *path, uint8_t **out_buf, size_t *out_len);

void gif_storage_free(uint8_t *buf);


