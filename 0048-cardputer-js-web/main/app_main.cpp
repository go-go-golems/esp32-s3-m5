#include "http_server.h"
#include "js_service.h"
#include "wifi_console.h"
#include "wifi_mgr.h"

#include "esp_log.h"

static const char* TAG = "cardputer_js_web_0048";

static void on_wifi_got_ip(uint32_t ip4_host_order, void* ctx) {
  (void)ip4_host_order;
  (void)ctx;
  (void)http_server_start();
}

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "boot");
  ESP_ERROR_CHECK(js_service_start());
  wifi_mgr_set_on_got_ip_cb(on_wifi_got_ip, nullptr);
  ESP_ERROR_CHECK(wifi_mgr_start());
  wifi_console_start();
  ESP_LOGI(TAG, "ready");
}
