#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_camera.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sensor.h"
#if CONFIG_SPIRAM
#include "esp_psram.h"
#endif
#include "camera_pin.h"

static const char *TAG = "cam_test";
static const uint32_t CAMERA_POWER_DELAY_MS = 200;
static const uint32_t CAMERA_WARMUP_DELAY_MS = 1000;

static void log_step(const char *step)
{
    ESP_LOGI(TAG, "STEP: %s", step);
}

static const char *camera_task_core_name(void)
{
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

static const char *pixformat_name(pixformat_t fmt)
{
    switch (fmt) {
    case PIXFORMAT_RGB565:
        return "RGB565";
    case PIXFORMAT_YUV422:
        return "YUV422";
    case PIXFORMAT_YUV420:
        return "YUV420";
    case PIXFORMAT_GRAYSCALE:
        return "GRAYSCALE";
    case PIXFORMAT_JPEG:
        return "JPEG";
    case PIXFORMAT_RGB888:
        return "RGB888";
    case PIXFORMAT_RAW:
        return "RAW";
    case PIXFORMAT_RGB444:
        return "RGB444";
    case PIXFORMAT_RGB555:
        return "RGB555";
    default:
        return "unknown";
    }
}

static const char *framesize_name(framesize_t size)
{
    switch (size) {
    case FRAMESIZE_QQVGA:
        return "QQVGA";
    case FRAMESIZE_QVGA:
        return "QVGA";
    case FRAMESIZE_VGA:
        return "VGA";
    case FRAMESIZE_SVGA:
        return "SVGA";
    case FRAMESIZE_XGA:
        return "XGA";
    case FRAMESIZE_SXGA:
        return "SXGA";
    case FRAMESIZE_UXGA:
        return "UXGA";
    case FRAMESIZE_96X96:
        return "96x96";
    case FRAMESIZE_240X240:
        return "240x240";
    case FRAMESIZE_CIF:
        return "CIF";
    case FRAMESIZE_HVGA:
        return "HVGA";
    case FRAMESIZE_HD:
        return "HD";
    case FRAMESIZE_FHD:
        return "FHD";
    default:
        return "unknown";
    }
}

static const char *fb_location_name(camera_fb_location_t location)
{
    switch (location) {
    case CAMERA_FB_IN_PSRAM:
        return "psram";
    case CAMERA_FB_IN_DRAM:
        return "dram";
    default:
        return "unknown";
    }
}

static int sccb_port_from_config(void)
{
#if CONFIG_SCCB_HARDWARE_I2C_PORT1
    return 1;
#else
    return 0;
#endif
}

static void camera_sccb_probe_known_addrs(void)
{
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
             CAMERA_PIN_SIOD,
             CAMERA_PIN_SIOC,
             (unsigned)CONFIG_SCCB_CLK_FREQ);

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)CAMERA_PIN_SIOD;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)CAMERA_PIN_SIOC;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = CONFIG_SCCB_CLK_FREQ;

    esp_err_t err = i2c_param_config(port, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sccb probe: i2c_param_config failed: %s", esp_err_to_name(err));
        return;
    }

    bool installed = false;
    err = i2c_driver_install(port, conf.mode, 0, 0, 0);
    if (err == ESP_OK) {
        installed = true;
    } else if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "sccb probe: i2c driver already installed on port %d", (int)port);
    } else {
        ESP_LOGE(TAG, "sccb probe: i2c_driver_install failed: %s", esp_err_to_name(err));
        return;
    }

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

    if (installed) {
        ESP_LOGI(TAG, "sccb probe: leaving i2c driver installed");
    }
}

static bool psram_ready(void)
{
#if CONFIG_SPIRAM
    if (esp_psram_is_initialized()) {
        ESP_LOGI(TAG, "PSRAM detected (%u bytes)", (unsigned)esp_psram_get_size());
        return true;
    }
    ESP_LOGW(TAG, "PSRAM not initialized");
    return false;
#else
    ESP_LOGW(TAG, "PSRAM disabled (CONFIG_SPIRAM not set)");
    return false;
#endif
}

static int camera_power_gpio_level(bool enable)
{
    int level = enable ? 1 : 0;
#if CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW
    level = !level;
#endif
    return level;
}

static void camera_power_set(bool enable)
{
#if CONFIG_ATOMS3R_CAMERA_POWER_GPIO >= 0
    const int gpio_num = CONFIG_ATOMS3R_CAMERA_POWER_GPIO;
    gpio_reset_pin(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(gpio_num, GPIO_PULLDOWN_ONLY);
    gpio_set_level(gpio_num, camera_power_gpio_level(enable));
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "camera power: gpio=%d active_low=%s level=%d",
             gpio_num,
#if CONFIG_ATOMS3R_CAMERA_POWER_ACTIVE_LOW
             "yes",
#else
             "no",
#endif
             gpio_get_level(gpio_num));
#else
    ESP_LOGI(TAG, "camera power: disabled (GPIO=-1)");
#endif
}

