idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "include"
    REQUIRES sky_core
    PRIV_REQUIRES esp_https_ota esp_http_client app_update esp_timer
)
