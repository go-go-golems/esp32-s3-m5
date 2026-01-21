#include "encoder_telemetry.h"

#include <inttypes.h>
#include <stdio.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

#include "http_server.h"

#if CONFIG_TUTORIAL_0048_ENCODER_CHAIN_UART
#include "chain_encoder_uart.h"
#endif

static const char* TAG = "enc_telemetry_0048";

static TaskHandle_t s_task = nullptr;

#if CONFIG_TUTORIAL_0048_ENCODER_CHAIN_UART
static void telemetry_task(void* arg) {
  (void)arg;

  ChainEncoderUart::Config cfg = {};
  cfg.uart_num = CONFIG_TUTORIAL_0048_CHAIN_ENCODER_UART_NUM;
  cfg.baud = CONFIG_TUTORIAL_0048_CHAIN_ENCODER_BAUD;
  cfg.tx_gpio = CONFIG_TUTORIAL_0048_CHAIN_ENCODER_TX_GPIO;
  cfg.rx_gpio = CONFIG_TUTORIAL_0048_CHAIN_ENCODER_RX_GPIO;
  cfg.index_id = (uint8_t)CONFIG_TUTORIAL_0048_CHAIN_ENCODER_INDEX_ID;
  cfg.poll_ms = CONFIG_TUTORIAL_0048_CHAIN_ENCODER_POLL_MS;

  ChainEncoderUart enc(cfg);
  if (!enc.init()) {
    ESP_LOGW(TAG, "encoder init failed; telemetry will be silent");
    s_task = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  int32_t pos = 0;
  int32_t last_sent_pos = 0;
  uint32_t seq = 0;

  const int broadcast_ms = CONFIG_TUTORIAL_0048_ENCODER_WS_BROADCAST_MS;
  TickType_t last_wake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(broadcast_ms > 0 ? broadcast_ms : 50);

  while (true) {
    const int32_t delta_accum = enc.take_delta();
    if (delta_accum != 0) {
      pos += delta_accum;
    }

    const int click_kind = enc.take_click_kind();

    const int32_t delta = pos - last_sent_pos;
    last_sent_pos = pos;

    const uint32_t ts_ms = (uint32_t)esp_log_timestamp();

    if (click_kind >= 0) {
      char click_msg[160];
      snprintf(click_msg,
               sizeof(click_msg),
               "{\"type\":\"encoder_click\",\"seq\":%" PRIu32 ",\"ts_ms\":%" PRIu32 ",\"kind\":%d}",
               seq++,
               ts_ms,
               click_kind);
      (void)http_server_ws_broadcast_text(click_msg);
    }

    char msg[192];
    snprintf(msg,
             sizeof(msg),
             "{\"type\":\"encoder\",\"seq\":%" PRIu32 ",\"ts_ms\":%" PRIu32 ",\"pos\":%" PRId32 ",\"delta\":%" PRId32 "}",
             seq++,
             ts_ms,
             pos,
             delta);

    (void)http_server_ws_broadcast_text(msg);

    vTaskDelayUntil(&last_wake, period);
  }
}
#endif

esp_err_t encoder_telemetry_start(void) {
  if (s_task) return ESP_OK;

#if !CONFIG_HTTPD_WS_SUPPORT
  return ESP_ERR_NOT_SUPPORTED;
#elif !CONFIG_TUTORIAL_0048_ENCODER_CHAIN_UART
  ESP_LOGI(TAG, "encoder telemetry disabled (CONFIG_TUTORIAL_0048_ENCODER_CHAIN_UART=n)");
  return ESP_OK;
#else
  const BaseType_t ok = xTaskCreate(&telemetry_task, "enc_ws", 4096, nullptr, 8, &s_task);
  if (ok != pdPASS) {
    s_task = nullptr;
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
#endif
}
