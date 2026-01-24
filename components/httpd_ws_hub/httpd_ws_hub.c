#include "httpd_ws_hub.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifndef CONFIG_HTTPD_WS_SUPPORT

esp_err_t httpd_ws_hub_init(httpd_ws_hub_t *hub, httpd_handle_t server)
{
    (void)hub;
    (void)server;
    return ESP_ERR_NOT_SUPPORTED;
}

void httpd_ws_hub_deinit(httpd_ws_hub_t *hub)
{
    (void)hub;
}

void httpd_ws_hub_set_text_rx_cb(httpd_ws_hub_t *hub, httpd_ws_hub_rx_cb_t cb)
{
    (void)hub;
    (void)cb;
}

void httpd_ws_hub_set_binary_rx_cb(httpd_ws_hub_t *hub, httpd_ws_hub_rx_cb_t cb)
{
    (void)hub;
    (void)cb;
}

esp_err_t httpd_ws_hub_broadcast_text(httpd_ws_hub_t *hub, const char *text)
{
    (void)hub;
    (void)text;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t httpd_ws_hub_broadcast_binary(httpd_ws_hub_t *hub, const uint8_t *data, size_t len)
{
    (void)hub;
    (void)data;
    (void)len;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t httpd_ws_hub_handle_req(httpd_ws_hub_t *hub, httpd_req_t *req)
{
    (void)hub;
    (void)req;
    return ESP_ERR_NOT_SUPPORTED;
}

#else

static SemaphoreHandle_t hub_mu(httpd_ws_hub_t *hub)
{
    return (SemaphoreHandle_t)hub->mu;
}

static void hub_client_add(httpd_ws_hub_t *hub, int fd)
{
    SemaphoreHandle_t mu = hub_mu(hub);
    if (!mu) return;
    xSemaphoreTake(mu, portMAX_DELAY);
    for (size_t i = 0; i < hub->clients_n; i++) {
        if (hub->clients[i] == fd) {
            xSemaphoreGive(mu);
            return;
        }
    }
    if (hub->clients_n < (sizeof(hub->clients) / sizeof(hub->clients[0]))) {
        hub->clients[hub->clients_n++] = fd;
    }
    xSemaphoreGive(mu);
}

static void hub_client_remove(httpd_ws_hub_t *hub, int fd)
{
    SemaphoreHandle_t mu = hub_mu(hub);
    if (!mu) return;
    xSemaphoreTake(mu, portMAX_DELAY);
    for (size_t i = 0; i < hub->clients_n; i++) {
        if (hub->clients[i] == fd) {
            hub->clients[i] = hub->clients[hub->clients_n - 1];
            hub->clients_n--;
            break;
        }
    }
    xSemaphoreGive(mu);
}

static size_t hub_clients_snapshot(httpd_ws_hub_t *hub, int *out, size_t max_out)
{
    if (!hub || !out || max_out == 0) return 0;
    SemaphoreHandle_t mu = hub_mu(hub);
    if (!mu) return 0;
    xSemaphoreTake(mu, portMAX_DELAY);
    const size_t n = (hub->clients_n < max_out) ? hub->clients_n : max_out;
    for (size_t i = 0; i < n; i++) {
        out[i] = hub->clients[i];
    }
    xSemaphoreGive(mu);
    return n;
}

static void ws_send_free_cb(esp_err_t err, int fd, void *arg)
{
    (void)err;
    (void)fd;
    free(arg);
}

esp_err_t httpd_ws_hub_init(httpd_ws_hub_t *hub, httpd_handle_t server)
{
    if (!hub) return ESP_ERR_INVALID_ARG;
    memset(hub, 0, sizeof(*hub));
    hub->server = server;
    hub->mu = (void *)xSemaphoreCreateMutex();
    if (!hub->mu) return ESP_ERR_NO_MEM;
    return ESP_OK;
}

void httpd_ws_hub_deinit(httpd_ws_hub_t *hub)
{
    if (!hub) return;
    SemaphoreHandle_t mu = hub_mu(hub);
    if (mu) vSemaphoreDelete(mu);
    memset(hub, 0, sizeof(*hub));
}

void httpd_ws_hub_set_text_rx_cb(httpd_ws_hub_t *hub, httpd_ws_hub_rx_cb_t cb)
{
    if (!hub) return;
    hub->text_rx_cb = cb;
}

void httpd_ws_hub_set_binary_rx_cb(httpd_ws_hub_t *hub, httpd_ws_hub_rx_cb_t cb)
{
    if (!hub) return;
    hub->binary_rx_cb = cb;
}

esp_err_t httpd_ws_hub_broadcast_text(httpd_ws_hub_t *hub, const char *text)
{
    if (!hub || !hub->server) return ESP_ERR_INVALID_STATE;
    if (!text) return ESP_OK;
    const size_t len = strlen(text);
    if (len == 0) return ESP_OK;

    int fds[HTTPD_WS_HUB_MAX_CLIENTS];
    const size_t n = hub_clients_snapshot(hub, fds, sizeof(fds) / sizeof(fds[0]));
    for (size_t i = 0; i < n; i++) {
        const int fd = fds[i];
        if (httpd_ws_get_fd_info(hub->server, fd) != HTTPD_WS_CLIENT_WEBSOCKET) {
            hub_client_remove(hub, fd);
            continue;
        }

        char *copy = (char *)malloc(len);
        if (!copy) return ESP_ERR_NO_MEM;
        memcpy(copy, text, len);

        httpd_ws_frame_t frame = {};
        frame.type = HTTPD_WS_TYPE_TEXT;
        frame.payload = (uint8_t *)copy;
        frame.len = len;

        (void)httpd_ws_send_data_async(hub->server, fd, &frame, ws_send_free_cb, copy);
    }
    return ESP_OK;
}

esp_err_t httpd_ws_hub_broadcast_binary(httpd_ws_hub_t *hub, const uint8_t *data, size_t len)
{
    if (!hub || !hub->server) return ESP_ERR_INVALID_STATE;
    if (!data || len == 0) return ESP_OK;

    int fds[HTTPD_WS_HUB_MAX_CLIENTS];
    const size_t n = hub_clients_snapshot(hub, fds, sizeof(fds) / sizeof(fds[0]));
    for (size_t i = 0; i < n; i++) {
        const int fd = fds[i];
        if (httpd_ws_get_fd_info(hub->server, fd) != HTTPD_WS_CLIENT_WEBSOCKET) {
            hub_client_remove(hub, fd);
            continue;
        }

        uint8_t *copy = (uint8_t *)malloc(len);
        if (!copy) return ESP_ERR_NO_MEM;
        memcpy(copy, data, len);

        httpd_ws_frame_t frame = {};
        frame.type = HTTPD_WS_TYPE_BINARY;
        frame.payload = copy;
        frame.len = len;

        (void)httpd_ws_send_data_async(hub->server, fd, &frame, ws_send_free_cb, copy);
    }
    return ESP_OK;
}

esp_err_t httpd_ws_hub_handle_req(httpd_ws_hub_t *hub, httpd_req_t *req)
{
    if (!hub || !req || !hub->server) return ESP_ERR_INVALID_ARG;

    const int fd = httpd_req_to_sockfd(req);
    hub_client_add(hub, fd);

    // Handshake request hits here first. Subsequent WS frames arrive with a different method.
    if (req->method == HTTP_GET) {
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {};
    esp_err_t err = httpd_ws_recv_frame(req, &frame, 0);
    if (err != ESP_OK) return err;

    if (frame.type == HTTPD_WS_TYPE_CLOSE) {
        hub_client_remove(hub, fd);
        return ESP_OK;
    }

    if (frame.len > HTTPD_WS_HUB_MAX_RX_LEN) {
        return ESP_FAIL;  // closes socket
    }

    uint8_t buf[HTTPD_WS_HUB_MAX_RX_LEN];
    frame.payload = buf;
    err = httpd_ws_recv_frame(req, &frame, frame.len);
    if (err != ESP_OK) return err;

    if (frame.type == HTTPD_WS_TYPE_TEXT) {
        if (hub->text_rx_cb) (void)hub->text_rx_cb(frame.payload, frame.len);
    } else if (frame.type == HTTPD_WS_TYPE_BINARY) {
        if (hub->binary_rx_cb) (void)hub->binary_rx_cb(frame.payload, frame.len);
    }

    return ESP_OK;
}

#endif  // CONFIG_HTTPD_WS_SUPPORT
