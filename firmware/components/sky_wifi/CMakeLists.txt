idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "include"
    REQUIRES sky_core
    PRIV_REQUIRES esp_wifi esp_event esp_netif esp_timer
)
