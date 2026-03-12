#include "remote_client.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_websocket_client.h"
#include "cJSON.h"

#include "wifi_mgr.h"

namespace {

static const char* TAG = "remote_client";
constexpr TickType_t kLoopDelayTicks = pdMS_TO_TICKS(250);
constexpr uint64_t kHeartbeatIntervalMs = 5000;

SemaphoreHandle_t s_mu = nullptr;
RemoteClientStatus s_status = {};
RemoteConfig s_cfg = {};
TaskHandle_t s_task = nullptr;
esp_websocket_client_handle_t s_client = nullptr;
bool s_hello_pending = false;
QueueHandle_t s_command_q = nullptr;

uint64_t now_ms() {
  return static_cast<uint64_t>(esp_timer_get_time() / 1000LL);
}

void lock() {
  xSemaphoreTake(s_mu, portMAX_DELAY);
}

void unlock() {
  xSemaphoreGive(s_mu);
}

void set_last_error_unsafe(const char* msg) {
  std::snprintf(s_status.last_error, sizeof(s_status.last_error), "%s", msg ? msg : "");
}

void set_last_message_unsafe(const char* msg) {
  std::snprintf(s_status.last_message, sizeof(s_status.last_message), "%s", msg ? msg : "");
}

bool has_url_unsafe() {
  return s_cfg.url[0] != '\0';
}

void stop_client() {
  if (!s_client) {
    return;
  }
  esp_websocket_client_stop(s_client);
  esp_websocket_client_destroy(s_client);
  s_client = nullptr;
}

void enqueue_ui_command(const char* data_ptr, int data_len) {
  if (!s_command_q || !data_ptr || data_len <= 0) {
    return;
  }

  char json[256];
  const int copy_len = std::min<int>(data_len, sizeof(json) - 1);
  std::memcpy(json, data_ptr, static_cast<size_t>(copy_len));
  json[copy_len] = '\0';

  cJSON* root = cJSON_Parse(json);
  if (!root) {
    ESP_LOGW(TAG, "failed to parse inbound ws json");
    return;
  }

  const cJSON* type = cJSON_GetObjectItemCaseSensitive(root, "type");
  if (!cJSON_IsString(type) || std::strcmp(type->valuestring, "ui_command") != 0) {
    cJSON_Delete(root);
    return;
  }

  RemoteUiCommand cmd = {};
  const cJSON* request_id = cJSON_GetObjectItemCaseSensitive(root, "request_id");
  if (cJSON_IsNumber(request_id) && request_id->valuedouble >= 0) {
    cmd.request_id = static_cast<uint32_t>(request_id->valuedouble);
  }

  const cJSON* command = cJSON_GetObjectItemCaseSensitive(root, "command");
  if (cJSON_IsString(command)) {
    std::snprintf(cmd.command, sizeof(cmd.command), "%s", command->valuestring);
    if (std::strcmp(command->valuestring, "show_message") == 0) {
      cmd.type = RemoteUiCommandType::kShowMessage;
    } else if (std::strcmp(command->valuestring, "set_position") == 0) {
      cmd.type = RemoteUiCommandType::kSetPosition;
    } else if (std::strcmp(command->valuestring, "set_mode") == 0) {
      cmd.type = RemoteUiCommandType::kSetMode;
    } else if (std::strcmp(command->valuestring, "set_station") == 0) {
      cmd.type = RemoteUiCommandType::kSetStation;
    } else if (std::strcmp(command->valuestring, "set_band") == 0) {
      cmd.type = RemoteUiCommandType::kSetBand;
    } else if (std::strcmp(command->valuestring, "show_reveal") == 0) {
      cmd.type = RemoteUiCommandType::kShowReveal;
    }
  }

  const cJSON* text = cJSON_GetObjectItemCaseSensitive(root, "text");
  if (cJSON_IsString(text)) {
    std::snprintf(cmd.text, sizeof(cmd.text), "%s", text->valuestring);
  }

  const cJSON* value = cJSON_GetObjectItemCaseSensitive(root, "value");
  if (cJSON_IsNumber(value)) {
    cmd.value = value->valueint;
  }

  cJSON_Delete(root);

  if (cmd.type == RemoteUiCommandType::kUnknown) {
    ESP_LOGW(TAG, "ignoring unknown ui command");
    return;
  }

  if (xQueueSend(s_command_q, &cmd, 0) != pdTRUE) {
    ESP_LOGW(TAG, "ui command queue full, dropping command=%s", cmd.command);
    return;
  }

  ESP_LOGI(TAG, "queued ui command command=%s text=%s value=%" PRId32, cmd.command, cmd.text, cmd.value);
}

void websocket_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
  (void)handler_args;
  (void)base;
  auto* data = static_cast<esp_websocket_event_data_t*>(event_data);

