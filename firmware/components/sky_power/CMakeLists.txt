idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "include"
    REQUIRES sky_core
    PRIV_REQUIRES driver esp_adc esp_timer esp_wifi esp_hw_support
)
