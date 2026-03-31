#pragma once

#include "esp_err.h"
#include "sky_registry.h"
#include "sky_error.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SKY_CRASH_ACTION_NONE,
    SKY_CRASH_ACTION_LOG,
    SKY_CRASH_ACTION_STOP,
    SKY_CRASH_ACTION_SAFE_STATE,
    SKY_CRASH_ACTION_RESTART,
    SKY_CRASH_ACTION_REBOOT,
} sky_crash_action_t;

typedef struct {
    sky_crash_action_t on_task_watchdog;
    sky_crash_action_t on_component_error;
    sky_crash_action_t on_system_panic;
    void (*safe_state_handler)(void *ctx);
    void  *ctx;
    uint32_t watchdog_timeout_ms;
} sky_crash_config_t;

#define SKY_CRASH_CONFIG_DEFAULT() {                       \
    .on_task_watchdog   = SKY_CRASH_ACTION_SAFE_STATE,     \
    .on_component_error = SKY_CRASH_ACTION_LOG,            \
    .on_system_panic    = SKY_CRASH_ACTION_SAFE_STATE,     \
    .safe_state_handler = NULL,                            \
    .ctx                = NULL,                            \
    .watchdog_timeout_ms = 0,                              \
}

esp_err_t sky_crash_init(void);

esp_err_t sky_crash_register(sky_service_id_t service_id,
                             const sky_crash_config_t *config);

esp_err_t sky_crash_trigger(sky_service_id_t service_id,
                            sky_error_severity_t severity);

esp_err_t sky_crash_enter_safe_state_all(void);

#ifdef __cplusplus
}
#endif
