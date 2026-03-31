#include "sky_power.h"
#include "sky_config.h"
#include "sky_event.h"
#include "sky_events.h"
#include "sky_registry.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <string.h>

static const char *TAG = "sky_power";

#ifndef CONFIG_SKY_POWER_BATTERY_THRESHOLD_LOW
#define CONFIG_SKY_POWER_BATTERY_THRESHOLD_LOW 20
#endif
#ifndef CONFIG_SKY_POWER_BATTERY_THRESHOLD_CRITICAL
#define CONFIG_SKY_POWER_BATTERY_THRESHOLD_CRITICAL 5
#endif

/* ── LiPo discharge curve ───────────────────────────────────────────── */

typedef struct { float voltage; uint8_t percent; } curve_point_t;

static const curve_point_t lipo_curve[] = {
    {4.20f, 100}, {4.15f,  95}, {4.11f,  90}, {4.08f,  85},
    {4.02f,  80}, {3.98f,  75}, {3.95f,  70}, {3.91f,  65},
    {3.87f,  60}, {3.85f,  55}, {3.84f,  50}, {3.82f,  45},
    {3.80f,  40}, {3.79f,  35}, {3.77f,  30}, {3.75f,  25},
    {3.73f,  20}, {3.71f,  15}, {3.69f,  10}, {3.61f,   5},
    {3.27f,   0},
};
#define CURVE_LEN (sizeof(lipo_curve) / sizeof(lipo_curve[0]))

/* ── GPIO → ADC1 channel mapping (ESP32) ────────────────────────────── */

static adc_channel_t gpio_to_channel(gpio_num_t pin)
{
    switch ((int)pin) {
    case 36: return ADC_CHANNEL_0;
    case 37: return ADC_CHANNEL_1;
    case 38: return ADC_CHANNEL_2;
    case 39: return ADC_CHANNEL_3;
    case 32: return ADC_CHANNEL_4;
    case 33: return ADC_CHANNEL_5;
    case 34: return ADC_CHANNEL_6;
    case 35: return ADC_CHANNEL_7;
    default: return ADC_CHANNEL_6;
    }
}

/* ── Module state ───────────────────────────────────────────────────── */

static struct {
    sky_power_config_t         config;
    bool                       initialized;
    bool                       started;
    bool                       registered;
    adc_oneshot_unit_handle_t  adc;
    adc_cali_handle_t          cali;
    adc_channel_t              channel;
    float                      voltage;
    uint8_t                    percent;
    float                      prev_voltage;
    bool                       low_active;
    bool                       critical_active;
    esp_timer_handle_t         sample_timer;
} s_pwr;

/* ── Forward declarations ───────────────────────────────────────────── */

static esp_err_t svc_init(void *cfg);
static esp_err_t svc_start(void);
static esp_err_t svc_stop(void);
static esp_err_t svc_deinit(void);
static void      sample_timer_cb(void *arg);

/* ── Voltage → percent (linear interpolation on LiPo curve) ─────────── */

static uint8_t voltage_to_percent(float v)
{
    if (v >= lipo_curve[0].voltage) return 100;
    if (v <= lipo_curve[CURVE_LEN - 1].voltage) return 0;

    for (size_t i = 0; i < CURVE_LEN - 1; i++) {
        if (v >= lipo_curve[i + 1].voltage) {
            float range = lipo_curve[i].voltage - lipo_curve[i + 1].voltage;
            float delta = v - lipo_curve[i + 1].voltage;
            uint8_t pct_range = lipo_curve[i].percent - lipo_curve[i + 1].percent;
            return lipo_curve[i + 1].percent +
                   (uint8_t)(delta / range * (float)pct_range);
        }
    }
    return 0;
}

/* ── ADC read with averaging ────────────────────────────────────────── */

static float read_voltage(void)
{
    int total = 0;
    int count = s_pwr.config.sample_average_count;
    if (count < 1) count = 1;

    for (int i = 0; i < count; i++) {
        int raw = 0;
        adc_oneshot_read(s_pwr.adc, s_pwr.channel, &raw);
        total += raw;
    }
    int avg = total / count;

    int mv = 0;
    if (s_pwr.cali) {
        adc_cali_raw_to_voltage(s_pwr.cali, avg, &mv);
    } else {
        mv = (int)((float)avg * 3300.0f / 4095.0f);
    }

    return (float)mv / 1000.0f * s_pwr.config.voltage_divider_ratio;
}

/* ── Threshold check + events ───────────────────────────────────────── */

static void check_thresholds(void)
{
    sky_power_battery_event_t evt = {
        .percent = s_pwr.percent,
        .voltage = s_pwr.voltage,
    };

    bool was_critical = s_pwr.critical_active;
    bool was_low      = s_pwr.low_active;

    s_pwr.critical_active = s_pwr.percent <= s_pwr.config.threshold_critical;
    s_pwr.low_active      = s_pwr.percent <= s_pwr.config.threshold_low;

    if (s_pwr.critical_active && !was_critical) {
        ESP_LOGW(TAG, "CRITICAL: %d%% (%.2fV)", s_pwr.percent, s_pwr.voltage);
        sky_event_publish(SKY_EVENT_BATTERY_CRITICAL, &evt, sizeof(evt));
    } else if (s_pwr.low_active && !was_low) {
        ESP_LOGW(TAG, "LOW: %d%% (%.2fV)", s_pwr.percent, s_pwr.voltage);
        sky_event_publish(SKY_EVENT_BATTERY_LOW, &evt, sizeof(evt));
    } else if (!s_pwr.low_active && was_low) {
        ESP_LOGI(TAG, "OK: %d%% (%.2fV)", s_pwr.percent, s_pwr.voltage);
        sky_event_publish(SKY_EVENT_BATTERY_OK, &evt, sizeof(evt));
    }

    if (s_pwr.voltage > s_pwr.prev_voltage + 0.05f) {
        sky_event_publish(SKY_EVENT_BATTERY_CHARGING, &evt, sizeof(evt));
    }
    s_pwr.prev_voltage = s_pwr.voltage;
}

