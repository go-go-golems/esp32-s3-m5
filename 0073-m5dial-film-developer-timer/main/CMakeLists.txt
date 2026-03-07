idf_component_register(
    SRCS
        "app_main.cpp"
        "lvgl_port_m5dial.cpp"
        "m5dial_board.cpp"
        "timer_controller.cpp"
        "timer_model.cpp"
        "ui_timer_screen.cpp"
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/Button/Button.cpp"
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/ESP32Encoder/src/ESP32Encoder.cpp"
    REQUIRES lvgl LovyanGFX
    PRIV_REQUIRES esp_timer driver
    INCLUDE_DIRS
        "."
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/Button"
        "/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Dial-UserDemo/main/hal/utils/ESP32Encoder/src"
)
