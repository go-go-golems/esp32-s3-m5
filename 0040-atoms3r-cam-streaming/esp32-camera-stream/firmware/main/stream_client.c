#include "stream_client.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_camera.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#if CONFIG_SPIRAM
#include "esp_psram.h"
#endif
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
#define CAMERA_XCLK_GPIO 21
#define CAMERA_SIOD_GPIO 12
#define CAMERA_SIOC_GPIO 9

static const uint32_t CAMERA_WARMUP_DELAY_MS = 1000;

static const char *TAG = "cam_stream";

static char s_host[STREAM_HOST_MAX + 1] = {0};
static uint16_t s_port = CONFIG_ATOMS3R_STREAM_PORT;
static bool s_has_saved = false;
static bool s_has_runtime = false;

static volatile bool s_stream_enabled = false;
static volatile bool s_ws_connected = false;
static volatile bool s_ws_start_pending = false;

static esp_websocket_client_handle_t s_ws = NULL;
static TaskHandle_t s_stream_task = NULL;
static bool s_camera_ready = false;
static int s_camera_power_level = 0;
static bool s_logged_frame_info = false;

typedef struct {
    uint64_t frames;
    uint64_t capture_fail;
    uint64_t convert_ok;
    uint64_t convert_fail;
    uint64_t ws_send_ok;
    uint64_t ws_send_fail;
    uint64_t ws_send_bytes;
    uint64_t capture_us;
    uint64_t convert_us;
    uint64_t send_us;
    int64_t last_log_us;
} stream_stats_t;

static stream_stats_t s_stats = {0};
static int64_t s_last_state_log_us = 0;

#define STREAM_STATS_INTERVAL_US (5 * 1000 * 1000)

static int camera_power_gpio_level(int requested_on) {
#if CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW
    return requested_on ? 0 : 1;
#else
    return requested_on ? 1 : 0;
#endif
}

static void camera_power_configure(int level) {
    int requested = level ? 1 : 0;
    int gpio_level = camera_power_gpio_level(requested);
    gpio_reset_pin(CAMERA_POWER_GPIO);
    gpio_set_direction(CAMERA_POWER_GPIO, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_pull_mode(CAMERA_POWER_GPIO, gpio_level ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY);
    gpio_set_level(CAMERA_POWER_GPIO, gpio_level);
    vTaskDelay(pdMS_TO_TICKS(200));
}

static void log_stream_state(const char *reason) {
    int64_t now = esp_timer_get_time();
    if (s_last_state_log_us == 0 || (now - s_last_state_log_us) > STREAM_STATS_INTERVAL_US) {
        ESP_LOGI(TAG, "stream idle: %s", reason);
        s_last_state_log_us = now;
    }
}

static void stats_log_maybe(void) {
    int64_t now = esp_timer_get_time();
    if (s_stats.last_log_us == 0) {
        s_stats.last_log_us = now;
        return;
    }
    int64_t elapsed = now - s_stats.last_log_us;
    if (elapsed < STREAM_STATS_INTERVAL_US) return;

    double seconds = (double)elapsed / 1000000.0;
    uint64_t frames = s_stats.frames;
    double fps = frames ? ((double)frames / seconds) : 0.0;
    double avg_bytes = frames ? ((double)s_stats.ws_send_bytes / (double)frames) : 0.0;
    double cap_ms = frames ? ((double)s_stats.capture_us / (double)frames / 1000.0) : 0.0;
    double conv_ms = s_stats.convert_ok ? ((double)s_stats.convert_us / (double)s_stats.convert_ok / 1000.0) : 0.0;
    double send_ms = s_stats.ws_send_ok ? ((double)s_stats.send_us / (double)s_stats.ws_send_ok / 1000.0) : 0.0;

    ESP_LOGI(TAG,
             "stream stats: frames=%" PRIu64 " fps=%.1f bytes=%" PRIu64 " avg=%.0f cap_ms=%.2f conv_ms=%.2f send_ms=%.2f cap_fail=%" PRIu64 " conv_fail=%" PRIu64 " ws_fail=%" PRIu64,
             frames, fps, s_stats.ws_send_bytes, avg_bytes, cap_ms, conv_ms, send_ms,
             s_stats.capture_fail, s_stats.convert_fail, s_stats.ws_send_fail);

    memset(&s_stats, 0, sizeof(s_stats));
    s_stats.last_log_us = now;
}

static bool psram_ready(void) {
#if CONFIG_SPIRAM
    bool ready = esp_psram_is_initialized();
    if (ready) {
        size_t size = esp_psram_get_size();
        ESP_LOGI(TAG, "PSRAM detected (%u bytes)", (unsigned)size);
    } else {
        ESP_LOGW(TAG, "PSRAM not initialized");
    }
    return ready;
#else
    return false;
#endif
}

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

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            s_ws_connected = true;
            s_ws_start_pending = false;
            ESP_LOGI(TAG, "WebSocket connected");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
        case WEBSOCKET_EVENT_CLOSED:
            s_ws_connected = false;
            s_ws_start_pending = false;
            ESP_LOGW(TAG, "WebSocket disconnected");
            break;
        case WEBSOCKET_EVENT_ERROR:
            s_ws_connected = false;
            s_ws_start_pending = false;
            ESP_LOGE(TAG, "WebSocket error");
            if (event_data) {
                const esp_websocket_event_data_t *data = (const esp_websocket_event_data_t *)event_data;
                ESP_LOGE(TAG,
                         "WebSocket error details: type=%d ws_status=%d tls_stack=%d tls_verify=0x%x sock_errno=%d",
                         data->error_handle.error_type,
                         data->error_handle.esp_ws_handshake_status_code,
                         data->error_handle.esp_tls_stack_err,
                         data->error_handle.esp_tls_cert_verify_flags,
                         data->error_handle.esp_transport_sock_errno);
            }
            break;
        case WEBSOCKET_EVENT_DATA: {
            const esp_websocket_event_data_t *data = (const esp_websocket_event_data_t *)event_data;
            if (data) {
                ESP_LOGI(TAG, "WebSocket data op=%d len=%d", data->op_code, data->data_len);
            }
            break;
        }
        default:
            ESP_LOGD(TAG, "WebSocket event %d", (int)event_id);
            break;
    }
}

