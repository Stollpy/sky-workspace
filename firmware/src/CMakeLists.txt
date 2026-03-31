idf_component_register(
    SRCS "main.c" "luxia_cmd.c" "luxia_http.c"
    INCLUDE_DIRS "."
    REQUIRES sky_core sky_stepper sky_wifi sky_matter sky_ota sky_power sky_http_server
    PRIV_REQUIRES json
)
