idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        console
        qjs_service
        picocalc_lcd
        picocalc_keyboard
        visual_repl
        picojs_runtime
        picoos_core
    PRIV_REQUIRES
        esp_psram
        esp_timer
        heap
)
