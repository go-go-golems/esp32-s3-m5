idf_component_register(
    SRCS
        "wifi_mgr.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        nvs_flash
        esp_event
        esp_netif
        esp_wifi
        lwip
)

