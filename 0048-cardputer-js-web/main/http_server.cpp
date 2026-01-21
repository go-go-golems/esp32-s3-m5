/*
 * 0048: HTTP server (esp_http_server) + REST JS eval.
 *
 * Patterned after: esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp
 */

#include "http_server.h"

#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"

#include "encoder_telemetry.h"
#include "js_runner.h"
#include "lwip/inet.h"

#include "wifi_mgr.h"

static const char* TAG = "http_server_0048";

static httpd_handle_t s_server = nullptr;

extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");
extern const uint8_t assets_app_js_start[] asm("_binary_app_js_start");
extern const uint8_t assets_app_js_end[] asm("_binary_app_js_end");
extern const uint8_t assets_app_css_start[] asm("_binary_app_css_start");
extern const uint8_t assets_app_css_end[] asm("_binary_app_css_end");

#if CONFIG_HTTPD_WS_SUPPORT
static SemaphoreHandle_t s_ws_mu = nullptr;
static int s_ws_clients[8] = {};
static size_t s_ws_clients_n = 0;

static void ws_client_add(int fd) {
  if (!s_ws_mu) return;
  xSemaphoreTake(s_ws_mu, portMAX_DELAY);
  for (size_t i = 0; i < s_ws_clients_n; i++) {
    if (s_ws_clients[i] == fd) {
      xSemaphoreGive(s_ws_mu);
      return;
    }
  }
  if (s_ws_clients_n < (sizeof(s_ws_clients) / sizeof(s_ws_clients[0]))) {
    s_ws_clients[s_ws_clients_n++] = fd;
  }
  xSemaphoreGive(s_ws_mu);
}

static void ws_client_remove(int fd) {
  if (!s_ws_mu) return;
  xSemaphoreTake(s_ws_mu, portMAX_DELAY);
  for (size_t i = 0; i < s_ws_clients_n; i++) {
    if (s_ws_clients[i] == fd) {
      s_ws_clients[i] = s_ws_clients[s_ws_clients_n - 1];
      s_ws_clients_n--;
      break;
    }
  }
  xSemaphoreGive(s_ws_mu);
}

static size_t ws_clients_snapshot(int* out, size_t max_out) {
  if (!out || max_out == 0 || !s_ws_mu) return 0;
  xSemaphoreTake(s_ws_mu, portMAX_DELAY);
  const size_t n = (s_ws_clients_n < max_out) ? s_ws_clients_n : max_out;
  for (size_t i = 0; i < n; i++) {
    out[i] = s_ws_clients[i];
  }
  xSemaphoreGive(s_ws_mu);
  return n;
}

static void ws_send_free_cb(esp_err_t err, int fd, void* arg) {
  (void)err;
  (void)fd;
  free(arg);
}

esp_err_t http_server_ws_broadcast_text(const char* text) {
  if (!s_server) return ESP_ERR_INVALID_STATE;
  if (!text) return ESP_OK;
  const size_t len = strlen(text);
  if (len == 0) return ESP_OK;

  int fds[8];
  const size_t n = ws_clients_snapshot(fds, sizeof(fds) / sizeof(fds[0]));
  for (size_t i = 0; i < n; i++) {
    const int fd = fds[i];
    if (httpd_ws_get_fd_info(s_server, fd) != HTTPD_WS_CLIENT_WEBSOCKET) {
      ws_client_remove(fd);
      continue;
    }

    char* copy = (char*)malloc(len);
    if (!copy) {
      return ESP_ERR_NO_MEM;
    }
    memcpy(copy, text, len);

    httpd_ws_frame_t frame = {};
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.payload = (uint8_t*)copy;
    frame.len = len;

    (void)httpd_ws_send_data_async(s_server, fd, &frame, ws_send_free_cb, copy);
  }
  return ESP_OK;
}

static esp_err_t ws_handler(httpd_req_t* req) {
  const int fd = httpd_req_to_sockfd(req);
  ws_client_add(fd);

  // The handshake request is an HTTP GET, but subsequent WS frames arrive with a different method.
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "ws connected: fd=%d", fd);
    return ESP_OK;
  }

  httpd_ws_frame_t frame = {};
  esp_err_t err = httpd_ws_recv_frame(req, &frame, 0);
  if (err != ESP_OK) return err;

  // We currently don't accept payloads from the browser; only handle close/control frames.
  if (frame.type == HTTPD_WS_TYPE_CLOSE) {
    ESP_LOGI(TAG, "ws closed: fd=%d", fd);
    ws_client_remove(fd);
    return ESP_OK;
  }

  if (frame.len > 1024) {
    ESP_LOGW(TAG, "ws frame too large: fd=%d len=%u", fd, (unsigned)frame.len);
    return ESP_FAIL; // closes socket
  }
  if (frame.len > 0) {
    uint8_t tmp[1024];
    frame.payload = tmp;
    err = httpd_ws_recv_frame(req, &frame, frame.len);
    if (err != ESP_OK) return err;
  }

  return ESP_OK;
}
#else
esp_err_t http_server_ws_broadcast_text(const char* text) {
  (void)text;
  return ESP_ERR_NOT_SUPPORTED;
}
#endif

static esp_err_t send_json(httpd_req_t* req, const char* body) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "cache-control", "no-store");
  return httpd_resp_sendstr(req, body);
}