static void camera_power_force_level(int level)
{
#if CONFIG_ATOMS3R_CAMERA_POWER_GPIO >= 0
    const int gpio_num = CONFIG_ATOMS3R_CAMERA_POWER_GPIO;
    gpio_reset_pin(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(gpio_num, GPIO_PULLDOWN_ONLY);
    gpio_set_level(gpio_num, level);
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "camera power: force gpio=%d level=%d", gpio_num, gpio_get_level(gpio_num));
#else
    ESP_LOGI(TAG, "camera power: force level skipped (GPIO=-1)");
#endif
}

static void log_camera_component_options(void)
{
    ESP_LOGI(TAG,
             "camera options: sccb_port=%d sccb_freq=%u cam_task_stack=%d cam_task_core=%s dma_buf=%d",
             sccb_port_from_config(),
             (unsigned)CONFIG_SCCB_CLK_FREQ,
             CONFIG_CAMERA_TASK_STACK_SIZE,
             camera_task_core_name(),
             CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX);
#if CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE_CUSTOM
    ESP_LOGI(TAG, "camera options: jpeg_frame_size=custom(%d)", CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE);
#else
    ESP_LOGI(TAG, "camera options: jpeg_frame_size=auto");
#endif
}

static void log_sensor_status(const sensor_t *sensor)
{
    if (!sensor) {
        ESP_LOGW(TAG, "sensor status: unavailable");
        return;
    }

    const camera_status_t *st = &sensor->status;
    ESP_LOGI(TAG,
             "sensor status: framesize=%d quality=%u brightness=%d contrast=%d saturation=%d sharpness=%d",
             st->framesize,
             st->quality,
             st->brightness,
             st->contrast,
             st->saturation,
             st->sharpness);
    ESP_LOGI(TAG,
             "sensor status: awb=%u awb_gain=%u aec=%u aec2=%u ae_level=%d aec_value=%u",
             st->awb,
             st->awb_gain,
             st->aec,
             st->aec2,
             st->ae_level,
             st->aec_value);
    ESP_LOGI(TAG,
             "sensor status: agc=%u agc_gain=%u gainceiling=%u bpc=%u wpc=%u lenc=%u hmirror=%u vflip=%u",
             st->agc,
             st->agc_gain,
             st->gainceiling,
             st->bpc,
             st->wpc,
             st->lenc,
             st->hmirror,
             st->vflip);
}

static esp_err_t camera_init_and_log(void)
{
    (void)psram_ready();

    camera_config_t cfg = {
        .pin_pwdn     = CAMERA_PIN_PWDN,
        .pin_reset    = CAMERA_PIN_RESET,
        .pin_xclk     = CAMERA_PIN_XCLK,
        .pin_sccb_sda = CAMERA_PIN_SIOD,
        .pin_sccb_scl = CAMERA_PIN_SIOC,
        .pin_d7       = CAMERA_PIN_D7,
        .pin_d6       = CAMERA_PIN_D6,
        .pin_d5       = CAMERA_PIN_D5,
        .pin_d4       = CAMERA_PIN_D4,
        .pin_d3       = CAMERA_PIN_D3,
        .pin_d2       = CAMERA_PIN_D2,
        .pin_d1       = CAMERA_PIN_D1,
        .pin_d0       = CAMERA_PIN_D0,
        .pin_vsync    = CAMERA_PIN_VSYNC,
        .pin_href     = CAMERA_PIN_HREF,
        .pin_pclk     = CAMERA_PIN_PCLK,
        .xclk_freq_hz = CONFIG_CAMERA_XCLK_FREQ,
        .ledc_timer   = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_RGB565,
        .frame_size   = FRAMESIZE_QVGA,
        .jpeg_quality = 14,
        .fb_count     = 2,
        .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
        .fb_location  = CAMERA_FB_IN_PSRAM,
        .sccb_i2c_port = sccb_port_from_config(),
    };

    log_camera_component_options();
    ESP_LOGI(TAG, "camera module: %s", CAMERA_MODULE_NAME);
    ESP_LOGI(TAG, "camera request: xclk=%u fmt=%s frame=%s quality=%d fb_count=%u",
             (unsigned)cfg.xclk_freq_hz,
             pixformat_name(cfg.pixel_format),
             framesize_name(cfg.frame_size),
             cfg.jpeg_quality,
             (unsigned)cfg.fb_count);
    ESP_LOGI(TAG,
             "camera cfg: xclk=%u fmt=%s frame=%s quality=%d fb_count=%u fb_loc=%s sccb_port=%d",
             (unsigned)cfg.xclk_freq_hz,
             pixformat_name(cfg.pixel_format),
             framesize_name(cfg.frame_size),
             cfg.jpeg_quality,
             (unsigned)cfg.fb_count,
             fb_location_name(cfg.fb_location),
             cfg.sccb_i2c_port);
    ESP_LOGI(TAG,
             "camera pins: xclk=%d sda=%d scl=%d vsync=%d href=%d pclk=%d d0=%d d7=%d",
             cfg.pin_xclk,
             cfg.pin_sccb_sda,
             cfg.pin_sccb_scl,
             cfg.pin_vsync,
             cfg.pin_href,
             cfg.pin_pclk,
             cfg.pin_d0,
             cfg.pin_d7);

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_camera_init failed: %s", esp_err_to_name(err));
        return err;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (!sensor) {
        ESP_LOGW(TAG, "esp_camera_sensor_get returned NULL");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "sensor id: PID=0x%04x MIDH=0x%02x MIDL=0x%02x",
             sensor->id.PID, sensor->id.MIDH, sensor->id.MIDL);
    ESP_LOGI(TAG, "sensor addr: 0x%02x pixformat=%s xclk=%d",
             sensor->slv_addr,
             pixformat_name(sensor->pixformat),
             sensor->xclk_freq_hz);

    camera_sensor_info_t *info = esp_camera_sensor_get_info(&sensor->id);
    if (info) {
        ESP_LOGI(TAG, "sensor info: model=%d name=%s sccb=0x%02x max_size=%d jpeg=%s",
                 info->model,
                 info->name ? info->name : "unknown",
                 info->sccb_addr,
                 info->max_size,
                 info->support_jpeg ? "yes" : "no");
    }

    sensor->set_vflip(sensor, 1);
    if (sensor->id.PID == OV3660_PID) {
        sensor->set_brightness(sensor, 1);
        sensor->set_saturation(sensor, -2);
    }

    if (sensor->id.PID == OV3660_PID || sensor->id.PID == OV2640_PID) {
        sensor->set_vflip(sensor, 1);
    } else if (sensor->id.PID == GC0308_PID) {
        sensor->set_hmirror(sensor, 0);
    } else if (sensor->id.PID == GC032A_PID) {
        sensor->set_vflip(sensor, 1);
    }

    log_sensor_status(sensor);
    return ESP_OK;
}

