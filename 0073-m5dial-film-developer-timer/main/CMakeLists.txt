idf_component_register(
    SRCS
        "app_main.cpp"
        "film_catalog.cpp"
        "film_selector_controller.cpp"
        "film_selector_screen.cpp"
        "film_process_screen.cpp"
        "film_timer_controller.cpp"
        "film_timer_model.cpp"
        "generated_film_catalog.cpp"
        "lvgl_port_m5dial.cpp"
        "m5dial_board.cpp"
        "recipe_selector_model.cpp"
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/Button/Button.cpp"
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/ESP32Encoder/src/ESP32Encoder.cpp"
    REQUIRES lvgl LovyanGFX
    PRIV_REQUIRES esp_timer driver
    INCLUDE_DIRS
        "."
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/Button"
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/ESP32Encoder/src"
)
