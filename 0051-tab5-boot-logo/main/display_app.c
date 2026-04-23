/**
 * @file display_app.c
 *
 * Tab5 boot logo display bring-up.
 *
 * This revision intentionally stops hand-rolling the low-level ST7123 + DSI
 * sequence in the application and instead follows the factory firmware's board
 * preparation order more closely:
 *
 *   1. Initialize the shared I2C bus.
 *   2. Initialize the PI4IOE I/O expanders.
 *   3. Reset the touch/LCD rails through the expander-controlled lines.
 *   4. Use the Tab5 BSP wrapper to start the display and LVGL.
 *   5. Turn on the backlight.
 *   6. Render the logo with LVGL.
 *
 * The goal is to satisfy the board-level display prerequisites before talking
 * to the ST7123 panel over MIPI DSI.
 */

#include "display_app.h"

#include "esp_check.h"
#include "esp_log.h"

#include "m5stack_tab5.h"

static const char *TAG = "display";

LV_IMG_DECLARE(logo_tab)

esp_err_t display_app_init(void)
{
    ESP_LOGI(TAG, "Initializing Tab5 display via BSP bring-up path");

    /* 1. Shared I2C bus: required by the PI4IOE GPIO expanders that control
     * LCD reset / external power rails. */
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "shared I2C init failed");
    i2c_master_bus_handle_t i2c = bsp_i2c_get_handle();
    ESP_RETURN_ON_FALSE(i2c != NULL, ESP_ERR_INVALID_STATE, TAG, "I2C handle is NULL");

    /* 2. Board-level GPIO expanders. These assert signals such as LCD_RST,
     * TP_RST, EXT5V_EN, WLAN_PWR_EN, etc. */
    bsp_io_expander_pi4ioe_init(i2c);

    /* 3. Match the factory sequence: reset the touch/display-related lines
     * before starting the display. This function toggles the expander outputs
     * that include LCD_RST and TP_RST. */
    bsp_reset_tp();

    /* 4. Let the BSP own the ST7123 + DSI + LVGL wiring.
     *
     * Factory reference:
     *   - full-screen buffers in SPIRAM
     *   - double-buffered
     *   - software rotation enabled on P4
     */
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

    /* 6. Render a simple black background + centered logo. */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *img = lv_img_create(scr);
    lv_img_set_src(img, &logo_tab);
    lv_obj_center(img);

    ESP_LOGI(TAG, "Display initialized -- M5 logo rendered on screen");
    return ESP_OK;
}
