idf_component_register(
    SRCS
        "app_main.cpp"
        "demo_manager.cpp"
        "demo_menu.cpp"
        "demo_basics.cpp"
        "demo_pomodoro.cpp"
        "input_keyboard.cpp"
        "lvgl_port_cardputer_kb.cpp"
        "lvgl_port_m5gfx.cpp"
    PRIV_REQUIRES esp_timer
    REQUIRES M5GFX cardputer_kb lvgl
    INCLUDE_DIRS "."
)