static void stop_ws_client(void) {
    if (!s_ws) return;
    ESP_LOGI(TAG, "WebSocket stop");
    esp_websocket_client_stop(s_ws);
    esp_websocket_client_destroy(s_ws);
    s_ws = NULL;
    s_ws_connected = false;
    s_ws_start_pending = false;
}

static void ensure_ws_client(void) {
    if (!s_ws && s_has_runtime) {
        char uri[STREAM_URI_MAX] = {0};
        if (!build_ws_uri(uri, sizeof(uri))) {
            ESP_LOGE(TAG, "invalid stream URI (host=%s port=%u)", s_host, (unsigned)s_port);
            return;
        }
        ESP_LOGI(TAG, "WebSocket target: %s", uri);
        esp_websocket_client_config_t cfg = {
            .uri = uri,
            .reconnect_timeout_ms = 5000,
            .network_timeout_ms = 15000,
            .buffer_size = 16 * 1024,
        };
        s_ws = esp_websocket_client_init(&cfg);
        if (!s_ws) {
            ESP_LOGE(TAG, "failed to init websocket client");
            return;
        }
        ESP_ERROR_CHECK(esp_websocket_register_events(s_ws, WEBSOCKET_EVENT_ANY, ws_event_handler, NULL));
    }

    if (s_ws && !esp_websocket_client_is_connected(s_ws)) {
        if (s_ws_start_pending) {
            return;
        }
        esp_err_t err = esp_websocket_client_start(s_ws);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "websocket start failed: %s", esp_err_to_name(err));
            s_ws_start_pending = false;
        } else {
            ESP_LOGI(TAG, "websocket start requested");
            s_ws_start_pending = true;
        }
    }
}

static void camera_power_on(void) {
    camera_power_configure(s_camera_power_level);
}

static const char *fb_location_name(camera_fb_location_t loc) {
    switch (loc) {
        case CAMERA_FB_IN_PSRAM:
            return "psram";
        case CAMERA_FB_IN_DRAM:
            return "dram";
        default:
            return "unknown";
    }
}

static const char *camera_task_core_name(void) {
#if CONFIG_CAMERA_CORE0
    return "core0";
#elif CONFIG_CAMERA_CORE1
    return "core1";
#elif CONFIG_CAMERA_NO_AFFINITY
    return "no_affinity";
#else
    return "unknown";
#endif
}

static void log_camera_component_options(void) {
    int sccb_port = 0;
#if CONFIG_SCCB_HARDWARE_I2C_PORT1
    sccb_port = 1;
#endif
#if CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE_CUSTOM
    ESP_LOGI(TAG,
             "camera options: sccb_port=%d sccb_freq=%u cam_task_stack=%d cam_task_core=%s dma_buf=%d jpeg_frame_size=custom(%d)",
             sccb_port,
             (unsigned)CONFIG_SCCB_CLK_FREQ,
             CONFIG_CAMERA_TASK_STACK_SIZE,
             camera_task_core_name(),
             CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX,
             CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE);
#else
    ESP_LOGI(TAG,
             "camera options: sccb_port=%d sccb_freq=%u cam_task_stack=%d cam_task_core=%s dma_buf=%d jpeg_frame_size=auto",
             sccb_port,
             (unsigned)CONFIG_SCCB_CLK_FREQ,
             CONFIG_CAMERA_TASK_STACK_SIZE,
             camera_task_core_name(),
             CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX);
#endif
}

