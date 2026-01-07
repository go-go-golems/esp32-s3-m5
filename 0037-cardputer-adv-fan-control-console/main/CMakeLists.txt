idf_component_register(
    SRCS
        "app_main.c"
        "fan_console.c"
        "fan_relay.c"
    PRIV_REQUIRES
        console
        esp_driver_gpio
    INCLUDE_DIRS
        "."
)

