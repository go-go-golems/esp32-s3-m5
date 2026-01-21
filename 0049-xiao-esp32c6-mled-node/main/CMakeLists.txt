idf_component_register(
    SRCS
        "app_main.c"
        "wifi_mgr.c"
        "wifi_console.c"
    PRIV_REQUIRES
        nvs_flash
        esp_event
        esp_netif
        esp_wifi
        esp_timer
        esp_driver_usb_serial_jtag
        console
        mled_node
    INCLUDE_DIRS "."
)
