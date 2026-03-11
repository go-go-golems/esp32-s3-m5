idf_component_register(
    SRCS
        "wifi_console.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        wifi_mgr
        console
        esp_wifi
        esp_netif
        lwip
        esp_driver_usb_serial_jtag
)

