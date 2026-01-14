#include "stream_client.h"

#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"

#include "driver/gpio.h"
#include "esp_camera.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "img_converters.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "wifi_sta.h"

#define STREAM_HOST_MAX 63
#define STREAM_URI_MAX 128
#define STREAM_NVS_NAMESPACE "stream"
#define STREAM_NVS_KEY_HOST "host"
#define STREAM_NVS_KEY_PORT "port"

#define CAMERA_POWER_GPIO 18

static const char *TAG = "cam_stream";

static char s_host[STREAM_HOST_MAX + 1] = {0};
static uint16_t s_port = CONFIG_ATOMS3R_STREAM_PORT;
static bool s_has_saved = false;
static bool s_has_runtime = false;

static volatile bool s_stream_enabled = false;
static volatile bool s_ws_connected = false;

static esp_websocket_client_handle_t s_ws = NULL;
static TaskHandle_t s_stream_task = NULL;
static bool s_camera_ready = false;

static esp_err_t ensure_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs init failed (%s), erasing...", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static bool build_ws_uri(char *out, size_t out_len) {
    if (!s_has_runtime || s_host[0] == '\0') return false;
    int n = snprintf(out, out_len, "ws://%s:%u/ws/camera", s_host, (unsigned)s_port);
    return (n > 0 && (size_t)n < out_len);
}

static esp_err_t stream_nvs_open(nvs_handle_t *out) {
    if (!out) return ESP_ERR_INVALID_ARG;
    return nvs_open(STREAM_NVS_NAMESPACE, NVS_READWRITE, out);
}

static esp_err_t load_saved_target(void) {
    nvs_handle_t nvs = 0;
    esp_err_t err = stream_nvs_open(&nvs);
    if (err != ESP_OK) return err;

    char host[STREAM_HOST_MAX + 1] = {0};
    size_t host_len = sizeof(host);
    err = nvs_get_str(nvs, STREAM_NVS_KEY_HOST, host, &host_len);
    if (err != ESP_OK || host[0] == '\0') {
        nvs_close(nvs);
        return ESP_ERR_NOT_FOUND;
    }

    uint16_t port = 0;
    esp_err_t port_err = nvs_get_u16(nvs, STREAM_NVS_KEY_PORT, &port);
    if (port_err != ESP_OK) {
        port = CONFIG_ATOMS3R_STREAM_PORT;
    }
    nvs_close(nvs);

    strlcpy(s_host, host, sizeof(s_host));
    s_port = port;
    s_has_runtime = true;
    s_has_saved = true;
    return ESP_OK;
}

static esp_err_t save_target_to_nvs(const char *host, uint16_t port) {
    nvs_handle_t nvs = 0;
    esp_err_t err = stream_nvs_open(&nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_str(nvs, STREAM_NVS_KEY_HOST, host ? host : "");
    if (err == ESP_OK) err = nvs_set_u16(nvs, STREAM_NVS_KEY_PORT, port);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

static esp_err_t clear_target_in_nvs(void) {
    nvs_handle_t nvs = 0;
    esp_err_t err = stream_nvs_open(&nvs);
    if (err != ESP_OK) return err;
    (void)nvs_erase_key(nvs, STREAM_NVS_KEY_HOST);
    (void)nvs_erase_key(nvs, STREAM_NVS_KEY_PORT);
    err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

static void ws_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    (void)handler_args;
    (void)base;
    (void)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            s_ws_connected = true;
            ESP_LOGI(TAG, "WebSocket connected");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
        case WEBSOCKET_EVENT_CLOSED:
            s_ws_connected = false;
            ESP_LOGW(TAG, "WebSocket disconnected");
            break;
        case WEBSOCKET_EVENT_ERROR:
            s_ws_connected = false;
            ESP_LOGE(TAG, "WebSocket error");
            break;
        default:
            break;
    }
}

static void stop_ws_client(void) {
    if (!s_ws) return;
    esp_websocket_client_stop(s_ws);
    esp_websocket_client_destroy(s_ws);
    s_ws = NULL;
    s_ws_connected = false;
}

static void ensure_ws_client(void) {
    if (!s_ws && s_has_runtime) {
        char uri[STREAM_URI_MAX] = {0};
        if (!build_ws_uri(uri, sizeof(uri))) {
            ESP_LOGE(TAG, "invalid stream URI (host=%s port=%u)", s_host, (unsigned)s_port);
            return;
        }
        esp_websocket_client_config_t cfg = {
            .uri = uri,
            .reconnect_timeout_ms = 5000,
            .buffer_size = 8 * 1024,
        };
        s_ws = esp_websocket_client_init(&cfg);
        if (!s_ws) {
            ESP_LOGE(TAG, "failed to init websocket client");
            return;
        }
        ESP_ERROR_CHECK(esp_websocket_register_events(s_ws, WEBSOCKET_EVENT_ANY, ws_event_handler, NULL));
    }

    if (s_ws && !esp_websocket_client_is_connected(s_ws)) {
        esp_err_t err = esp_websocket_client_start(s_ws);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "websocket start failed: %s", esp_err_to_name(err));
        }
    }
}