  lock();
  switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
      s_status.state = RemoteClientState::kConnected;
      set_last_error_unsafe("");
      set_last_message_unsafe("connected");
      s_hello_pending = true;
      break;
    case WEBSOCKET_EVENT_DISCONNECTED:
      s_status.state = s_status.desired_connected ? RemoteClientState::kConnecting : RemoteClientState::kIdle;
      s_status.last_http_status = data ? data->error_handle.esp_ws_handshake_status_code : 0;
      s_status.last_socket_errno =
          (data && data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT)
              ? data->error_handle.esp_transport_sock_errno
              : 0;
      set_last_message_unsafe("disconnected");
      break;
    case WEBSOCKET_EVENT_DATA: {
      s_status.rx_count++;
      s_status.last_rx_ms = now_ms();
      if (data && data->data_ptr && data->data_len > 0) {
        const int count = std::min<int>(data->data_len, sizeof(s_status.last_message) - 1);
        std::memcpy(s_status.last_message, data->data_ptr, static_cast<size_t>(count));
        s_status.last_message[count] = '\0';
        enqueue_ui_command(static_cast<const char*>(data->data_ptr), data->data_len);
      }
      break;
    }
    case WEBSOCKET_EVENT_ERROR:
      s_status.state = RemoteClientState::kError;
      if (data) {
        s_status.last_http_status = data->error_handle.esp_ws_handshake_status_code;
        s_status.last_socket_errno = data->error_handle.esp_transport_sock_errno;
      }
      set_last_error_unsafe("websocket error");
      break;
    default:
      break;
  }
  unlock();
}

bool wifi_ready() {
  wifi_mgr_status_t wifi = {};
  if (wifi_mgr_get_status(&wifi) != ESP_OK) {
    return false;
  }
  return wifi.state == WIFI_MGR_STATE_CONNECTED && wifi.ip4 != 0;
}

void start_client_if_needed() {
  lock();
  const bool should_connect = s_status.desired_connected && has_url_unsafe();
  const bool already_has_client = s_client != nullptr;
  RemoteConfig cfg = s_cfg;
  if (!should_connect) {
    s_status.state = RemoteClientState::kIdle;
  }
  unlock();

  if (!should_connect || already_has_client || !wifi_ready()) {
    return;
  }

  esp_websocket_client_config_t ws_cfg = {};
  ws_cfg.uri = cfg.url;
  ws_cfg.network_timeout_ms = 3000;
  ws_cfg.reconnect_timeout_ms = 2000;
  ws_cfg.disable_auto_reconnect = false;

  ESP_LOGI(TAG, "connecting to %s as %s", cfg.url, cfg.device_id[0] ? cfg.device_id : "unset");
  s_client = esp_websocket_client_init(&ws_cfg);
  if (!s_client) {
    lock();
    s_status.state = RemoteClientState::kError;
    set_last_error_unsafe("client init failed");
    unlock();
    return;
  }

  esp_websocket_register_events(s_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, nullptr);
  const esp_err_t err = esp_websocket_client_start(s_client);
  bool should_stop = false;
  lock();
  if (err == ESP_OK) {
    s_status.state = RemoteClientState::kConnecting;
    set_last_message_unsafe("connect requested");
  } else {
    s_status.state = RemoteClientState::kError;
    set_last_error_unsafe(esp_err_to_name(err));
    should_stop = true;
  }
  unlock();
  if (should_stop) {
    stop_client();
  }
}