static void log_frame_preview(const camera_fb_t *fb)
{
    if (!fb || !fb->buf || fb->len == 0) {
        ESP_LOGW(TAG, "frame preview: empty");
        return;
    }

    const size_t preview = fb->len < 8 ? fb->len : 8;
    char hex[3 * 8 + 1];
    size_t offset = 0;
    for (size_t i = 0; i < preview; ++i) {
        offset += (size_t)snprintf(hex + offset, sizeof(hex) - offset, "%02x ", fb->buf[i]);
    }
    ESP_LOGI(TAG, "frame preview: %s", hex);
}

void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    log_step("Step 0: boot");
    ESP_LOGI(TAG, "boot: atomS3R camera init test");
    ESP_LOGI(TAG, "chip: model=%d rev=%d cores=%d", chip_info.model, chip_info.revision, chip_info.cores);

    log_step("Step 1A: power polarity sweep (level=0)");
    camera_power_force_level(0);
    vTaskDelay(pdMS_TO_TICKS(CAMERA_POWER_DELAY_MS));

    log_step("Step 1B: power polarity sweep (level=1)");
    camera_power_force_level(1);
    vTaskDelay(pdMS_TO_TICKS(CAMERA_POWER_DELAY_MS));

    log_step("Step 1C: power enable (config)");
    camera_power_set(true);
    vTaskDelay(pdMS_TO_TICKS(CAMERA_POWER_DELAY_MS));

    log_step("Step 1D: power warmup delay");
    ESP_LOGI(TAG, "camera power: warmup delay=%u ms", (unsigned)CAMERA_WARMUP_DELAY_MS);
    vTaskDelay(pdMS_TO_TICKS(CAMERA_WARMUP_DELAY_MS));

    log_step("Step 2: camera init");
    esp_err_t err = camera_init_and_log();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "camera init failed: %s", esp_err_to_name(err));
    }

    log_step("Step 3: sccb probe (post-xclk)");
    if (err == ESP_OK) {
        camera_sccb_probe_known_addrs();
    } else {
        ESP_LOGW(TAG, "sccb probe skipped (camera init failed)");
    }

    log_step("Step 4: capture loop");
    uint32_t frame_count = 0;
    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "esp_camera_fb_get failed");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        ESP_LOGI(TAG,
                 "frame %" PRIu32 ": len=%u %ux%u fmt=%s ts=%ld.%06ld",
                 frame_count++,
                 (unsigned)fb->len,
                 (unsigned)fb->width,
                 (unsigned)fb->height,
                 pixformat_name(fb->format),
                 (long)fb->timestamp.tv_sec,
                 (long)fb->timestamp.tv_usec);
        log_frame_preview(fb);
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
