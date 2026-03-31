#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t sky_event_id_t;

typedef struct {
    sky_event_id_t event_id;
    void          *data;
    size_t         data_size;
    uint32_t       timestamp_ms;
} sky_event_t;

typedef void (*sky_event_handler_t)(const sky_event_t *event, void *ctx);

typedef struct {
    uint16_t max_subscribers;
    uint16_t queue_size;
    bool     async;
} sky_event_bus_config_t;

esp_err_t sky_event_bus_init(const sky_event_bus_config_t *config);

esp_err_t sky_event_subscribe(sky_event_id_t event_id,
                              sky_event_handler_t handler,
                              void *ctx);

esp_err_t sky_event_unsubscribe(sky_event_id_t event_id,
                                sky_event_handler_t handler);

esp_err_t sky_event_publish(sky_event_id_t event_id,
                            const void *data,
                            size_t data_size);

void sky_event_bus_deinit(void);

#ifdef __cplusplus
}
#endif
