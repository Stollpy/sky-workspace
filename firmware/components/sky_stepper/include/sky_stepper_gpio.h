#pragma once

#include "sky_stepper_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sky_stepper_gpio *sky_stepper_gpio_handle_t;

esp_err_t sky_stepper_gpio_init(const sky_stepper_gpio_config_t *config,
                                sky_stepper_gpio_handle_t *handle);

esp_err_t sky_stepper_gpio_set_phase(sky_stepper_gpio_handle_t handle,
                                     const int levels[4]);

esp_err_t sky_stepper_gpio_all_off(sky_stepper_gpio_handle_t handle);

void sky_stepper_gpio_deinit(sky_stepper_gpio_handle_t handle);

#ifdef __cplusplus
}
#endif
