idf_component_register(
    SRCS
        "app_main.cpp"
        "m5dial_board.cpp"
    REQUIRES lvgl LovyanGFX
    PRIV_REQUIRES esp_timer driver
    INCLUDE_DIRS "."
)
