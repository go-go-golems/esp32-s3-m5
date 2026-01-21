idf_component_register(
    SRCS
        "app_main.cpp"
        "lvgl_port_m5gfx.cpp"
        "chain_encoder_uart.cpp"
    PRIV_REQUIRES esp_timer esp_driver_uart
    REQUIRES M5GFX lvgl
    INCLUDE_DIRS "."
)

