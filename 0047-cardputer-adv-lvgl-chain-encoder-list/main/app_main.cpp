/*
 * Tutorial 0047 — Cardputer-ADV LVGL list navigation via Chain Encoder (U207 over Grove UART).
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "M5GFX.h"

#include "chain_encoder_uart.h"
#include "lvgl_port_m5gfx.h"

static const char *TAG = "0047";

namespace {

static lv_group_t *s_group = nullptr;
static lv_obj_t *s_list = nullptr;
static lv_obj_t *s_status = nullptr;

static ChainEncoderUart *s_enc = nullptr;
static lv_indev_t *s_enc_indev = nullptr;

static void list_btn_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *list = s_list ? s_list : lv_event_get_user_data(e) ? static_cast<lv_obj_t *>(lv_event_get_user_data(e))
                                                                 : nullptr;
    const char *txt = list ? lv_list_get_btn_text(list, btn) : nullptr;
    ESP_LOGI(TAG, "clicked: %s", txt ? txt : "(null)");
    if (s_status) {
        lv_label_set_text_fmt(s_status, "clicked: %s", txt ? txt : "(null)");
    }
}

static void enc_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    (void)drv;

    int32_t d = 0;
    if (s_enc) {
        d = s_enc->take_delta();
    }

    const int clamp = CONFIG_TUTORIAL_0047_DELTA_CLAMP;
    if (clamp > 0) {
        d = std::clamp(d, (int32_t)-clamp, (int32_t)clamp);
    }
    data->enc_diff = (int16_t)std::clamp(d, (int32_t)-32768, (int32_t)32767);

    // Synthesize a short press/release pulse from a pending click event.
    static bool pulse_pressed = false;
    if (s_enc && s_enc->has_click_pending()) {
        if (!pulse_pressed) {
            pulse_pressed = true;
            data->state = LV_INDEV_STATE_PR;
            return;
        }

        pulse_pressed = false;
        s_enc->clear_click_pending();
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    data->state = LV_INDEV_STATE_REL;
}

static lv_indev_t *lvgl_port_chain_encoder_init(void) {
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = enc_read_cb;
    return lv_indev_drv_register(&indev_drv);
}

static void create_ui(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_text_color(scr, lv_palette_main(LV_PALETTE_GREEN), 0);

    static lv_style_t s_btn_base;
    static lv_style_t s_btn_focused;
    static bool s_styles_inited = false;
    if (!s_styles_inited) {
        s_styles_inited = true;

        lv_style_init(&s_btn_base);
        lv_style_set_bg_opa(&s_btn_base, LV_OPA_TRANSP);
        lv_style_set_border_width(&s_btn_base, 0);
        lv_style_set_radius(&s_btn_base, 6);
        lv_style_set_pad_all(&s_btn_base, 6);
        lv_style_set_text_color(&s_btn_base, lv_palette_main(LV_PALETTE_GREEN));

        lv_style_init(&s_btn_focused);
        lv_style_set_bg_opa(&s_btn_focused, LV_OPA_COVER);
        lv_style_set_bg_color(&s_btn_focused, lv_palette_main(LV_PALETTE_GREEN));
        lv_style_set_text_color(&s_btn_focused, lv_color_black());
    }

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Chain Encoder List (0047)");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *list = lv_list_create(scr);
    s_list = list;
    lv_obj_set_size(list, 240 - 16, 135 - 38);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);

    s_group = lv_group_create();
    lv_group_set_wrap(s_group, true);

    for (int i = 1; i <= 24; i++) {
        char label[32];
        snprintf(label, sizeof(label), "Item %02d", i);
        lv_obj_t *btn = lv_list_add_btn(list, nullptr, label);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_add_style(btn, &s_btn_base,
                         (lv_style_selector_t)((lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT));
        lv_obj_add_style(btn, &s_btn_focused,
                         (lv_style_selector_t)((lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_FOCUSED));
        lv_obj_add_event_cb(btn, list_btn_cb, LV_EVENT_CLICKED, list);
        lv_group_add_obj(s_group, btn);
        if (i == 1) lv_group_focus_obj(btn);
    }

    s_status = lv_label_create(scr);
    lv_obj_set_style_text_font(s_status, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_status, "rotate: select   click: activate");
    lv_obj_align(s_status, LV_ALIGN_BOTTOM_MID, 0, -2);

    if (s_enc_indev && s_group) {
        lv_indev_set_group(s_enc_indev, s_group);
    }
}

} // namespace

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "boot; free_heap=%" PRIu32 " dma_free=%" PRIu32, esp_get_free_heap_size(),
             (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DMA));

    m5gfx::M5GFX display;
    ESP_LOGI(TAG, "display init via M5GFX autodetect...");
    display.init();
    display.setColorDepth(16);

    ESP_LOGI(TAG, "display ready: %dx%d", (int)display.width(), (int)display.height());

    LvglM5gfxConfig lv_cfg{};
    lv_cfg.buffer_lines = 40;
    lv_cfg.double_buffer = true;
    lv_cfg.swap_bytes = false;
    lv_cfg.tick_ms = 2;

    if (!lvgl_port_m5gfx_init(display, lv_cfg)) {
        ESP_LOGE(TAG, "lvgl_port_m5gfx_init failed");
        return;
    }

    static ChainEncoderUart enc(ChainEncoderUart::Config{
        .uart_num = CONFIG_TUTORIAL_0047_UART_NUM,
        .baud = CONFIG_TUTORIAL_0047_UART_BAUD,
        .tx_gpio = CONFIG_TUTORIAL_0047_UART_TX_GPIO,
        .rx_gpio = CONFIG_TUTORIAL_0047_UART_RX_GPIO,
        .index_id = (uint8_t)CONFIG_TUTORIAL_0047_CHAIN_INDEX_ID,
        .poll_ms = CONFIG_TUTORIAL_0047_POLL_MS,
    });
    s_enc = &enc;
    if (!enc.init()) {
        ESP_LOGW(TAG, "chain encoder init failed; UI will still render, but no encoder input");
    }

    s_enc_indev = lvgl_port_chain_encoder_init();
    create_ui();

    while (true) {
        lv_timer_handler();
        vTaskDelay(1);
    }
}
