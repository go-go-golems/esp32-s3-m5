idf_component_register(
    SRCS
        "app_main.c"
        "wifi_mgr.c"
        "wifi_console.c"
        "mled_node.c"
        "mled_protocol.c"
        "mled_time.c"
    PRIV_REQUIRES
        nvs_flash
        esp_event
        esp_netif
        esp_wifi
        esp_timer
        esp_driver_usb_serial_jtag
        console
    INCLUDE_DIRS "."
)
