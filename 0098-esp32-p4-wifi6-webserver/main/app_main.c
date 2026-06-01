/*
 * ESP32-P4-WIFI6 simple webserver experiment.
 *
 * Networking model:
 *   - ESP32-P4 has no native Wi-Fi peripheral.
 *   - The Waveshare board carries an onboard ESP32-C6 radio.
 *   - ESP32-P4 talks to the C6 over ESP-Hosted SDIO.
 *   - The application still uses normal esp_wifi_* calls; esp_wifi_remote
 *     forwards those calls to the C6.
 *
 * Operator model:
 *   - Console is the CH343 USB-UART bridge on ESP32-P4 UART0 GPIO37/GPIO38.
 *   - Default STA credentials are compiled in for the first experiment.
 *   - esp_console exposes small Wi-Fi commands for status, reconnect, scan,
 *     and runtime credential changes.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_chip_info.h"
#include "esp_console.h"
#include "esp_event.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/inet.h"

static const char *TAG = "p4_web";

#define DEFAULT_WIFI_SSID      "yolobolo"
#define DEFAULT_WIFI_PASSWORD  "bring3248camera"
#define WIFI_MAX_RETRY         20
#define WIFI_NVS_NAMESPACE     "wifi"
#define WIFI_NVS_KEY_SSID      "ssid"
#define WIFI_NVS_KEY_PASSWORD  "password"

static httpd_handle_t s_httpd = NULL;
static esp_netif_t *s_sta_netif = NULL;
static int s_retry_count = 0;
static int s_last_disconnect_reason = -1;
static bool s_sta_connected = false;
static bool s_sta_got_ip = false;
static uint32_t s_sta_ip4_host = 0;
static char s_ssid[33] = DEFAULT_WIFI_SSID;
static char s_password[65] = DEFAULT_WIFI_PASSWORD;
static bool s_credentials_saved = false;

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS init returned %s; erasing and retrying", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static esp_err_t save_credentials_to_nvs(void)
{
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(nvs, WIFI_NVS_KEY_SSID, s_ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(nvs, WIFI_NVS_KEY_PASSWORD, s_password);
    }
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }
    nvs_close(nvs);

    if (err == ESP_OK) {
        s_credentials_saved = true;
        ESP_LOGI(TAG, "saved Wi-Fi credentials for ssid='%s'", s_ssid);
    }
    return err;
}

static esp_err_t clear_credentials_from_nvs(void)
{
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        return err;
    }

    (void)nvs_erase_key(nvs, WIFI_NVS_KEY_SSID);
    (void)nvs_erase_key(nvs, WIFI_NVS_KEY_PASSWORD);
    err = nvs_commit(nvs);
    nvs_close(nvs);

    if (err == ESP_OK) {
        s_credentials_saved = false;
        ESP_LOGI(TAG, "cleared saved Wi-Fi credentials; runtime ssid remains '%s'", s_ssid);
    }
    return err;
}

static esp_err_t load_credentials_from_nvs(void)
{
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "using built-in Wi-Fi defaults (no NVS namespace yet)");
        return err;
    }

    char ssid[sizeof(s_ssid)] = {0};
    size_t ssid_len = sizeof(ssid);
    err = nvs_get_str(nvs, WIFI_NVS_KEY_SSID, ssid, &ssid_len);
    if (err != ESP_OK || ssid[0] == '\0') {
        nvs_close(nvs);
        ESP_LOGI(TAG, "using built-in Wi-Fi defaults (no saved ssid)");
        return err == ESP_OK ? ESP_ERR_NOT_FOUND : err;
    }

    char password[sizeof(s_password)] = {0};
    size_t password_len = sizeof(password);
    esp_err_t pass_err = nvs_get_str(nvs, WIFI_NVS_KEY_PASSWORD, password, &password_len);
    nvs_close(nvs);

    if (pass_err != ESP_OK) {
        password[0] = '\0';
    }

    strlcpy(s_ssid, ssid, sizeof(s_ssid));
    strlcpy(s_password, password, sizeof(s_password));
    s_credentials_saved = true;
    ESP_LOGI(TAG, "loaded saved Wi-Fi credentials for ssid='%s'", s_ssid);
    return ESP_OK;
}

static const char *wifi_state_string(void)
{
    if (s_sta_got_ip) {
        return "got_ip";
    }
    if (s_sta_connected) {
        return "connected_no_ip";
    }
    if (s_retry_count > 0) {
        return "connecting";
    }
    return "idle";
}

static void log_sta_url(void)
{
    if (s_sta_ip4_host == 0) {
        return;
    }
    ip4_addr_t ip = {.addr = htonl(s_sta_ip4_host)};
    ESP_LOGI(TAG, "STA IP:  " IPSTR, IP2STR(&ip));
    ESP_LOGI(TAG, "Browse:  http://" IPSTR "/", IP2STR(&ip));
    ESP_LOGI(TAG, "Status:  http://" IPSTR "/status", IP2STR(&ip));
}

static void apply_sta_config(void)
{
    wifi_config_t cfg = {0};
    strlcpy((char *)cfg.sta.ssid, s_ssid, sizeof(cfg.sta.ssid));
    strlcpy((char *)cfg.sta.password, s_password, sizeof(cfg.sta.password));
    cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    cfg.sta.pmf_cfg.capable = true;
    cfg.sta.pmf_cfg.required = false;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "STA started; connecting to SSID '%s'", s_ssid);
        s_retry_count = 1;
        ESP_ERROR_CHECK(esp_wifi_connect());
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        const wifi_event_sta_connected_t *e = (const wifi_event_sta_connected_t *)event_data;
        s_sta_connected = true;
        s_last_disconnect_reason = -1;
        if (e) {
            ESP_LOGI(TAG, "STA connected: ssid=%.*s channel=%u authmode=%u",
                     e->ssid_len, (const char *)e->ssid, e->channel, e->authmode);
        }
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *e = (const wifi_event_sta_disconnected_t *)event_data;
        s_sta_connected = false;
        s_sta_got_ip = false;
        s_sta_ip4_host = 0;
        s_last_disconnect_reason = e ? (int)e->reason : -1;

        const bool should_retry = s_retry_count < WIFI_MAX_RETRY;
        ESP_LOGW(TAG, "STA disconnected reason=%d%s", s_last_disconnect_reason, should_retry ? "; retrying" : "; retry budget exhausted");
        if (should_retry) {
            s_retry_count += 1;
            (void)esp_wifi_connect();
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *e = (const ip_event_got_ip_t *)event_data;
        s_sta_got_ip = true;
        s_retry_count = 0;
        if (e) {
            s_sta_ip4_host = ntohl(e->ip_info.ip.addr);
        }
        log_sta_url();
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        s_sta_got_ip = false;
        s_sta_ip4_host = 0;
        ESP_LOGW(TAG, "STA lost IP");
        return;
    }
}

static esp_err_t start_wifi_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (!s_sta_netif) {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    apply_sta_config();
    ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}

static esp_err_t send_json(httpd_req_t *req, const char *json)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t root_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    const char *html =
        "<!doctype html>"
        "<html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>ESP32-P4 PicoCalc Web</title>"
        "<style>body{font-family:system-ui,sans-serif;max-width:760px;margin:2rem auto;padding:0 1rem;line-height:1.45}"
        "code,pre{background:#eee;padding:.15rem .3rem;border-radius:.25rem}button{font-size:1rem;padding:.4rem .7rem}</style>"
        "</head><body>"
        "<h1>ESP32-P4-WIFI6 webserver</h1>"
        "<p>This page is served by the ESP32-P4. Wi-Fi is provided by the onboard ESP32-C6 over ESP-Hosted SDIO.</p>"
        "<p>The app connects as a station to the configured Wi-Fi network, then serves HTTP on the assigned LAN IP.</p>"
        "<p><button onclick='loadStatus()'>Refresh status</button></p>"
        "<pre id='out'>Loading...</pre>"
        "<script>async function loadStatus(){const r=await fetch('/status');document.getElementById('out').textContent=JSON.stringify(await r.json(),null,2)}loadStatus()</script>"
        "</body></html>";

    return httpd_resp_sendstr(req, html);
}

static esp_err_t ping_get(httpd_req_t *req)
{
    return send_json(req, "{\"ok\":true,\"message\":\"pong\"}");
}

static esp_err_t status_get(httpd_req_t *req)
{
    esp_chip_info_t chip_info = {0};
    esp_chip_info(&chip_info);

    uint32_t flash_size = 0;
    (void)esp_flash_get_size(NULL, &flash_size);

    ip4_addr_t ip = {.addr = htonl(s_sta_ip4_host)};
    const int64_t uptime_ms = esp_timer_get_time() / 1000;
    const size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const size_t psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    const size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    char body[896];
    const int n = snprintf(body, sizeof(body),
                           "{"
                           "\"ok\":true,"
                           "\"project\":\"0098-esp32-p4-wifi6-webserver\","
                           "\"uptime_ms\":%" PRId64 ","
                           "\"chip\":{\"target\":\"%s\",\"revision\":%d,\"cores\":%d},"
                           "\"flash\":{\"bytes\":%" PRIu32 "},"
                           "\"heap\":{\"internal_free\":%u,\"psram_total\":%u,\"psram_free\":%u},"
                           "\"wifi\":{\"mode\":\"sta\",\"state\":\"%s\",\"ssid\":\"%s\",\"saved\":%s,\"ip\":\"" IPSTR "\",\"retries\":%d,\"last_disconnect_reason\":%d}"
                           "}",
                           uptime_ms,
                           CONFIG_IDF_TARGET,
                           chip_info.revision,
                           chip_info.cores,
                           flash_size,
                           (unsigned)internal_free,
                           (unsigned)psram_total,
                           (unsigned)psram_free,
                           wifi_state_string(),
                           s_ssid,
                           s_credentials_saved ? "true" : "false",
                           IP2STR(&ip),
                           s_retry_count,
                           s_last_disconnect_reason);

    if (n < 0 || n >= (int)sizeof(body)) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        return send_json(req, "{\"ok\":false,\"error\":\"status buffer overflow\"}");
    }

    return send_json(req, body);
}

static esp_err_t start_http_server(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = 8192;
    cfg.max_uri_handlers = 6;
    cfg.lru_purge_enable = true;

    ESP_LOGI(TAG, "starting HTTP server on port %d", cfg.server_port);
    esp_err_t err = httpd_start(&s_httpd, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        s_httpd = NULL;
        return err;
    }

    const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get};
    const httpd_uri_t status = {.uri = "/status", .method = HTTP_GET, .handler = status_get};
    const httpd_uri_t ping = {.uri = "/api/ping", .method = HTTP_GET, .handler = ping_get};

    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &status));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &ping));

    return ESP_OK;
}

static void print_wifi_status(void)
{
    ip4_addr_t ip = {.addr = htonl(s_sta_ip4_host)};
    printf("wifi state=%s ssid=%s saved=%s ip=" IPSTR " retries=%d last_reason=%d\n",
           wifi_state_string(), s_ssid, s_credentials_saved ? "yes" : "no", IP2STR(&ip), s_retry_count, s_last_disconnect_reason);
}

static int cmd_wifi(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "status") == 0) {
        print_wifi_status();
        return 0;
    }

    if (strcmp(argv[1], "reconnect") == 0 || strcmp(argv[1], "connect") == 0) {
        s_retry_count = 1;
        s_sta_connected = false;
        s_sta_got_ip = false;
        s_sta_ip4_host = 0;
        apply_sta_config();
        esp_err_t err = esp_wifi_disconnect();
        if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_CONNECT) {
            printf("disconnect before reconnect: %s\n", esp_err_to_name(err));
        }
        err = esp_wifi_connect();
        if (err != ESP_OK) {
            printf("wifi connect: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("connect requested for ssid=%s\n", s_ssid);
        return 0;
    }

    if (strcmp(argv[1], "set") == 0) {
        if (argc < 3) {
            printf("usage: wifi set <ssid> [password] [save]\n");
            return 1;
        }

        bool save = false;
        const char *password = NULL;
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "save") == 0 || strcmp(argv[i], "--save") == 0) {
                save = true;
                continue;
            }
            if (!password) {
                password = argv[i];
            }
        }

        strlcpy(s_ssid, argv[2], sizeof(s_ssid));
        if (password) {
            strlcpy(s_password, password, sizeof(s_password));
        } else {
            s_password[0] = '\0';
        }
        apply_sta_config();

        if (save) {
            esp_err_t err = save_credentials_to_nvs();
            if (err != ESP_OK) {
                printf("wifi set/save: %s\n", esp_err_to_name(err));
                return 1;
            }
        } else {
            printf("runtime credentials set: ssid=%s pass=%s (not saved)\n", s_ssid, s_password[0] ? "<non-empty>" : "<empty>");
        }
        return 0;
    }

    if (strcmp(argv[1], "save") == 0) {
        esp_err_t err = save_credentials_to_nvs();
        if (err != ESP_OK) {
            printf("wifi save: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("saved credentials for ssid=%s\n", s_ssid);
        return 0;
    }

    if (strcmp(argv[1], "clear") == 0) {
        esp_err_t err = clear_credentials_from_nvs();
        if (err != ESP_OK) {
            printf("wifi clear: %s\n", esp_err_to_name(err));
            return 1;
        }
        printf("cleared saved credentials; runtime ssid=%s remains active until changed or rebooted\n", s_ssid);
        return 0;
    }

    if (strcmp(argv[1], "scan") == 0) {
        wifi_scan_config_t scan_cfg = {0};
        esp_err_t err = esp_wifi_scan_start(&scan_cfg, true);
        if (err != ESP_OK) {
            printf("wifi scan_start: %s\n", esp_err_to_name(err));
            return 1;
        }
        uint16_t count = 0;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&count));
        wifi_ap_record_t *records = calloc(count ? count : 1, sizeof(*records));
        if (!records) {
            printf("wifi scan: no memory\n");
            return 1;
        }
        uint16_t show = count;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&show, records));
        printf("found %u networks\n", show);
        for (uint16_t i = 0; i < show; i++) {
            printf("%3u: ch=%u rssi=%d auth=%u ssid=%s\n", i, records[i].primary, records[i].rssi, records[i].authmode, records[i].ssid);
        }
        free(records);
        return 0;
    }

    printf("usage: wifi status | wifi reconnect | wifi set <ssid> [password] [save] | wifi save | wifi clear | wifi scan\n");
    return 1;
}

static void start_console(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "p4web> ";
    repl_cfg.task_stack_size = 4096;

    esp_console_register_help_command();

    const esp_console_cmd_t wifi_cmd = {
        .command = "wifi",
        .help = "Wi-Fi controls: status, reconnect, set <ssid> [password] [save], save, clear, scan",
        .func = cmd_wifi,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_cmd));

#if CONFIG_ESP_CONSOLE_UART_DEFAULT || CONFIG_ESP_CONSOLE_UART_CUSTOM
    esp_console_dev_uart_config_t hw_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_cfg, &repl_cfg, &repl));
#else
#error This app expects UART console on the CH343 bridge.
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    ESP_LOGI(TAG, "console ready: try 'help' or 'wifi status'");
}

void app_main(void)
{
    ESP_LOGI(TAG, "boot: ESP32-P4-WIFI6 webserver experiment");
    ESP_LOGI(TAG, "console: CH343 UART0 bridge at 115200 baud");
    ESP_LOGI(TAG, "wifi: ESP32-C6 over ESP-Hosted SDIO; default ssid=%s", DEFAULT_WIFI_SSID);

    ESP_ERROR_CHECK(init_nvs());
    (void)load_credentials_from_nvs();
    ESP_ERROR_CHECK(start_wifi_sta());
    ESP_ERROR_CHECK(start_http_server());
    start_console();

    while (true) {
        ESP_LOGI(TAG,
                 "heartbeat: state=%s internal_free=%u psram_free=%u",
                 wifi_state_string(),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
