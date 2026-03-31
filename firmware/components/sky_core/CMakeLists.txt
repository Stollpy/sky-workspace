idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_timer
    PRIV_REQUIRES driver esp_system
)
