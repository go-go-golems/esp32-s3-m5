#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int status;
    char content_type[96];
    char *body;
    size_t body_len;
} http_dynamic_response_t;

typedef esp_err_t (*http_dynamic_get_handler_t)(const char *path, http_dynamic_response_t *out, void *user);

esp_err_t http_server_start(uint16_t port);
esp_err_t http_server_stop(void);
esp_err_t http_server_get_status(bool *running, uint16_t *port);
esp_err_t http_server_add_static_mount(const char *url_prefix, const char *virtual_root);
esp_err_t http_server_clear_static_mounts(void);
esp_err_t http_server_set_dynamic_get_handler(http_dynamic_get_handler_t handler, void *user);
void http_dynamic_response_free(http_dynamic_response_t *response);
void register_http_commands(void);

#ifdef __cplusplus
}
#endif
