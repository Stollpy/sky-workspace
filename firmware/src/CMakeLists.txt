idf_component_register(
    SRCS "main.c"
         "luxia/luxia.c"
         "luxia/luxia_cmd.c"
         "luxia/luxia_http.c"
    INCLUDE_DIRS "." "luxia"
    REQUIRES sky_core sky_stepper sky_wifi sky_matter sky_ota sky_power sky_http_server
    PRIV_REQUIRES json
)
