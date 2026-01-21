#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HTTPD_WS_HUB_MAX_CLIENTS 8
#define HTTPD_WS_HUB_MAX_RX_LEN 1024

typedef esp_err_t (*httpd_ws_hub_rx_cb_t)(const uint8_t *data, size_t len);

typedef struct {
    httpd_handle_t server;
    void *mu;  // SemaphoreHandle_t
    int clients[HTTPD_WS_HUB_MAX_CLIENTS];
    size_t clients_n;
    httpd_ws_hub_rx_cb_t text_rx_cb;
    httpd_ws_hub_rx_cb_t binary_rx_cb;
} httpd_ws_hub_t;

esp_err_t httpd_ws_hub_init(httpd_ws_hub_t *hub, httpd_handle_t server);
void httpd_ws_hub_deinit(httpd_ws_hub_t *hub);

esp_err_t httpd_ws_hub_handle_req(httpd_ws_hub_t *hub, httpd_req_t *req);

esp_err_t httpd_ws_hub_broadcast_text(httpd_ws_hub_t *hub, const char *text);
esp_err_t httpd_ws_hub_broadcast_binary(httpd_ws_hub_t *hub, const uint8_t *data, size_t len);

void httpd_ws_hub_set_text_rx_cb(httpd_ws_hub_t *hub, httpd_ws_hub_rx_cb_t cb);
void httpd_ws_hub_set_binary_rx_cb(httpd_ws_hub_t *hub, httpd_ws_hub_rx_cb_t cb);

#ifdef __cplusplus
}  // extern "C"
#endif

