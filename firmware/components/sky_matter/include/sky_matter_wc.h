#pragma once

#include "esp_err.h"
#include "sky_matter_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Window Covering callbacks (called when Matter requests an action) ── */

typedef struct {
    esp_err_t (*on_move_to_percent)(uint8_t percent, void *ctx);
    esp_err_t (*on_open)(void *ctx);
    esp_err_t (*on_close)(void *ctx);
    esp_err_t (*on_stop)(void *ctx);
    void      *ctx;
} sky_matter_wc_callbacks_t;

typedef struct sky_matter_wc *sky_matter_wc_handle_t;

esp_err_t sky_matter_add_window_covering(
    const sky_matter_wc_callbacks_t *callbacks,
    sky_matter_wc_handle_t *handle);

esp_err_t sky_matter_wc_update_position(
    sky_matter_wc_handle_t handle,
    uint8_t current_percent);

esp_err_t sky_matter_wc_update_status(
    sky_matter_wc_handle_t handle,
    sky_matter_wc_status_t status);

#ifdef __cplusplus
}
#endif
