#pragma once

extern "C" {
#include "lvgl.h"
}

static inline const lv_font_t *lvgl_font_small(void) {
#if defined(LV_FONT_MONTSERRAT_12) && LV_FONT_MONTSERRAT_12
    return &lv_font_montserrat_12;
#elif defined(LV_FONT_MONTSERRAT_14) && LV_FONT_MONTSERRAT_14
    return &lv_font_montserrat_14;
#else
    return LV_FONT_DEFAULT;
#endif
}

static inline const lv_font_t *lvgl_font_body(void) {
#if defined(LV_FONT_MONTSERRAT_14) && LV_FONT_MONTSERRAT_14
    return &lv_font_montserrat_14;
#else
    return LV_FONT_DEFAULT;
#endif
}

static inline const lv_font_t *lvgl_font_time_big(void) {
#if defined(LV_FONT_MONTSERRAT_32) && LV_FONT_MONTSERRAT_32
    return &lv_font_montserrat_32;
#elif defined(LV_FONT_MONTSERRAT_28) && LV_FONT_MONTSERRAT_28
    return &lv_font_montserrat_28;
#elif defined(LV_FONT_MONTSERRAT_24) && LV_FONT_MONTSERRAT_24
    return &lv_font_montserrat_24;
#elif defined(LV_FONT_MONTSERRAT_20) && LV_FONT_MONTSERRAT_20
    return &lv_font_montserrat_20;
#elif defined(LV_FONT_MONTSERRAT_18) && LV_FONT_MONTSERRAT_18
    return &lv_font_montserrat_18;
#else
    return LV_FONT_DEFAULT;
#endif
}

