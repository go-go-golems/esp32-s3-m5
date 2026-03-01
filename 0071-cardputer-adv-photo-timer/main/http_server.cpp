#include "http_server.h"

#include <stdlib.h>
#include <string.h>

#include <string>

#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "app_state.h"
#include "httpd_assets_embed.h"
#include "preset_store.h"
#include "sdkconfig.h"

namespace {

static const char* TAG = "photo_http_0071";
static httpd_handle_t s_server = nullptr;

extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");

esp_err_t send_json(httpd_req_t* req, const char* body) {
  httpd_resp_set_type(req, "application/json; charset=utf-8");
  httpd_resp_set_hdr(req, "cache-control", "no-store");
  return httpd_resp_sendstr(req, body ? body : "{}");
}

esp_err_t send_json_obj(httpd_req_t* req, cJSON* root) {
  if (!root) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
  char* body = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  if (!body) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "json encode failed");
  esp_err_t err = send_json(req, body);
  cJSON_free(body);
  return err;
}

bool read_body(httpd_req_t* req, std::string* out) {
  if (!req || !out) return false;
  if (req->content_len <= 0 || req->content_len > CONFIG_PHOTO_TIMER_HTTP_MAX_BODY) return false;

  out->assign((size_t)req->content_len, '\0');
  size_t off = 0;
  const size_t n = (size_t)req->content_len;
  while (off < n) {
    const int got = httpd_req_recv(req, out->data() + off, n - off);
    if (got <= 0) return false;
    off += (size_t)got;
  }
  return true;
}

esp_err_t root_get(httpd_req_t* req) {
  return httpd_assets_embed_send(req,
                                 assets_index_html_start,
                                 assets_index_html_end,
                                 "text/html; charset=utf-8",
                                 "no-store",
                                 true);
}

const char* state_to_str(TimerRunState state) {
  switch (state) {
    case TimerRunState::kIdle:
      return "IDLE";
    case TimerRunState::kRunning:
      return "RUNNING";
    case TimerRunState::kPaused:
      return "PAUSED";
    case TimerRunState::kComplete:
      return "COMPLETE";
  }
  return "?";
}

esp_err_t status_get(httpd_req_t* req) {
  const TimerSnapshot snap = app_state_snapshot();

  cJSON* root = cJSON_CreateObject();
  cJSON_AddBoolToObject(root, "ok", true);
  cJSON_AddStringToObject(root, "state", state_to_str(snap.state));
  cJSON_AddBoolToObject(root, "has_preset", snap.has_preset);
  cJSON_AddStringToObject(root, "preset_id", snap.preset_id.c_str());
  cJSON_AddStringToObject(root, "preset_name", snap.preset_name.c_str());
  cJSON_AddNumberToObject(root, "step_index", snap.step_index);
  cJSON_AddNumberToObject(root, "step_count", snap.step_count);
  cJSON_AddStringToObject(root, "step_name", snap.step_name.c_str());
  cJSON_AddNumberToObject(root, "step_total_sec", snap.step_total_sec);
  cJSON_AddNumberToObject(root, "step_remaining_sec", snap.step_remaining_sec);

  return send_json_obj(req, root);
}

esp_err_t presets_get(httpd_req_t* req) {
  const TimerConfig cfg = app_state_config_copy();
  const std::string json = preset_store_to_json(cfg);
  return send_json(req, json.c_str());
}

esp_err_t presets_post(httpd_req_t* req) {
  std::string body;
  if (!read_body(req, &body)) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
  }

  TimerConfig parsed;
  std::string err_detail;
  const esp_err_t st = preset_store_parse_json(body.data(), body.size(), &parsed, &err_detail);
  if (st != ESP_OK) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "ok", false);
    cJSON_AddStringToObject(root, "error", err_detail.empty() ? "invalid preset JSON" : err_detail.c_str());
    return send_json_obj(req, root);
  }

  if (app_state_replace_config(&parsed) != ESP_OK) {
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "replace config failed");
  }
  if (preset_store_save(parsed) != ESP_OK) {
    return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "save config failed");
  }

  cJSON* root = cJSON_CreateObject();
  cJSON_AddBoolToObject(root, "ok", true);
  cJSON_AddStringToObject(root, "path", preset_store_path());
  cJSON_AddNumberToObject(root, "preset_count", (double)parsed.presets.size());
  return send_json_obj(req, root);
}

esp_err_t control_post(httpd_req_t* req) {
  std::string body;
  if (!read_body(req, &body)) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid body");
  }

  cJSON* root = cJSON_ParseWithLength(body.data(), body.size());
  if (!root) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid json");
  }

  const cJSON* j_action = cJSON_GetObjectItemCaseSensitive(root, "action");
  const cJSON* j_preset_id = cJSON_GetObjectItemCaseSensitive(root, "preset_id");

  if (!cJSON_IsString(j_action) || !j_action->valuestring) {
    cJSON_Delete(root);
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing action");
  }

  const char* preset_id = (cJSON_IsString(j_preset_id) && j_preset_id->valuestring) ? j_preset_id->valuestring : nullptr;
  const esp_err_t st = app_state_timer_action(j_action->valuestring, preset_id);
  cJSON_Delete(root);

  if (st != ESP_OK) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "action failed");
  }

  return status_get(req);
}

}  // namespace

esp_err_t photo_http_server_start(void) {
  if (s_server) return ESP_OK;

  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "starting http server on port %d", cfg.server_port);
  esp_err_t err = httpd_start(&s_server, &cfg);
  if (err != ESP_OK) {
    s_server = nullptr;
    return err;
  }

  httpd_uri_t root = {};
  root.uri = "/";
  root.method = HTTP_GET;
  root.handler = root_get;
  httpd_register_uri_handler(s_server, &root);

  httpd_uri_t status = {};
  status.uri = "/api/status";
  status.method = HTTP_GET;
  status.handler = status_get;
  httpd_register_uri_handler(s_server, &status);

  httpd_uri_t presets_get_uri = {};
  presets_get_uri.uri = "/api/presets";
  presets_get_uri.method = HTTP_GET;
  presets_get_uri.handler = presets_get;
  httpd_register_uri_handler(s_server, &presets_get_uri);

  httpd_uri_t presets_post_uri = {};
  presets_post_uri.uri = "/api/presets";
  presets_post_uri.method = HTTP_POST;
  presets_post_uri.handler = presets_post;
  httpd_register_uri_handler(s_server, &presets_post_uri);

  httpd_uri_t control_post_uri = {};
  control_post_uri.uri = "/api/control";
  control_post_uri.method = HTTP_POST;
  control_post_uri.handler = control_post;
  httpd_register_uri_handler(s_server, &control_post_uri);

  return ESP_OK;
}
