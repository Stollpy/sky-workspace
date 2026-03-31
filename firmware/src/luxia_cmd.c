#include "luxia_cmd.h"
#include "sky_stepper.h"
#include "sky_power.h"
#include "sky_wifi.h"
#include "sky_matter.h"
#include "sky_ota.h"
#include "sky_core.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "luxia_cmd";

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.1.0"
#endif

static sky_stepper_handle_t s_motor;

/* ═══════════════════════════════════════════════════════════════════════
 *  Init
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t luxia_cmd_init(sky_stepper_handle_t motor)
{
    if (!motor) return ESP_ERR_INVALID_ARG;
    s_motor = motor;
    ESP_LOGI(TAG, "command module ready");
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Calibration
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t luxia_cmd_calibrate_start(sky_direction_t dir)
{
    return sky_stepper_calibrate_start(s_motor, dir);
}

esp_err_t luxia_cmd_calibrate_mark_open(int32_t *out_position)
{
    esp_err_t err = sky_stepper_calibrate_mark_open(s_motor);
    if (err == ESP_OK && out_position) {
        *out_position = sky_stepper_get_position(s_motor);
    }
    return err;
}

esp_err_t luxia_cmd_calibrate_mark_closed(int32_t *out_position,
                                          int32_t *out_total)
{
    esp_err_t err = sky_stepper_calibrate_mark_closed(s_motor);
    if (err != ESP_OK) return err;

    for (int i = 0; i < 50; i++) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (sky_stepper_is_calibrated(s_motor)) break;
    }

    sky_stepper_calibration_t cal;
    err = sky_stepper_get_calibration(s_motor, &cal);
    if (err == ESP_OK) {
        if (out_position) *out_position = cal.closed_position;
        if (out_total)    *out_total    = cal.total_steps;
    }
    return err;
}

esp_err_t luxia_cmd_calibrate_cancel(void)
{
    return sky_stepper_calibrate_cancel(s_motor);
}

esp_err_t luxia_cmd_calibrate_status(bool *calibrated, int32_t *total)
{
    if (calibrated) *calibrated = sky_stepper_is_calibrated(s_motor);
    if (total) {
        sky_stepper_calibration_t cal;
        if (sky_stepper_get_calibration(s_motor, &cal) == ESP_OK) {
            *total = cal.total_steps;
        } else {
            *total = 0;
        }
    }
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Presets
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t luxia_cmd_preset_save(const char *name, uint8_t percent)
{
    return sky_stepper_preset_save(s_motor, name, percent);
}

esp_err_t luxia_cmd_preset_apply(const char *name)
{
    return sky_stepper_preset_apply(s_motor, name);
}

esp_err_t luxia_cmd_preset_delete(const char *name)
{
    return sky_stepper_preset_delete(s_motor, name);
}

esp_err_t luxia_cmd_preset_list(sky_pattern_t *out, uint8_t *count,
                                uint8_t max)
{
    sky_pattern_t all[CONFIG_SKY_STEPPER_MAX_PATTERNS];
    uint8_t total = 0;
    esp_err_t err = sky_stepper_pattern_list(s_motor, all, &total,
                                             CONFIG_SKY_STEPPER_MAX_PATTERNS);
    if (err != ESP_OK) return err;

    uint8_t n = 0;
    for (uint8_t i = 0; i < total && n < max; i++) {
        if (all[i].type == SKY_PATTERN_PRESET) {
            out[n++] = all[i];
        }
    }
    *count = n;
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Sequences
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t luxia_cmd_sequence_record_start(const char *name)
{
    return sky_stepper_sequence_record_start(s_motor, name);
}

esp_err_t luxia_cmd_sequence_record_step(uint8_t percent,
                                         uint32_t delay_ms)
{
    return sky_stepper_sequence_record_step(s_motor, percent, delay_ms);
}

esp_err_t luxia_cmd_sequence_record_stop(void)
{
    return sky_stepper_sequence_record_stop(s_motor);
}

esp_err_t luxia_cmd_sequence_play(const char *name)
{
    return sky_stepper_sequence_play(s_motor, name);
}

esp_err_t luxia_cmd_sequence_stop(void)
{
    return sky_stepper_sequence_stop(s_motor);
}

esp_err_t luxia_cmd_sequence_list(sky_pattern_t *out, uint8_t *count,
                                  uint8_t max)
{
    sky_pattern_t all[CONFIG_SKY_STEPPER_MAX_PATTERNS];
    uint8_t total = 0;
    esp_err_t err = sky_stepper_pattern_list(s_motor, all, &total,
                                             CONFIG_SKY_STEPPER_MAX_PATTERNS);
    if (err != ESP_OK) return err;

    uint8_t n = 0;
    for (uint8_t i = 0; i < total && n < max; i++) {
        if (all[i].type == SKY_PATTERN_SEQUENCE) {
            out[n++] = all[i];
        }
    }
    *count = n;
    return ESP_OK;
}

esp_err_t luxia_cmd_sequence_delete(const char *name)
{
    return sky_stepper_preset_delete(s_motor, name);
}

esp_err_t luxia_cmd_sequence_export(const char *name, sky_pattern_t *out)
{
    return sky_stepper_pattern_export(s_motor, name, out);
}

esp_err_t luxia_cmd_sequence_import(const sky_pattern_t *pattern)
{
    return sky_stepper_pattern_import(s_motor, pattern);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Diagnostics
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t luxia_cmd_get_status(luxia_status_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));

    /* Motor */
    sky_stepper_status_t mst;
    if (sky_stepper_get_status(s_motor, &mst) == ESP_OK) {
        out->motor.state       = mst.state;
        out->motor.position    = mst.current_position;
        out->motor.calibrated  = mst.calibrated;
        out->motor.total_steps = mst.max_position - mst.min_position;
    }
    out->motor.percent = sky_stepper_get_height_percent(s_motor);

    /* Battery */
    sky_power_status_t pst;
    if (sky_power_get_status(&pst) == ESP_OK) {
        out->battery.voltage  = pst.voltage;
        out->battery.percent  = pst.percent;
        out->battery.charging = pst.charging;
        out->battery.low      = pst.low;
        out->battery.critical = pst.critical;
    }

    /* Wi-Fi */
    sky_wifi_status_t wst;
    if (sky_wifi_get_status(&wst) == ESP_OK) {
        out->wifi.connected = wst.connected;
        out->wifi.rssi      = wst.rssi;
        memcpy(out->wifi.ip, wst.ip, sizeof(out->wifi.ip));
    }

    /* Matter */
    out->matter.commissioned = sky_matter_is_commissioned();

    /* System */
    out->heap_free = esp_get_free_heap_size();

    return ESP_OK;
}

esp_err_t luxia_cmd_reboot(void)
{
    ESP_LOGW(TAG, "reboot requested");
    sky_core_stop();
    esp_restart();
    return ESP_OK;
}

esp_err_t luxia_cmd_factory_reset(void)
{
    ESP_LOGW(TAG, "factory reset requested");

    sky_stepper_clear_calibration(s_motor);
    sky_wifi_reset_credentials();
    sky_matter_factory_reset();

    ESP_LOGI(TAG, "factory reset complete — rebooting");
    sky_core_stop();
    esp_restart();
    return ESP_OK;
}

esp_err_t luxia_cmd_get_firmware_version(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return ESP_ERR_INVALID_ARG;
    snprintf(buf, buf_size, "%s", FIRMWARE_VERSION);
    return ESP_OK;
}

esp_err_t luxia_cmd_ota_check(bool *available)
{
    return sky_ota_check(available);
}

esp_err_t luxia_cmd_ota_update(void)
{
    return sky_ota_update();
}

esp_err_t luxia_cmd_set_motor_name(const char *name)
{
    return sky_stepper_set_name(s_motor, name);
}