void stop_client_if_needed() {
  lock();
  const bool should_stop = !s_status.desired_connected || !has_url_unsafe() || !wifi_ready();
  if (should_stop && !wifi_ready() && s_status.desired_connected && has_url_unsafe()) {
    s_status.state = RemoteClientState::kWaitingForWifi;
    set_last_message_unsafe("waiting for wifi");
  }
  unlock();

  if (!should_stop) {
    return;
  }

  stop_client();
}

esp_err_t send_json(const char* json) {
  if (!json) {
    return ESP_ERR_INVALID_ARG;
  }
  if (!s_client || !esp_websocket_client_is_connected(s_client)) {
    return ESP_ERR_INVALID_STATE;
  }
  const int len = std::strlen(json);
  if (esp_websocket_client_send_text(s_client, json, len, pdMS_TO_TICKS(2000)) < 0) {
    return ESP_FAIL;
  }
  lock();
  s_status.tx_count++;
  s_status.last_tx_ms = now_ms();
  unlock();
  return ESP_OK;
}

void send_hello_if_needed() {
  lock();
  const bool should_send = s_hello_pending;
  RemoteConfig cfg = s_cfg;
  unlock();

  if (!should_send || !s_client || !esp_websocket_client_is_connected(s_client)) {
    return;
  }

  char json[256];
  std::snprintf(json,
                sizeof(json),
                "{\"type\":\"device_hello\",\"device_id\":\"%s\",\"device_kind\":\"m5dial\",\"protocol\":1,"
                "\"ts_ms\":%" PRIu64 "}",
                cfg.device_id,
                now_ms());

  if (send_json(json) == ESP_OK) {
    lock();
    s_hello_pending = false;
    set_last_message_unsafe("hello sent");
    unlock();
  }
}

void send_heartbeat_if_due() {
  if (!s_client || !esp_websocket_client_is_connected(s_client)) {
    return;
  }

  lock();
  const uint64_t last_tx_ms = s_status.last_tx_ms;
  RemoteConfig cfg = s_cfg;
  unlock();

  const uint64_t ms = now_ms();
  if (last_tx_ms != 0 && (ms - last_tx_ms) < kHeartbeatIntervalMs) {
    return;
  }

  char json[256];
  std::snprintf(json,
                sizeof(json),
                "{\"type\":\"heartbeat\",\"device_id\":\"%s\",\"ts_ms\":%" PRIu64 "}",
                cfg.device_id,
                ms);
  (void)send_json(json);
}

void remote_task(void* arg) {
  (void)arg;
  while (true) {
    stop_client_if_needed();
    start_client_if_needed();
    send_hello_if_needed();
    send_heartbeat_if_due();
    vTaskDelay(kLoopDelayTicks);
  }
}

}  // namespace

