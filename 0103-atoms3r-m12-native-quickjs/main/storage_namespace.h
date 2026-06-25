#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "qjs_service.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t storage_namespace_start(bool format_if_mount_failed);
esp_err_t install_storage_namespace(qjs_service_t *svc);
void register_storage_commands(void);

#ifdef __cplusplus
}
#endif
