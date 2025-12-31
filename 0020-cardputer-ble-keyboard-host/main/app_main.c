#include <inttypes.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"

#include "bt_console.h"
#include "ble_host.h"
#include "hid_host.h"
#include "keylog.h"

static const char *TAG = "ble_kbd_host";

void app_main(void) {
    ESP_LOGI(TAG, "boot: 0020 BLE keyboard host");

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    keylog_init();
    ble_host_init();
    hid_host_init();

    bt_console_start();
}
