#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Power modes ────────────────────────────────────────────────────── */

typedef enum {
    SKY_POWER_MODE_ACTIVE,
    SKY_POWER_MODE_MODEM_SLEEP,
    SKY_POWER_MODE_LIGHT_SLEEP,
    SKY_POWER_MODE_DEEP_SLEEP,
} sky_power_mode_t;

/* ── Wake reason ────────────────────────────────────────────────────── */

typedef enum {
    SKY_WAKE_REASON_POWERON,
    SKY_WAKE_REASON_TIMER,
    SKY_WAKE_REASON_GPIO,
    SKY_WAKE_REASON_UNKNOWN,
} sky_wake_reason_t;

/* ── Configuration ──────────────────────────────────────────────────── */

typedef struct {
    sky_power_mode_t idle_mode;
    uint32_t         idle_timeout_ms;
    uint32_t         deep_sleep_duration_ms;
    gpio_num_t       wake_gpio;
    bool             wake_on_high;
    bool             save_state_before_sleep;
} sky_sleep_config_t;

#define SKY_SLEEP_CONFIG_DEFAULT() {                   \
    .idle_mode              = SKY_POWER_MODE_MODEM_SLEEP, \
    .idle_timeout_ms        = 30000,                   \
    .deep_sleep_duration_ms = 0,                       \
    .wake_gpio              = GPIO_NUM_MAX,            \
    .wake_on_high           = true,                    \
    .save_state_before_sleep = true,                   \
}

/* ── Event data ─────────────────────────────────────────────────────── */

typedef struct {
    sky_power_mode_t mode;
    uint32_t         duration_ms;
} sky_sleep_enter_event_t;

typedef struct {
    sky_wake_reason_t reason;
    sky_power_mode_t  prev_mode;
} sky_sleep_exit_event_t;

typedef struct {
    sky_power_mode_t old_mode;
    sky_power_mode_t new_mode;
} sky_power_mode_changed_event_t;

/* ── Wakelocks ──────────────────────────────────────────────────────── */

typedef uint32_t sky_wakelock_t;

/* ── API ────────────────────────────────────────────────────────────── */

esp_err_t         sky_sleep_init(const sky_sleep_config_t *config);
esp_err_t         sky_sleep_set_mode(sky_power_mode_t mode);
sky_power_mode_t  sky_sleep_get_mode(void);
esp_err_t         sky_sleep_acquire_wakelock(const char *reason,
                                             sky_wakelock_t *lock);
esp_err_t         sky_sleep_release_wakelock(sky_wakelock_t lock);
bool              sky_sleep_has_wakelocks(void);
esp_err_t         sky_sleep_enter_deep_sleep(uint32_t duration_ms);
sky_wake_reason_t sky_sleep_get_wake_reason(void);
void              sky_sleep_deinit(void);

#ifdef __cplusplus
}
#endif