/* ── Sampling timer callback ────────────────────────────────────────── */

static void sample_timer_cb(void *arg)
{
    (void)arg;
    s_pwr.voltage = read_voltage();
    s_pwr.percent = voltage_to_percent(s_pwr.voltage);
    check_thresholds();
}

/* ── Service lifecycle ──────────────────────────────────────────────── */

static esp_err_t svc_init(void *cfg)  { (void)cfg; return ESP_OK; }
static esp_err_t svc_start(void)      { return sky_power_start(); }
static esp_err_t svc_stop(void)       { return sky_power_stop(); }
static esp_err_t svc_deinit(void)     { sky_power_deinit(); return ESP_OK; }

static esp_err_t ensure_registered(void)
{
    if (s_pwr.registered) return ESP_OK;
    static const sky_service_lifecycle_t lc = {
        .init = svc_init, .start = svc_start,
        .stop = svc_stop, .deinit = svc_deinit,
    };
    esp_err_t err = sky_registry_register("sky_power", &lc, NULL);
    if (err != ESP_OK) return err;
    s_pwr.registered = true;
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t sky_power_init(const sky_power_config_t *config)
{
    if (s_pwr.initialized) return ESP_ERR_INVALID_STATE;
    s_pwr.config = config ? *config
                          : (sky_power_config_t)SKY_POWER_CONFIG_LIPO_DEFAULT();

    s_pwr.channel = gpio_to_channel(s_pwr.config.adc_pin);

    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_pwr.adc);
    if (err != ESP_OK) return err;

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = (adc_atten_t)s_pwr.config.adc_atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(s_pwr.adc, s_pwr.channel, &chan_cfg);

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .chan     = s_pwr.channel,
        .atten    = (adc_atten_t)s_pwr.config.adc_atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_pwr.cali) != ESP_OK)
        s_pwr.cali = NULL;
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .atten    = (adc_atten_t)s_pwr.config.adc_atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_cfg, &s_pwr.cali) != ESP_OK)
        s_pwr.cali = NULL;
#else
    s_pwr.cali = NULL;
#endif

    esp_timer_create_args_t timer_args = {
        .callback = sample_timer_cb,
        .name     = "pwr_sample",
    };
    esp_timer_create(&timer_args, &s_pwr.sample_timer);

    s_pwr.voltage = read_voltage();
    s_pwr.percent = voltage_to_percent(s_pwr.voltage);
    s_pwr.prev_voltage = s_pwr.voltage;
    s_pwr.initialized = true;

    ensure_registered();
    ESP_LOGI(TAG, "initialized: %.2fV %d%% (pin=%d)",
             s_pwr.voltage, s_pwr.percent, s_pwr.config.adc_pin);
    return ESP_OK;
}

esp_err_t sky_power_start(void)
{
    if (!s_pwr.initialized) return ESP_ERR_INVALID_STATE;
    if (s_pwr.started) return ESP_OK;

    if (s_pwr.config.sample_interval_ms > 0) {
        esp_timer_start_periodic(s_pwr.sample_timer,
                                 (uint64_t)s_pwr.config.sample_interval_ms * 1000);
    }
    s_pwr.started = true;
    return ESP_OK;
}

esp_err_t sky_power_get_status(sky_power_status_t *status)
{
    if (!status) return ESP_ERR_INVALID_ARG;
    status->voltage  = s_pwr.voltage;
    status->percent  = s_pwr.percent;
    status->charging = (s_pwr.voltage > s_pwr.prev_voltage + 0.02f);
    status->low      = s_pwr.low_active;
    status->critical = s_pwr.critical_active;
    return ESP_OK;
}

uint8_t sky_power_get_percent(void)  { return s_pwr.percent; }
float   sky_power_get_voltage(void)  { return s_pwr.voltage; }
bool    sky_power_is_low(void)       { return s_pwr.low_active; }
bool    sky_power_is_critical(void)  { return s_pwr.critical_active; }

esp_err_t sky_power_stop(void)
{
    if (!s_pwr.started) return ESP_OK;
    esp_timer_stop(s_pwr.sample_timer);
    s_pwr.started = false;
    return ESP_OK;
}

void sky_power_deinit(void)
{
    if (!s_pwr.initialized) return;
    sky_power_stop();
    esp_timer_delete(s_pwr.sample_timer);
    if (s_pwr.cali) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_delete_scheme_curve_fitting(s_pwr.cali);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_delete_scheme_line_fitting(s_pwr.cali);
#endif
    }
    adc_oneshot_del_unit(s_pwr.adc);
    s_pwr.initialized = false;
}
