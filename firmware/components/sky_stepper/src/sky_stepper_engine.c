#include "sky_stepper_engine.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "sky_engine";

/* ── Stepping sequences ─────────────────────────────────────────────── */

static const int half_step[8][4] = {
    {1,0,0,0}, 
    {1,0,1,0}, 
    {0,0,1,0}, 
    {0,1,1,0},
    {0,1,0,0}, 
    {0,1,0,1}, 
    {0,0,0,1}, 
    {1,0,0,1},
};

static const int full_step[4][4] = {
    {1,0,1,0}, 
    {0,1,1,0}, 
    {0,1,0,1}, 
    {1,0,0,1},
};

static const int wave_step[4][4] = {
    {1,0,0,0}, 
    {0,0,1,0}, 
    {0,1,0,0}, 
    {0,0,0,1},
};

/* ── Internal structure ─────────────────────────────────────────────── */

struct sky_step_engine {
    sky_step_mode_t           mode;
    uint32_t                  step_delay_us;
    uint16_t                  steps_per_revolution;
    int                       phase_index;
    sky_stepper_gpio_handle_t gpio;
};

/* ── Helpers ────────────────────────────────────────────────────────── */

static int phase_count(sky_step_mode_t mode)
{
    return mode == SKY_STEP_MODE_HALF ? 8 : 4;
}

static const int (*get_sequence(sky_step_mode_t mode))[4]
{
    switch (mode) {
    case SKY_STEP_MODE_HALF: return half_step;
    case SKY_STEP_MODE_FULL: return full_step;
    default:                 return wave_step;
    }
}

/* ── Public API ─────────────────────────────────────────────────────── */

esp_err_t sky_step_engine_init(const sky_step_engine_config_t *config,
                               sky_stepper_gpio_handle_t gpio,
                               sky_step_engine_handle_t *handle)
{
    if (!config || !gpio || !handle) return ESP_ERR_INVALID_ARG;

    struct sky_step_engine *e = calloc(1, sizeof(*e));
    if (!e) return ESP_ERR_NO_MEM;

    e->mode                = config->mode;
    e->step_delay_us       = config->step_delay_us;
    e->steps_per_revolution = config->steps_per_revolution;
    e->phase_index         = 0;
    e->gpio                = gpio;

    *handle = e;
    ESP_LOGI(TAG, "init mode=%d delay=%luus spr=%d",
             e->mode, (unsigned long)e->step_delay_us, e->steps_per_revolution);
    return ESP_OK;
}

esp_err_t sky_step_engine_step(sky_step_engine_handle_t handle,
                               sky_direction_t direction)
{
    if (!handle) return ESP_ERR_INVALID_ARG;

    int count = phase_count(handle->mode);
    const int (*seq)[4] = get_sequence(handle->mode);

    if (direction == SKY_DIR_CW) {
        handle->phase_index = (handle->phase_index + 1) % count;
    } else {
        handle->phase_index = (handle->phase_index - 1 + count) % count;
    }

    return sky_stepper_gpio_set_phase(handle->gpio, seq[handle->phase_index]);
}

int sky_step_engine_get_phase(sky_step_engine_handle_t handle)
{
    return handle ? handle->phase_index : -1;
}

esp_err_t sky_step_engine_set_mode(sky_step_engine_handle_t handle,
                                   sky_step_mode_t mode)
{
    if (!handle) return ESP_ERR_INVALID_ARG;
    handle->mode = mode;
    handle->phase_index = handle->phase_index % phase_count(mode);
    return ESP_OK;
}

esp_err_t sky_step_engine_set_delay(sky_step_engine_handle_t handle,
                                    uint32_t step_delay_us)
{
    if (!handle || step_delay_us == 0) return ESP_ERR_INVALID_ARG;
    handle->step_delay_us = step_delay_us;
    return ESP_OK;
}

uint32_t sky_step_engine_get_delay(sky_step_engine_handle_t handle)
{
    return handle ? handle->step_delay_us : 0;
}

uint16_t sky_step_engine_get_steps_per_rev(sky_step_engine_handle_t handle)
{
    return handle ? handle->steps_per_revolution : 0;
}

void sky_step_engine_deinit(sky_step_engine_handle_t handle)
{
    if (!handle) return;
    ESP_LOGI(TAG, "deinit");
    free(handle);
}
