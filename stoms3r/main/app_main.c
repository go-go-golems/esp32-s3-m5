/*
 * SToMS3R — AtomS3R Lite Thermal Printer Console Firmware
 *
 * Hardware target:
 *   Controller : M5Stack AtomS3R Lite (ESP32-S3-PICO-1-N8R8)
 *   Printer    : M5Stack K118 Thermal Printer Kit (58 mm, 203 dpi)
 *   Connection : UART1 9600 8N1, TX=GPIO5, RX=GPIO6
 *   Console    : USB Serial/JTAG (/dev/ttyACM0)
 *   Power      : USB-C for logic, 12 V / 2.5 A for printer mechanism
 */

#include <stdio.h>
#include <string.h>

#include "esp_console.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "nvs_store.h"
#include "printer_cmd.h"
#include "printer_drv.h"
#include "wifi_cmd.h"
#include "wifi_mgr.h"

static const char *TAG = "stoms3r";

void app_main(void)
{
    ESP_LOGI(TAG, "============================");
    ESP_LOGI(TAG, "SToMS3R starting...");
    ESP_LOGI(TAG, "AtomS3R Lite + K118 printer");
    ESP_LOGI(TAG, "============================");

    /* 1. NVS */
    ESP_ERROR_CHECK(nvs_store_init());

    /* 2. Network stack */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 3. WiFi manager (registers event handlers) */
    ESP_ERROR_CHECK(wifi_mgr_init());

    /* 4. Printer UART */
    ESP_ERROR_CHECK(printer_drv_init());

    /* 5. Auto-connect if credentials are saved */
    char ssid[64] = {0};
    char password[64] = {0};
    if (nvs_store_load_wifi(ssid, sizeof(ssid), password, sizeof(password)) == ESP_OK) {
        ESP_LOGI(TAG, "Saved WiFi found: \"%s\" — connecting...", ssid);
        wifi_mgr_connect(ssid, password);
    } else {
        ESP_LOGI(TAG, "No saved WiFi credentials");
    }

    /* 6. Start the interactive console */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "stoms3r> ";
    repl_cfg.max_cmdline_length = 256;
    repl_cfg.task_stack_size = 6144;

    esp_console_dev_usb_serial_jtag_config_t hw_cfg =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_console_new_repl_usb_serial_jtag(&hw_cfg, &repl_cfg, &repl));

    /* 7. Register commands */
    esp_console_register_help_command();
    printer_cmd_register();
    wifi_cmd_register();

    /* 8. Start REPL (does not return) */
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
