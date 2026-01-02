idf_component_register(
    SRCS
        "app_main.cpp"
        "input_keyboard.cpp"
        "lvgl_port_cardputer_kb.cpp"
        "lvgl_port_m5gfx.cpp"
    PRIV_REQUIRES esp_timer
    REQUIRES M5GFX cardputer_kb lvgl
    INCLUDE_DIRS "."
)

