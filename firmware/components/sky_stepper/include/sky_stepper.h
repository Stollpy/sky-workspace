#pragma once

#include "sky_stepper_types.h"
#include "sky_stepper_gpio.h"
#include "sky_stepper_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Registration ───────────────────────────────────────────────────── */

esp_err_t sky_stepper_register(const sky_stepper_config_t *config,
                               sky_stepper_handle_t *handle);

sky_stepper_handle_t sky_stepper_get(const char *name);

esp_err_t sky_stepper_list(sky_stepper_handle_t *handles,
                           uint8_t *count, uint8_t max);

esp_err_t sky_stepper_unregister(sky_stepper_handle_t handle);

esp_err_t sky_stepper_set_name(sky_stepper_handle_t handle, const char *name);
const char *sky_stepper_get_name(sky_stepper_handle_t handle);

/* ── Movement ───────────────────────────────────────────────────────── */

esp_err_t sky_stepper_move(sky_stepper_handle_t handle,
                           uint32_t steps, sky_direction_t direction);

esp_err_t sky_stepper_move_to(sky_stepper_handle_t handle,
                              int32_t target_position);

esp_err_t sky_stepper_set_height(sky_stepper_handle_t handle,
                                 uint8_t percent);

esp_err_t sky_stepper_stop(sky_stepper_handle_t handle);

esp_err_t sky_stepper_emergency_stop(sky_stepper_handle_t handle);

/* ── Status ─────────────────────────────────────────────────────────── */

sky_stepper_state_t sky_stepper_get_state(sky_stepper_handle_t handle);
int32_t             sky_stepper_get_position(sky_stepper_handle_t handle);
int8_t              sky_stepper_get_height_percent(sky_stepper_handle_t handle);
esp_err_t           sky_stepper_get_status(sky_stepper_handle_t handle,
                                           sky_stepper_status_t *status);

/* ── Ramp ───────────────────────────────────────────────────────────── */

esp_err_t sky_stepper_set_ramp(sky_stepper_handle_t handle,
                               const sky_stepper_ramp_config_t *ramp);

/* ── Hooks ──────────────────────────────────────────────────────────── */

esp_err_t sky_stepper_add_hook(sky_stepper_handle_t handle,
                               sky_stepper_hook_type_t type,
                               sky_stepper_hook_t hook, void *ctx);

esp_err_t sky_stepper_remove_hook(sky_stepper_handle_t handle,
                                  sky_stepper_hook_type_t type,
                                  sky_stepper_hook_t hook);

/* ── Calibration ────────────────────────────────────────────────────── */

esp_err_t sky_stepper_calibrate_start(sky_stepper_handle_t handle,
                                      sky_direction_t direction);
esp_err_t sky_stepper_calibrate_mark_open(sky_stepper_handle_t handle);
esp_err_t sky_stepper_calibrate_mark_closed(sky_stepper_handle_t handle);
esp_err_t sky_stepper_calibrate_cancel(sky_stepper_handle_t handle);
bool      sky_stepper_is_calibrated(sky_stepper_handle_t handle);
esp_err_t sky_stepper_get_calibration(sky_stepper_handle_t handle,
                                      sky_stepper_calibration_t *cal);
esp_err_t sky_stepper_clear_calibration(sky_stepper_handle_t handle);

/* ── Patterns — presets ─────────────────────────────────────────────── */

esp_err_t sky_stepper_preset_save(sky_stepper_handle_t handle,
                                  const char *name, uint8_t percent);
esp_err_t sky_stepper_preset_apply(sky_stepper_handle_t handle,
                                   const char *name);
esp_err_t sky_stepper_preset_delete(sky_stepper_handle_t handle,
                                    const char *name);

/* ── Patterns — sequences ───────────────────────────────────────────── */

esp_err_t sky_stepper_sequence_record_start(sky_stepper_handle_t handle,
                                            const char *name);
esp_err_t sky_stepper_sequence_record_step(sky_stepper_handle_t handle,
                                           uint8_t target_percent,
                                           uint32_t delay_after_ms);
esp_err_t sky_stepper_sequence_record_stop(sky_stepper_handle_t handle);
esp_err_t sky_stepper_sequence_play(sky_stepper_handle_t handle,
                                    const char *name);
esp_err_t sky_stepper_sequence_stop(sky_stepper_handle_t handle);

/* ── Patterns — management ──────────────────────────────────────────── */

esp_err_t sky_stepper_pattern_list(sky_stepper_handle_t handle,
                                   sky_pattern_t *patterns,
                                   uint8_t *count, uint8_t max);
esp_err_t sky_stepper_pattern_export(sky_stepper_handle_t handle,
                                     const char *name, sky_pattern_t *out);
esp_err_t sky_stepper_pattern_import(sky_stepper_handle_t handle,
                                     const sky_pattern_t *pattern);

#ifdef __cplusplus
}
#endif
