#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const char *sky_service_id_t;

typedef struct {
    esp_err_t (*init)(void *config);
    esp_err_t (*start)(void);
    esp_err_t (*stop)(void);
    esp_err_t (*deinit)(void);
} sky_service_lifecycle_t;

typedef struct {
    uint8_t max_services;
} sky_registry_config_t;

esp_err_t sky_registry_init(const sky_registry_config_t *config);

esp_err_t sky_registry_register(sky_service_id_t id,
                                const sky_service_lifecycle_t *lifecycle,
                                void *config);

/**
 * Store an opaque instance pointer for a service.
 * Typically called by the service during its init() callback.
 */
esp_err_t sky_registry_set_instance(sky_service_id_t id, void *instance);

/**
 * Retrieve the instance pointer previously set via sky_registry_set_instance().
 * Returns the config pointer if no instance was explicitly set.
 */
void *sky_registry_get(sky_service_id_t id);

esp_err_t sky_registry_start_all(void);
esp_err_t sky_registry_stop_all(void);
void      sky_registry_deinit(void);

#ifdef __cplusplus
}
#endif