static esp_err_t root_get(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "cache-control", "no-store");
  const size_t len = (size_t)(assets_index_html_end - assets_index_html_start);
  return httpd_resp_send(req, (const char*)assets_index_html_start, len);
}

static esp_err_t asset_app_js_get(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/javascript");
  httpd_resp_set_hdr(req, "cache-control", "no-store");
  const size_t len = (size_t)(assets_app_js_end - assets_app_js_start);
  return httpd_resp_send(req, (const char*)assets_app_js_start, len);
}

static esp_err_t asset_app_css_get(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/css");
  httpd_resp_set_hdr(req, "cache-control", "no-store");
  const size_t len = (size_t)(assets_app_css_end - assets_app_css_start);
  return httpd_resp_send(req, (const char*)assets_app_css_start, len);
}

static esp_err_t status_get(httpd_req_t* req) {
  wifi_mgr_status_t st = {};
  if (wifi_mgr_get_status(&st) != ESP_OK) {
    return send_json(req, "{\"ok\":false,\"error\":\"wifi status unavailable\"}");
  }

  const char* state = "?";
  switch (st.state) {
    case WIFI_MGR_STATE_UNINIT:
      state = "UNINIT";
      break;
    case WIFI_MGR_STATE_IDLE:
      state = "IDLE";
      break;
    case WIFI_MGR_STATE_CONNECTING:
      state = "CONNECTING";
      break;
    case WIFI_MGR_STATE_CONNECTED:
      state = "CONNECTED";
      break;
  }

  char ip[16] = "0.0.0.0";
  if (st.ip4) {
    ip4_addr_t ip4 = {.addr = htonl(st.ip4)};
    snprintf(ip, sizeof(ip), IPSTR, IP2STR(&ip4));
  }
  char buf[256];
  snprintf(buf,
           sizeof(buf),
           "{\"ok\":true,\"mode\":\"sta\",\"state\":\"%s\",\"ssid\":\"%s\",\"ip\":\"%s\",\"saved\":%s,\"runtime\":%s,\"reason\":%d}",
           state,
           st.ssid[0] ? st.ssid : "",
           ip,
           st.has_saved_creds ? "true" : "false",
           st.has_runtime_creds ? "true" : "false",
           st.last_disconnect_reason);
  return send_json(req, buf);
}

static esp_err_t js_eval_post(httpd_req_t* req) {
  const int max_body = CONFIG_TUTORIAL_0048_JS_MAX_BODY;
  if (max_body <= 0) {
    return send_json(req, "{\"ok\":false,\"output\":\"\",\"error\":\"server misconfigured\",\"timed_out\":false}");
  }
  if (req->content_len <= 0) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");
    return ESP_OK;
  }
  if (req->content_len > max_body) {
    httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "body too large");
    return ESP_OK;
  }

  const size_t n = (size_t)req->content_len;
  char* buf = (char*)malloc(n + 1);
  if (!buf) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
    return ESP_OK;
  }
  size_t off = 0;
  while (off < n) {
    int got = httpd_req_recv(req, buf + off, (n - off));
    if (got <= 0) {
      free(buf);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv failed");
      return ESP_OK;
    }
    off += (size_t)got;
  }
  buf[n] = 0;

  std::string json = js_runner_eval_to_json(buf, n);
  free(buf);
  return send_json(req, json.c_str());
}

esp_err_t http_server_start(void) {
  if (s_server) return ESP_OK;

  ESP_ERROR_CHECK(js_runner_init());

  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "starting http server on port %d", cfg.server_port);
  esp_err_t err = httpd_start(&s_server, &cfg);
  if (err != ESP_OK) {
    s_server = nullptr;
    return err;
  }

#if CONFIG_HTTPD_WS_SUPPORT
  if (!s_ws_mu) {
    s_ws_mu = xSemaphoreCreateMutex();
  }
#endif

  httpd_uri_t root = {};
  root.uri = "/";
  root.method = HTTP_GET;
  root.handler = root_get;
  httpd_register_uri_handler(s_server, &root);

  httpd_uri_t app_js = {};
  app_js.uri = "/assets/app.js";
  app_js.method = HTTP_GET;
  app_js.handler = asset_app_js_get;
  httpd_register_uri_handler(s_server, &app_js);

  httpd_uri_t app_css = {};
  app_css.uri = "/assets/app.css";
  app_css.method = HTTP_GET;
  app_css.handler = asset_app_css_get;
  httpd_register_uri_handler(s_server, &app_css);

  httpd_uri_t status = {};
  status.uri = "/api/status";
  status.method = HTTP_GET;
  status.handler = status_get;
  httpd_register_uri_handler(s_server, &status);

  httpd_uri_t eval = {};
  eval.uri = "/api/js/eval";
  eval.method = HTTP_POST;
  eval.handler = js_eval_post;
  httpd_register_uri_handler(s_server, &eval);

#if CONFIG_HTTPD_WS_SUPPORT
  httpd_uri_t ws = {};
  ws.uri = "/ws";
  ws.method = HTTP_GET;
  ws.handler = ws_handler;
  ws.is_websocket = true;
  ws.handle_ws_control_frames = true;
  httpd_register_uri_handler(s_server, &ws);
#endif

  // Phase 2: encoder telemetry over WebSocket (best-effort; logs on failure).
  (void)encoder_telemetry_start();

  return ESP_OK;
}
