#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"
#include "qjs_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef esp_err_t (*storage_stream_writer_t)(const void *data, size_t len, void *user);

esp_err_t storage_namespace_start(bool format_if_mount_failed);
esp_err_t storage_namespace_validate_virtual_path(const char *virtual_path);
esp_err_t storage_namespace_stream_file(const char *virtual_path,
                                        size_t max_bytes,
                                        storage_stream_writer_t writer,
                                        void *user,
                                        size_t *out_len);
esp_err_t install_storage_namespace(qjs_service_t *svc);
void register_storage_commands(void);

#ifdef __cplusplus
}
#endif
