#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const char *sky_config_ns_t;

esp_err_t sky_config_init(void);

esp_err_t sky_config_set_u32(sky_config_ns_t ns, const char *key, uint32_t value);
esp_err_t sky_config_get_u32(sky_config_ns_t ns, const char *key, uint32_t *value);

esp_err_t sky_config_set_i32(sky_config_ns_t ns, const char *key, int32_t value);
esp_err_t sky_config_get_i32(sky_config_ns_t ns, const char *key, int32_t *value);

esp_err_t sky_config_set_str(sky_config_ns_t ns, const char *key, const char *value);
esp_err_t sky_config_get_str(sky_config_ns_t ns, const char *key, char *buf, size_t len);

esp_err_t sky_config_set_blob(sky_config_ns_t ns, const char *key,
                              const void *data, size_t len);
esp_err_t sky_config_get_blob(sky_config_ns_t ns, const char *key,
                              void *data, size_t *len);

esp_err_t sky_config_erase(sky_config_ns_t ns, const char *key);
esp_err_t sky_config_erase_ns(sky_config_ns_t ns);
void      sky_config_deinit(void);

#ifdef __cplusplus
}
#endif
