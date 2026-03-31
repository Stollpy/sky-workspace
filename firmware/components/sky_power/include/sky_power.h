#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration ──────────────────────────────────────────────────── */

typedef struct {
    gpio_num_t adc_pin;
    int        adc_atten;
    float      voltage_divider_ratio;
    float      voltage_max;
    float      voltage_min;
    uint8_t    threshold_low;
    uint8_t    threshold_critical;
    uint32_t   sample_interval_ms;
    uint8_t    sample_average_count;
    bool       block_motor_on_critical;
    bool       auto_sleep_on_critical;
} sky_power_config_t;

#define SKY_POWER_ADC_ATTEN_12DB  3

#define SKY_POWER_CONFIG_LIPO_DEFAULT() {          \
    .adc_pin                = GPIO_NUM_34,         \
    .adc_atten              = SKY_POWER_ADC_ATTEN_12DB, \
    .voltage_divider_ratio  = 2.0f,                \
    .voltage_max            = 4.2f,                \
    .voltage_min            = 3.0f,                \
    .threshold_low          = 20,                  \
    .threshold_critical     = 5,                   \
    .sample_interval_ms     = 60000,               \
    .sample_average_count   = 16,                  \
    .block_motor_on_critical = true,               \
    .auto_sleep_on_critical  = false,              \
}

/* ── Status ─────────────────────────────────────────────────────────── */

typedef struct {
    float   voltage;
    uint8_t percent;
    bool    charging;
    bool    low;
    bool    critical;
} sky_power_status_t;

/* ── Event data ─────────────────────────────────────────────────────── */

typedef struct {
    uint8_t percent;
    float   voltage;
} sky_power_battery_event_t;

/* ── API ────────────────────────────────────────────────────────────── */

esp_err_t sky_power_init(const sky_power_config_t *config);
esp_err_t sky_power_start(void);
esp_err_t sky_power_get_status(sky_power_status_t *status);
uint8_t   sky_power_get_percent(void);
float     sky_power_get_voltage(void);
bool      sky_power_is_low(void);
bool      sky_power_is_critical(void);
esp_err_t sky_power_stop(void);
void      sky_power_deinit(void);

#ifdef __cplusplus
}
#endif