static void camera_power_on(void) {
    gpio_reset_pin(CAMERA_POWER_GPIO);
    gpio_set_direction(CAMERA_POWER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(CAMERA_POWER_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
}

static esp_err_t camera_init_once(void) {
    if (s_camera_ready) return ESP_OK;

    camera_power_on();

    camera_config_t cfg = {
        .pin_pwdn = -1,
        .pin_reset = -1,
        .pin_xclk = 21,
        .pin_sscb_sda = 12,
        .pin_sscb_scl = 9,

        .pin_d7 = 13,
        .pin_d6 = 11,
        .pin_d5 = 17,
        .pin_d4 = 4,
        .pin_d3 = 48,
        .pin_d2 = 46,
        .pin_d1 = 42,
        .pin_d0 = 3,
        .pin_vsync = 10,
        .pin_href = 14,
        .pin_pclk = 40,

        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_RGB565,
        .frame_size = FRAMESIZE_QVGA,

        .jpeg_quality = CONFIG_ATOMS3R_JPEG_QUALITY,
        .fb_count = 2,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
        .fb_location = CAMERA_FB_IN_PSRAM,
    };

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "camera init failed: %s", esp_err_to_name(err));
        return err;
    }

    s_camera_ready = true;
    ESP_LOGI(TAG, "camera ready");
    return ESP_OK;
}

static void stream_task(void *arg) {
    (void)arg;
    const TickType_t delay_ticks = pdMS_TO_TICKS(CONFIG_ATOMS3R_STREAM_INTERVAL_MS);

    while (true) {
        if (!s_stream_enabled) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (!s_has_runtime) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (!cam_wifi_is_connected()) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (!s_camera_ready) {
            if (camera_init_once() != ESP_OK) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
        }

        ensure_ws_client();
        if (!s_ws || !esp_websocket_client_is_connected(s_ws)) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGW(TAG, "camera capture failed");
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        uint8_t *jpeg_buf = fb->buf;
        size_t jpeg_len = fb->len;
        bool needs_free = false;

        if (fb->format != PIXFORMAT_JPEG) {
            if (!frame2jpg(fb, CONFIG_ATOMS3R_JPEG_QUALITY, &jpeg_buf, &jpeg_len)) {
                ESP_LOGW(TAG, "frame2jpg failed");
                esp_camera_fb_return(fb);
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            needs_free = true;
        }

        int sent = esp_websocket_client_send_bin(s_ws, (const char *)jpeg_buf, (int)jpeg_len, 0);
        if (sent < 0) {
            ESP_LOGW(TAG, "websocket send failed (%d)", sent);
        }

        if (needs_free) {
            free(jpeg_buf);
        }
        esp_camera_fb_return(fb);

        vTaskDelay(delay_ticks);
    }
}

esp_err_t stream_client_init(void) {
    ESP_ERROR_CHECK(ensure_nvs());

    esp_err_t saved_err = load_saved_target();
    if (saved_err == ESP_OK) {
        ESP_LOGI(TAG, "loaded stream target from NVS (%s:%u)", s_host, (unsigned)s_port);
    } else if (saved_err != ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "failed to load stream target from NVS: %s", esp_err_to_name(saved_err));
    }

    if (!s_has_runtime && CONFIG_ATOMS3R_STREAM_HOST[0] != '\0') {
        strlcpy(s_host, CONFIG_ATOMS3R_STREAM_HOST, sizeof(s_host));
        s_port = CONFIG_ATOMS3R_STREAM_PORT;
        s_has_runtime = true;
        ESP_LOGI(TAG, "using stream target from Kconfig (%s:%u)", s_host, (unsigned)s_port);
    }

    if (!s_stream_task) {
        xTaskCreate(stream_task, "cam_stream", 6144, NULL, 5, &s_stream_task);
    }

    return ESP_OK;
}

esp_err_t stream_client_set_target(const char *host, uint16_t port, bool save_to_nvs) {
    if (!host || host[0] == '\0') return ESP_ERR_INVALID_ARG;
    if (strlen(host) > STREAM_HOST_MAX) return ESP_ERR_INVALID_SIZE;

    strlcpy(s_host, host, sizeof(s_host));
    s_port = port;
    s_has_runtime = true;

    if (save_to_nvs) {
        esp_err_t err = save_target_to_nvs(host, port);
        if (err != ESP_OK) return err;
        s_has_saved = true;
    }

    stop_ws_client();
    return ESP_OK;
}

esp_err_t stream_client_clear_target(void) {
    esp_err_t err = clear_target_in_nvs();
    s_host[0] = '\0';
    s_has_runtime = false;
    s_has_saved = false;
    stop_ws_client();
    return err;
}

void stream_client_get_target(stream_target_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    strlcpy(out->host, s_host, sizeof(out->host));
    out->port = s_port;
    out->has_saved = s_has_saved;
    out->has_runtime = s_has_runtime;
}

void stream_client_start(void) {
    s_stream_enabled = true;
}

void stream_client_stop(void) {
    s_stream_enabled = false;
    stop_ws_client();
}

bool stream_client_is_running(void) {
    return s_stream_enabled;
}

bool stream_client_is_connected(void) {
    return s_ws_connected;
}
