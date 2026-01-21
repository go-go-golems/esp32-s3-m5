#include "http_server.h"
#include "wifi_app.h"

#include "esp_log.h"

static const char* TAG = "cardputer_js_web_0048";

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "boot");
  ESP_ERROR_CHECK(wifi_app_start());
  ESP_ERROR_CHECK(http_server_start());
  ESP_LOGI(TAG, "ready");
}