esp_err_t remote_client_init() {
  if (s_mu) {
    return ESP_OK;
  }
  s_mu = xSemaphoreCreateMutex();
  if (!s_mu) {
    return ESP_ERR_NO_MEM;
  }
  if (xTaskCreatePinnedToCore(remote_task, "remote_task", 6144, nullptr, 4, &s_task, 0) != pdPASS) {
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}

void remote_client_set_config(const RemoteConfig& cfg) {
  lock();
  s_cfg = cfg;
  std::snprintf(s_status.url, sizeof(s_status.url), "%s", cfg.url);
  std::snprintf(s_status.device_id, sizeof(s_status.device_id), "%s", cfg.device_id);
  unlock();

  stop_client();
}

void remote_client_set_command_queue(QueueHandle_t queue) {
  lock();
  s_command_q = queue;
  unlock();
}

esp_err_t remote_client_connect() {
  lock();
  if (!has_url_unsafe()) {
    unlock();
    return ESP_ERR_INVALID_STATE;
  }
  s_status.desired_connected = true;
  if (!wifi_ready()) {
    s_status.state = RemoteClientState::kWaitingForWifi;
  }
  unlock();
  return ESP_OK;
}

esp_err_t remote_client_disconnect() {
  lock();
  s_status.desired_connected = false;
  s_status.state = RemoteClientState::kIdle;
  unlock();
  stop_client();
  return ESP_OK;
}

void remote_client_get_status(RemoteClientStatus* out) {
  if (!out) {
    return;
  }
  lock();
  *out = s_status;
  unlock();
}

esp_err_t remote_client_send_encoder(uint32_t seq, int32_t pos, int32_t delta, uint64_t ts_ms) {
  lock();
  RemoteConfig cfg = s_cfg;
  unlock();

  char json[256];
  std::snprintf(json,
                sizeof(json),
                "{\"type\":\"encoder\",\"device_id\":\"%s\",\"seq\":%" PRIu32 ",\"ts_ms\":%" PRIu64
                ",\"pos\":%" PRId32 ",\"delta\":%" PRId32 "}",
                cfg.device_id,
                seq,
                ts_ms,
                pos,
                delta);
  return send_json(json);
}

esp_err_t remote_client_send_button(uint32_t seq, const char* kind, int32_t pos, uint64_t ts_ms) {
  lock();
  RemoteConfig cfg = s_cfg;
  unlock();

  char json[256];
  std::snprintf(json,
                sizeof(json),
                "{\"type\":\"button\",\"device_id\":\"%s\",\"seq\":%" PRIu32 ",\"ts_ms\":%" PRIu64
                ",\"kind\":\"%s\",\"pos\":%" PRId32 "}",
                cfg.device_id,
                seq,
                ts_ms,
                kind ? kind : "unknown",
                pos);
  return send_json(json);
}

esp_err_t remote_client_send_swipe(uint32_t seq, const char* direction, uint64_t ts_ms) {
  lock();
  RemoteConfig cfg = s_cfg;
  unlock();

  char json[256];
  std::snprintf(json,
                sizeof(json),
                "{\"type\":\"swipe\",\"device_id\":\"%s\",\"seq\":%" PRIu32 ",\"ts_ms\":%" PRIu64
                ",\"direction\":\"%s\"}",
                cfg.device_id,
                seq,
                ts_ms,
                direction ? direction : "unknown");
  return send_json(json);
}

esp_err_t remote_client_send_ui_ack(uint32_t seq,
                                    uint32_t request_id,
                                    const char* command,
                                    const char* status,
                                    int32_t pos,
                                    const char* text,
                                    uint64_t ts_ms) {
  lock();
  RemoteConfig cfg = s_cfg;
  unlock();

  cJSON* root = cJSON_CreateObject();
  if (!root) {
    return ESP_ERR_NO_MEM;
  }

  cJSON_AddStringToObject(root, "type", "ui_command_ack");
  cJSON_AddStringToObject(root, "device_id", cfg.device_id);
  cJSON_AddNumberToObject(root, "seq", seq);
  cJSON_AddNumberToObject(root, "request_id", request_id);
  cJSON_AddNumberToObject(root, "ts_ms", static_cast<double>(ts_ms));
  cJSON_AddStringToObject(root, "command", command ? command : "unknown");
  cJSON_AddStringToObject(root, "status", status ? status : "unknown");
  cJSON_AddNumberToObject(root, "pos", pos);
  cJSON_AddStringToObject(root, "text", text ? text : "");

  char* json = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  if (!json) {
    return ESP_ERR_NO_MEM;
  }

  const esp_err_t err = send_json(json);
  cJSON_free(json);
  return err;
}