static void log_camera_config(const camera_config_t *cfg, bool use_psram) {
    if (!cfg) return;
    ESP_LOGI(TAG,
             "camera cfg: psram=%s xclk=%u fmt=%d frame=%d fb_count=%d fb_loc=%s",
             use_psram ? "yes" : "no",
             (unsigned)cfg->xclk_freq_hz,
             cfg->pixel_format,
             cfg->frame_size,
             cfg->fb_count,
             fb_location_name(cfg->fb_location));
    ESP_LOGI(TAG,
             "camera pins: xclk=%d sda=%d scl=%d vsync=%d href=%d pclk=%d d0=%d d7=%d",
             cfg->pin_xclk,
             cfg->pin_sccb_sda,
             cfg->pin_sccb_scl,
             cfg->pin_vsync,
             cfg->pin_href,
            cfg->pin_pclk,
            cfg->pin_d0,
            cfg->pin_d7);
}

static void camera_sccb_probe_known_addrs(void) {
#if CONFIG_SCCB_HARDWARE_I2C_PORT1
    const i2c_port_t port = I2C_NUM_1;
#else
    const i2c_port_t port = I2C_NUM_0;
#endif
    static const uint8_t known_addrs[] = {
        0x21, // OV7725/OV7670/GC032A/GC0308
        0x30, // OV2640/SC031GS
        0x3C, // OV5640/OV3660/GC2145
        0x2A, // NT99141
        0x6E, // BF3005/BF20A6
        0x68, // SC101IOT/SC030IOT
    };

    ESP_LOGI(TAG, "sccb probe: port=%d sda=%d scl=%d freq=%u",
             (int)port,
             CAMERA_SIOD_GPIO,
             CAMERA_SIOC_GPIO,
             (unsigned)CONFIG_SCCB_CLK_FREQ);
    ESP_LOGI(TAG, "sccb probe: using existing i2c driver");

    bool found = false;
    for (size_t i = 0; i < sizeof(known_addrs) / sizeof(known_addrs[0]); ++i) {
        const uint8_t addr = known_addrs[i];
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "sccb probe: ack at 0x%02x", addr);
            found = true;
            break;
        }
    }

    if (!found) {
        ESP_LOGW(TAG, "sccb probe: no known devices found");
    }
}

static esp_err_t camera_init_once(void) {
    if (s_camera_ready) return ESP_OK;

    ESP_LOGI(TAG, "camera init: requested=%d gpio=%d active_low=%s gpio_level=%d",
             s_camera_power_level,
             CAMERA_POWER_GPIO,
#if CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW
             "yes",
#else
             "no",
#endif
             camera_power_gpio_level(s_camera_power_level));
    camera_power_on();
    ESP_LOGI(TAG, "camera power actual level=%d", gpio_get_level(CAMERA_POWER_GPIO));
    ESP_LOGI(TAG, "camera warmup delay=%u ms", (unsigned)CAMERA_WARMUP_DELAY_MS);
    vTaskDelay(pdMS_TO_TICKS(CAMERA_WARMUP_DELAY_MS));

    const bool use_psram = psram_ready();
    if (!use_psram) {
        ESP_LOGE(TAG, "PSRAM unavailable; refusing to init camera without PSRAM");
        return ESP_ERR_INVALID_STATE;
    }

    log_camera_component_options();

    camera_config_t cfg = {
        .pin_pwdn = -1,
        .pin_reset = -1,
        .pin_xclk = CAMERA_XCLK_GPIO,
        .pin_sccb_sda = CAMERA_SIOD_GPIO,
        .pin_sccb_scl = CAMERA_SIOC_GPIO,

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

        .xclk_freq_hz = CONFIG_CAMERA_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_RGB565,
        .frame_size = FRAMESIZE_QVGA,

        .jpeg_quality = CONFIG_ATOMS3R_SENSOR_JPEG_QUALITY,
        .fb_count = 2,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
        .fb_location = CAMERA_FB_IN_PSRAM,
    };

    log_camera_config(&cfg, use_psram);

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "camera init failed: %s", esp_err_to_name(err));
        return err;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        ESP_LOGI(TAG, "sensor id: PID=0x%04x MIDH=0x%02x MIDL=0x%02x",
                 s->id.PID, s->id.MIDH, s->id.MIDL);
        s->set_vflip(s, 1);
        if (s->id.PID == OV3660_PID) {
            s->set_brightness(s, 1);
            s->set_saturation(s, -2);
        }

        if (s->id.PID == OV3660_PID || s->id.PID == OV2640_PID) {
            s->set_vflip(s, 1);
        } else if (s->id.PID == GC0308_PID) {
            s->set_hmirror(s, 0);
        } else if (s->id.PID == GC032A_PID) {
            s->set_vflip(s, 1);
        }
    }

    camera_sccb_probe_known_addrs();

    s_camera_ready = true;
    ESP_LOGI(TAG, "camera ready");
    return ESP_OK;
}

