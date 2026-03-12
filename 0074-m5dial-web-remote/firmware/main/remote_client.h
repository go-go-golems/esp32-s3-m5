#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_err.h"

#include "app_commands.h"
#include "remote_config.h"

enum class RemoteClientState : uint8_t {
  kIdle = 0,
  kWaitingForWifi,
  kConnecting,
  kConnected,
  kError,
};

struct RemoteClientStatus {
  RemoteClientState state = RemoteClientState::kIdle;
  bool desired_connected = false;
  char url[kRemoteUrlMaxLen] = {0};
  char device_id[kRemoteDeviceIdMaxLen] = {0};
  char last_error[96] = {0};
  char last_message[96] = {0};
  int last_http_status = 0;
  int last_socket_errno = 0;
  uint32_t tx_count = 0;
  uint32_t rx_count = 0;
  uint64_t last_tx_ms = 0;
  uint64_t last_rx_ms = 0;
};

esp_err_t remote_client_init();
void remote_client_set_config(const RemoteConfig& cfg);
void remote_client_set_app_command_queue(QueueHandle_t queue);
esp_err_t remote_client_connect();
esp_err_t remote_client_disconnect();
void remote_client_get_status(RemoteClientStatus* out);
esp_err_t remote_client_send_encoder(uint32_t seq, int32_t pos, int32_t delta, uint64_t ts_ms);
esp_err_t remote_client_send_button(uint32_t seq, const char* kind, int32_t pos, uint64_t ts_ms);
esp_err_t remote_client_send_swipe(uint32_t seq, const char* direction, uint64_t ts_ms);
esp_err_t remote_client_send_ui_ack(uint32_t seq,
                                    uint32_t request_id,
                                    const char* command,
                                    const char* status,
                                    int32_t pos,
                                    const char* text,
                                    uint64_t ts_ms);
esp_err_t remote_client_send_script_result(uint32_t request_id,
                                           const char* status,
                                           bool ok,
                                           bool timed_out,
                                           const char* output,
                                           const char* error,
                                           uint64_t ts_ms);
esp_err_t remote_client_send_script_console(uint32_t request_id,
                                            const char* level,
                                            const char* message,
                                            uint64_t ts_ms);
esp_err_t remote_client_send_script_event(uint32_t request_id,
                                          const char* event_name,
                                          const char* detail,
                                          uint64_t ts_ms);
