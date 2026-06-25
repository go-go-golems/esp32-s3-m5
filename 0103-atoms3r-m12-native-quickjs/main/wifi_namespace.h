#pragma once

#include "esp_err.h"
#include "qjs_service.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t install_wifi_namespace(qjs_service_t *svc);

#ifdef __cplusplus
}
#endif