static void stream_task(void *arg) {
    (void)arg;
    const TickType_t delay_ticks = pdMS_TO_TICKS(CONFIG_ATOMS3R_STREAM_INTERVAL_MS);

    while (true) {
        if (!s_stream_enabled) {
            log_stream_state("disabled");
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (!s_has_runtime) {
            log_stream_state("target not set");
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (!cam_wifi_is_connected()) {
            log_stream_state("wifi not connected");
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (!s_camera_ready) {
            if (camera_init_once() != ESP_OK) {
                log_stream_state("camera init failed");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
        }

        ensure_ws_client();
        if (!s_ws || !esp_websocket_client_is_connected(s_ws)) {
            log_stream_state("websocket not connected");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        int64_t capture_start = esp_timer_get_time();
        camera_fb_t *fb = esp_camera_fb_get();
        int64_t capture_end = esp_timer_get_time();
        if (!fb) {
            s_stats.capture_fail++;
            s_stats.capture_us += (uint64_t)(capture_end - capture_start);
            ESP_LOGW(TAG, "camera capture failed");
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        s_stats.frames++;
        s_stats.capture_us += (uint64_t)(capture_end - capture_start);

        if (!s_logged_frame_info) {
            ESP_LOGI(TAG, "frame: %ux%u fmt=%d len=%u",
                     (unsigned)fb->width,
                     (unsigned)fb->height,
                     fb->format,
                     (unsigned)fb->len);
            s_logged_frame_info = true;
        }

        uint8_t *jpeg_buf = fb->buf;
        size_t jpeg_len = fb->len;
        bool needs_free = false;

        if (fb->format != PIXFORMAT_JPEG) {
            int64_t convert_start = esp_timer_get_time();
            bool ok = frame2jpg(fb, CONFIG_ATOMS3R_CONVERT_JPEG_QUALITY, &jpeg_buf, &jpeg_len);
            int64_t convert_end = esp_timer_get_time();
            if (!ok) {
                s_stats.convert_fail++;
                ESP_LOGW(TAG, "frame2jpg failed");
                esp_camera_fb_return(fb);
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            s_stats.convert_ok++;
            s_stats.convert_us += (uint64_t)(convert_end - convert_start);
            needs_free = true;
        }

        int64_t send_start = esp_timer_get_time();
        TickType_t ws_timeout = pdMS_TO_TICKS(1000);
        int sent = esp_websocket_client_send_bin(s_ws, (const char *)jpeg_buf, (int)jpeg_len, ws_timeout);
        int64_t send_end = esp_timer_get_time();
        if (sent < 0) {
            s_stats.ws_send_fail++;
            ESP_LOGW(TAG, "websocket send failed (%d)", sent);
        } else {
            s_stats.ws_send_ok++;
            s_stats.ws_send_bytes += (uint64_t)sent;
            s_stats.send_us += (uint64_t)(send_end - send_start);
        }

        if (needs_free) {
            free(jpeg_buf);
        }
        esp_camera_fb_return(fb);

        stats_log_maybe();
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
    ESP_LOGI(TAG, "stream target set: %s:%u (save=%s)", s_host, (unsigned)s_port, save_to_nvs ? "yes" : "no");

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
    ESP_LOGI(TAG, "stream target cleared");
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
    ESP_LOGI(TAG, "stream start");
    s_stream_enabled = true;
}

void stream_client_stop(void) {
    ESP_LOGI(TAG, "stream stop");
    s_stream_enabled = false;
    stop_ws_client();
}

bool stream_client_is_running(void) {
    return s_stream_enabled;
}

bool stream_client_is_connected(void) {
    return s_ws_connected;
}

void stream_camera_power_set(int level) {
    s_camera_power_level = level ? 1 : 0;
    camera_power_configure(s_camera_power_level);
    ESP_LOGI(TAG, "camera power set: requested=%d active_low=%s gpio_level=%d actual=%d",
             s_camera_power_level,
#if CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW
             "yes",
#else
             "no",
#endif
             camera_power_gpio_level(s_camera_power_level),
             gpio_get_level(CAMERA_POWER_GPIO));
}

void stream_camera_power_dump(void) {
    gpio_dump_io_configuration(stdout, 1ULL << CAMERA_POWER_GPIO);
    ESP_LOGI(TAG, "camera power gpio=%d level=%d requested=%d active_low=%s gpio_level=%d",
             CAMERA_POWER_GPIO,
             gpio_get_level(CAMERA_POWER_GPIO),
             s_camera_power_level,
#if CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW
             "yes",
#else
             "no",
#endif
             camera_power_gpio_level(s_camera_power_level));
}
