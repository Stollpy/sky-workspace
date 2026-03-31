#include "sky_matter_wc.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "sky_matter_wc";

struct sky_matter_wc {
    sky_matter_wc_callbacks_t callbacks;
    uint8_t                   current_percent;
    sky_matter_wc_status_t    status;
};

esp_err_t sky_matter_add_window_covering(
    const sky_matter_wc_callbacks_t *callbacks,
    sky_matter_wc_handle_t *handle)
{
    if (!callbacks || !handle) return ESP_ERR_INVALID_ARG;

    struct sky_matter_wc *wc = calloc(1, sizeof(*wc));
    if (!wc) return ESP_ERR_NO_MEM;
    wc->callbacks = *callbacks;
    *handle = wc;

    ESP_LOGI(TAG, "window covering endpoint registered (skeleton)");
    return ESP_OK;
}

esp_err_t sky_matter_wc_update_position(sky_matter_wc_handle_t handle,
                                        uint8_t current_percent)
{
    if (!handle) return ESP_ERR_INVALID_ARG;
    handle->current_percent = current_percent;
    return ESP_OK;
}

esp_err_t sky_matter_wc_update_status(sky_matter_wc_handle_t handle,
                                      sky_matter_wc_status_t status)
{
    if (!handle) return ESP_ERR_INVALID_ARG;
    handle->status = status;
    return ESP_OK;
}
