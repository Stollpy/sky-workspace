#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Provisioning mode ──────────────────────────────────────────────── */

typedef enum {
    SKY_WIFI_PROV_NONE,
    SKY_WIFI_PROV_BLE,
    SKY_WIFI_PROV_SOFTAP,
} sky_wifi_prov_mode_t;

/* ── Configuration ──────────────────────────────────────────────────── */

typedef struct {
    const char          *ssid;
    const char          *password;
    sky_wifi_prov_mode_t prov_mode;
    const char          *hostname;
    uint8_t              max_retry;
    uint32_t             retry_interval_ms;
    bool                 auto_reconnect;
} sky_wifi_config_t;

#define SKY_WIFI_CONFIG_DEFAULT() {            \
    .ssid               = NULL,                \
    .password           = NULL,                \
    .prov_mode          = SKY_WIFI_PROV_NONE,  \
    .hostname           = "sky-device",        \
    .max_retry          = 0,                   \
    .retry_interval_ms  = 5000,                \
    .auto_reconnect     = true,                \
}

/* ── Status ─────────────────────────────────────────────────────────── */

typedef struct {
    bool     connected;
    int8_t   rssi;
    char     ssid[33];
    char     ip[16];
    uint32_t uptime_ms;
} sky_wifi_status_t;

/* ── Event data (published on the sky event bus) ────────────────────── */

typedef struct {
    char   ssid[33];
    char   ip[16];
    int8_t rssi;
} sky_wifi_connected_event_t;

typedef struct {
    int reason;
} sky_wifi_disconnected_event_t;

typedef struct {
    uint8_t attempt;
    uint8_t max_retry;
} sky_wifi_reconnecting_event_t;

/* ── API ────────────────────────────────────────────────────────────── */

esp_err_t sky_wifi_init(const sky_wifi_config_t *config);
esp_err_t sky_wifi_start(void);
esp_err_t sky_wifi_stop(void);
bool      sky_wifi_is_connected(void);
esp_err_t sky_wifi_get_status(sky_wifi_status_t *status);
esp_err_t sky_wifi_reconnect(void);
esp_err_t sky_wifi_reset_credentials(void);
void      sky_wifi_deinit(void);

#ifdef __cplusplus
}
#endif
