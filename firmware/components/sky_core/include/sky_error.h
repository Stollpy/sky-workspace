#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SKY_SEVERITY_INFO,
    SKY_SEVERITY_WARNING,
    SKY_SEVERITY_ERROR,
    SKY_SEVERITY_CRITICAL,
    SKY_SEVERITY_FATAL,
} sky_error_severity_t;

typedef struct {
    esp_err_t              code;
    sky_error_severity_t   severity;
    const char            *component;
    const char            *message;
    uint32_t               timestamp_ms;
} sky_error_t;

typedef void (*sky_error_handler_t)(const sky_error_t *error, void *ctx);

esp_err_t sky_error_init(void);

esp_err_t sky_error_set_handler(sky_error_severity_t min_severity,
                                sky_error_handler_t handler,
                                void *ctx);

void sky_error_report(esp_err_t code,
                      sky_error_severity_t severity,
                      const char *component,
                      const char *message);

#ifdef __cplusplus
}
#endif
