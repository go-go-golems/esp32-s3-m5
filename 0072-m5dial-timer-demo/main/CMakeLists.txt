idf_component_register(
    SRCS
        "app_main.cpp"
        "lvgl_port_m5dial.cpp"
        "m5dial_board.cpp"
        "timer_controller.cpp"
        "timer_model.cpp"
        "ui_timer_screen.cpp"
    REQUIRES lvgl LovyanGFX
    PRIV_REQUIRES esp_timer driver
    INCLUDE_DIRS "."
)
