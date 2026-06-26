// http_server.cpp — minimal host-owned HTTP service for 0103 AtomS3R M12.
#include "http_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"

namespace {
constexpr const char *kTag = "0103_http";
httpd_handle_t s_server = nullptr;
uint16_t s_port = 0;

esp_err_t healthz_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    return httpd_resp_sendstr(req, "ok\n");
}

esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_sendstr(req,
                              "<!doctype html><html><head><title>AtomS3R QuickJS</title></head>"
                              "<body><h1>AtomS3R QuickJS</h1><p>Try <a href=\"/healthz\">/healthz</a>.</p></body></html>");
}

void print_usage()
{
    printf("usage:\n");
    printf("  http status\n");
    printf("  http start [port]\n");
    printf("  http stop\n");
}

bool parse_port(const char *s, uint16_t *out)
{
    if (!out) return false;
    if (!s || !*s) {
        *out = 80;
        return true;
    }
    char *end = nullptr;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0' || v <= 0 || v > 65535) {
        return false;
    }
    *out = (uint16_t)v;
    return true;
}

int cmd_http(int argc, char **argv)
{
    if (argc < 2) {
        print_usage();
        return 1;
    }
    if (strcmp(argv[1], "status") == 0) {
        printf("running=%d port=%u\n", s_server != nullptr, (unsigned)s_port);
        return 0;
    }
    if (strcmp(argv[1], "start") == 0) {
        uint16_t port = 80;
        if (argc >= 3 && !parse_port(argv[2], &port)) {
            printf("http start: invalid port: %s\n", argv[2]);
            return 1;
        }
        esp_err_t err = http_server_start(port);
        printf("http start: %s port=%u\n", esp_err_to_name(err), (unsigned)port);
        return err == ESP_OK ? 0 : 1;
    }
    if (strcmp(argv[1], "stop") == 0) {
        esp_err_t err = http_server_stop();
        printf("http stop: %s\n", esp_err_to_name(err));
        return err == ESP_OK ? 0 : 1;
    }
    print_usage();
    return 1;
}
}  // namespace

esp_err_t http_server_start(uint16_t port)
{
    if (s_server) {
        return ESP_OK;
    }
    if (port == 0) {
        port = 80;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.ctrl_port = (uint16_t)(port + 1);
    config.max_uri_handlers = 8;
    config.stack_size = 4096;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        s_server = nullptr;
        return err;
    }

    httpd_uri_t healthz = {};
    healthz.uri = "/healthz";
    healthz.method = HTTP_GET;
    healthz.handler = healthz_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &healthz));

    httpd_uri_t root = {};
    root.uri = "/";
    root.method = HTTP_GET;
    root.handler = root_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &root));

    s_port = port;
    ESP_LOGI(kTag, "HTTP server started on port %u", (unsigned)s_port);
    return ESP_OK;
}

esp_err_t http_server_stop(void)
{
    if (!s_server) {
        return ESP_OK;
    }
    esp_err_t err = httpd_stop(s_server);
    s_server = nullptr;
    s_port = 0;
    return err;
}

void register_http_commands(void)
{
    esp_console_cmd_t cmd = {};
    cmd.command = "http";
    cmd.help = "HTTP server: http status|start [port]|stop";
    cmd.func = &cmd_http;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    ESP_LOGI(kTag, "registered HTTP console commands");
}
