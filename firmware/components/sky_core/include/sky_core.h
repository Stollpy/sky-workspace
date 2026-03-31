#pragma once

#include "esp_err.h"
#include "sky_event.h"
#include "sky_events.h"
#include "sky_registry.h"
#include "sky_error.h"
#include "sky_config.h"
#include "sky_crash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    sky_event_bus_config_t event_bus;
    sky_registry_config_t  registry;
} sky_core_config_t;

#define SKY_CORE_DEFAULT_CONFIG() {                                         \
    .event_bus = { .max_subscribers = 32, .queue_size = 16, .async = true },\
    .registry  = { .max_services = 16 },                                    \
}

esp_err_t sky_core_init(void);
esp_err_t sky_core_init_with_config(const sky_core_config_t *config);
esp_err_t sky_core_start(void);
esp_err_t sky_core_stop(void);
void      sky_core_deinit(void);

#ifdef __cplusplus
}
#endif
