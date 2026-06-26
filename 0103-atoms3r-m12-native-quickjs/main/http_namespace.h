#pragma once

#include "esp_err.h"
#include "qjs_service.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t install_http_namespace(qjs_service_t *svc);
esp_err_t clear_http_namespace_state(qjs_service_t *svc);

#ifdef __cplusplus
}
#endif
