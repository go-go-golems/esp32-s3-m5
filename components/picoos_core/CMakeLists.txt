idf_component_register(
    SRCS "picoos_core.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer freertos log picojs_runtime qjs_service
)
