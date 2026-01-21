idf_component_register(
    SRCS
        "src/mled_node.c"
        "src/mled_protocol.c"
        "src/mled_time.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        esp_hw_support
        esp_system
        esp_timer
        esp_wifi
        log
        lwip
)

