#pragma once

#include "sky_stepper_types.h"
#include "sky_stepper_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sky_step_engine *sky_step_engine_handle_t;

esp_err_t sky_step_engine_init(const sky_step_engine_config_t *config,
                               sky_stepper_gpio_handle_t gpio,
                               sky_step_engine_handle_t *handle);

esp_err_t sky_step_engine_step(sky_step_engine_handle_t handle,
                               sky_direction_t direction);

int sky_step_engine_get_phase(sky_step_engine_handle_t handle);

esp_err_t sky_step_engine_set_mode(sky_step_engine_handle_t handle,
                                   sky_step_mode_t mode);

esp_err_t sky_step_engine_set_delay(sky_step_engine_handle_t handle,
                                    uint32_t step_delay_us);

uint32_t sky_step_engine_get_delay(sky_step_engine_handle_t handle);

uint16_t sky_step_engine_get_steps_per_rev(sky_step_engine_handle_t handle);

void sky_step_engine_deinit(sky_step_engine_handle_t handle);

#ifdef __cplusplus
}
#endif
