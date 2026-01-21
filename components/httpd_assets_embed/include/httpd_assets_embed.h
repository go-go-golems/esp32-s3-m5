#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t httpd_assets_embed_len(const uint8_t *start, const uint8_t *end, bool trim_trailing_nul);

esp_err_t httpd_assets_embed_send(httpd_req_t *req,
                                  const uint8_t *start,
                                  const uint8_t *end,
                                  const char *content_type,
                                  const char *cache_control,
                                  bool trim_trailing_nul);

#ifdef __cplusplus
}  // extern "C"
#endif

