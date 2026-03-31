#pragma once

#include "esp_err.h"
#include "sky_matter_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Configuration ──────────────────────────────────────────────────── */

typedef struct {
    const char            *device_name;
    uint16_t               vendor_id;
    uint16_t               product_id;
    const char            *serial_number;
    const char            *firmware_version;
    uint32_t               setup_passcode;
    uint16_t               discriminator;
    sky_matter_transport_t transport;
} sky_matter_config_t;

#define SKY_MATTER_CONFIG_DEV_DEFAULT() {     \
    .device_name      = "Sky Device",         \
    .vendor_id        = 0xFFF1,               \
    .product_id       = 0x0001,               \
    .serial_number    = "SKY-000001",         \
    .firmware_version = "0.1.0",              \
    .setup_passcode   = 20202021,             \
    .discriminator    = 0xF00,                \
    .transport        = SKY_MATTER_TRANSPORT_AUTO, \
}

/* ── Core API ───────────────────────────────────────────────────────── */

esp_err_t sky_matter_init(const sky_matter_config_t *config);
esp_err_t sky_matter_start(void);
esp_err_t sky_matter_stop(void);
bool      sky_matter_is_commissioned(void);
esp_err_t sky_matter_get_qr_code(char *buf, size_t buf_size);
esp_err_t sky_matter_get_manual_code(char *buf, size_t buf_size);
esp_err_t sky_matter_factory_reset(void);
void      sky_matter_deinit(void);

#ifdef __cplusplus
}
#endif
