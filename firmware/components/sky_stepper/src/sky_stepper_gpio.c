#include "sky_stepper_gpio.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "sky_gpio";

struct sky_stepper_gpio {
    gpio_num_t pins[4];
    bool       invert;
};

esp_err_t sky_stepper_gpio_init(const sky_stepper_gpio_config_t *config,
                                sky_stepper_gpio_handle_t *handle)
{
    if (!config || !handle) return ESP_ERR_INVALID_ARG;

    struct sky_stepper_gpio *g = calloc(1, sizeof(*g));
    if (!g) return ESP_ERR_NO_MEM;

    memcpy(g->pins, config->pins, sizeof(g->pins));
    g->invert = config->invert_logic;

    for (int i = 0; i < 4; i++) {
        gpio_config_t io = {
            .pin_bit_mask  = 1ULL << config->pins[i],
            .mode          = GPIO_MODE_OUTPUT,
            .pull_up_en    = GPIO_PULLUP_DISABLE,
            .pull_down_en  = GPIO_PULLDOWN_DISABLE,
            .intr_type     = GPIO_INTR_DISABLE,
        };
        esp_err_t err = gpio_config(&io);
        if (err != ESP_OK) {
            free(g);
            return err;
        }
        gpio_set_level(config->pins[i], g->invert ? 1 : 0);
    }

    *handle = g;
    ESP_LOGI(TAG, "init pins [%d,%d,%d,%d] invert=%d",
             config->pins[0], config->pins[1],
             config->pins[2], config->pins[3],
             config->invert_logic);
    return ESP_OK;
}

esp_err_t sky_stepper_gpio_set_phase(sky_stepper_gpio_handle_t handle,
                                     const int levels[4])
{
    if (!handle || !levels) return ESP_ERR_INVALID_ARG;
    for (int i = 0; i < 4; i++) {
        gpio_set_level(handle->pins[i], handle->invert ? !levels[i] : levels[i]);
    }
    return ESP_OK;
}

esp_err_t sky_stepper_gpio_all_off(sky_stepper_gpio_handle_t handle)
{
    if (!handle) return ESP_ERR_INVALID_ARG;
    const int off[4] = {0, 0, 0, 0};
    return sky_stepper_gpio_set_phase(handle, off);
}

void sky_stepper_gpio_deinit(sky_stepper_gpio_handle_t handle)
{
    if (!handle) return;
    sky_stepper_gpio_all_off(handle);
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(handle->pins[i]);
    }
    free(handle);
}
