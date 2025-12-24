#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

// Mount the FATFS storage partition at the configured mount point.
esp_err_t echo_gif_storage_mount(void);

// Scan configured GIF directory and return up to max_paths full paths (e.g. "/storage/gifs/foo.gif").
// Returns the number of entries written to out_paths.
int echo_gif_storage_list_gifs(char *out_paths, int max_paths, int max_path_len);

// Read a file fully into RAM. Caller must free with echo_gif_storage_free().
esp_err_t echo_gif_storage_read_file(const char *path, uint8_t **out_buf, size_t *out_len);
void echo_gif_storage_free(uint8_t *buf);


