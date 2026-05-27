/**
 * @file display_app.c
 *
 * Tab5 UI screen viewer display bring-up.
 *
 * Initializes the MIPI DSI display, allocates a full-screen SPIRAM buffer
 * for the uploaded image, and creates a fullscreen LVGL image object that
 * can be updated via POST /api/upload.
 */

#include "display_app.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"

#include "bsp/m5stack_tab5.h"

/* Screen dimensions in landscape orientation (after ROTATION_90) */
#define SCREEN_W  1280
#define SCREEN_H  720
#define SCREEN_BPP 2  /* bytes per pixel (RGB565) */
#define SCREEN_BUF_SIZE (SCREEN_W * SCREEN_H * SCREEN_BPP)
#define SCREEN_STRIDE (SCREEN_W * SCREEN_BPP)

static const char *TAG = "display";

/* Full-screen pixel buffer in SPIRAM, cleared to black on init. */
static uint8_t *s_screen_buf = NULL;

/* LVGL image descriptor pointing at s_screen_buf. */
static lv_image_dsc_t s_screen_dsc;

/* The fullscreen image object on the active screen. */
static lv_obj_t *s_screen_img = NULL;

/* True once at least one upload has been received. */
static bool s_has_image = false;

/* ---- Public API ---- */

uint8_t *display_app_get_buffer(void)
{
    return s_screen_buf;
}

lv_obj_t *display_app_get_image_obj(void)
{
    return s_screen_img;
}

int display_app_get_width(void)
{
    return SCREEN_W;
}

int display_app_get_height(void)
{
    return SCREEN_H;
}

size_t display_app_get_buf_size(void)
{
    return SCREEN_BUF_SIZE;
}

bool display_app_has_image(void)
{
    return s_has_image;
}

void display_app_clear(void)
{
    if (s_screen_buf) {
        memset(s_screen_buf, 0, SCREEN_BUF_SIZE);
    }
    s_has_image = false;
    display_app_invalidate();
}

void display_app_invalidate(void)
{
    if (s_screen_img) {
        s_has_image = true;
        bsp_display_lock(0);
        lv_obj_invalidate(s_screen_img);
        bsp_display_unlock();
    }
}

/* ---- Init ---- */

esp_err_t display_app_init(void)
{
    ESP_LOGI(TAG, "Initializing Tab5 display via BSP bring-up path");

    /* 1. Shared I2C bus: required by the PI4IOE GPIO expanders. */
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "shared I2C init failed");
    i2c_master_bus_handle_t i2c = bsp_i2c_get_handle();
    ESP_RETURN_ON_FALSE(i2c != NULL, ESP_ERR_INVALID_STATE, TAG, "I2C handle is NULL");

    /* 2. Board-level GPIO expanders. */
    bsp_io_expander_pi4ioe_init(i2c);

    /* 3. Reset touch/display lines before starting the display. */
    bsp_reset_tp();

    /* 4. Start display through the Tab5 BSP wrapper. */
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size   = BSP_LCD_H_RES * BSP_LCD_V_RES,
        .double_buffer = true,
        .flags = {
#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
            .buff_dma = false,
#else
            .buff_dma = true,
#endif
            .buff_spiram = true,
            .sw_rotate   = true,
        },
    };

    lv_display_t *disp = bsp_display_start_with_config(&cfg);
    ESP_RETURN_ON_FALSE(disp != NULL, ESP_FAIL, TAG, "bsp_display_start_with_config failed");

    /* 5. Portrait physical panel, landscape logical drawing coordinates. */
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
    ESP_RETURN_ON_ERROR(bsp_display_backlight_on(), TAG, "backlight enable failed");

    /* 6. Allocate the full-screen SPIRAM buffer and fill with black. */
    s_screen_buf = heap_caps_malloc(SCREEN_BUF_SIZE, MALLOC_CAP_SPIRAM);
    ESP_RETURN_ON_FALSE(s_screen_buf != NULL, ESP_ERR_NO_MEM, TAG,
                        "Failed to allocate screen buffer (%d bytes) in SPIRAM", SCREEN_BUF_SIZE);
    memset(s_screen_buf, 0, SCREEN_BUF_SIZE);
    ESP_LOGI(TAG, "Screen buffer allocated: %d bytes in SPIRAM", SCREEN_BUF_SIZE);

    /* 7. Set up the LVGL image descriptor (LVGL 9 format).
     *
     * The header must include LV_IMAGE_HEADER_MAGIC, the color format
     * LV_COLOR_FORMAT_RGB565, and the stride (bytes per row).  The data
     * pointer points at the SPIRAM buffer. */
    s_screen_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_screen_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    s_screen_dsc.header.flags = 0;
    s_screen_dsc.header.w = SCREEN_W;
    s_screen_dsc.header.h = SCREEN_H;
    s_screen_dsc.header.stride = SCREEN_STRIDE;
    s_screen_dsc.data_size = SCREEN_BUF_SIZE;
    s_screen_dsc.data = s_screen_buf;

    /* 8. Create a fullscreen image object on the active screen.
     *
     * Must hold the LVGL lock because the LVGL task is already running
     * (started by bsp_display_start_with_config). Without the lock,
     * lv_obj_invalidate triggers an assertion during rendering. */
    bsp_display_lock(0);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    s_screen_img = lv_img_create(scr);
    lv_img_set_src(s_screen_img, &s_screen_dsc);
    lv_obj_center(s_screen_img);

    bsp_display_unlock();

    ESP_LOGI(TAG, "Display initialized — black screen ready for image upload");
    return ESP_OK;
}
