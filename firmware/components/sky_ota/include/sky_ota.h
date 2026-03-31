#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Self-test callback ─────────────────────────────────────────────── */

typedef void (*sky_ota_self_test_t)(bool *passed, void *ctx);

/* ── Configuration ──────────────────────────────────────────────────── */

typedef struct {
    const char         *update_url;
    const char         *server_cert_pem;
    uint32_t            check_interval_ms;
    bool                auto_update;
    bool                matter_ota_enabled;
    sky_ota_self_test_t self_test;
    void               *self_test_ctx;
    uint32_t            self_test_timeout_ms;
    bool                skip_server_cert;
} sky_ota_config_t;

#define SKY_OTA_CONFIG_DEFAULT() {         \
    .update_url           = NULL,          \
    .server_cert_pem      = NULL,          \
    .check_interval_ms    = 0,             \
    .auto_update          = false,         \
    .matter_ota_enabled   = false,         \
    .self_test            = NULL,          \
    .self_test_ctx        = NULL,          \
    .self_test_timeout_ms = 30000,         \
    .skip_server_cert     = true,          \
}

/* ── Status ─────────────────────────────────────────────────────────── */

typedef struct {
    char    current_version[32];
    char    available_version[32];
    bool    update_available;
    bool    update_in_progress;
    uint8_t download_progress;
} sky_ota_status_t;

/* ── Event data ─────────────────────────────────────────────────────── */

typedef struct {
    uint8_t percent;
} sky_ota_progress_event_t;

typedef struct {
    char version[32];
    bool success;
} sky_ota_complete_event_t;

/* ── API ────────────────────────────────────────────────────────────── */

esp_err_t sky_ota_init(const sky_ota_config_t *config);
esp_err_t sky_ota_start(void);
esp_err_t sky_ota_check(bool *update_available);
esp_err_t sky_ota_update(void);
esp_err_t sky_ota_get_status(sky_ota_status_t *status);
void      sky_ota_deinit(void);

#ifdef __cplusplus
}
#endif
