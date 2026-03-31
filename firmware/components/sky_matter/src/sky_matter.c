#include "sky_matter.h"
#include "sky_registry.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "sky_matter";

#ifdef CONFIG_SKY_MATTER_ENABLED
/* ── Full implementation when esp-matter SDK is integrated ──────────── */
/* TODO: integrate esp-matter SDK (connectedhomeip)                     */
/*   - Create Matter node with endpoints                                */
/*   - Handle commissioning (BLE PASE, CASE)                            */
/*   - Map attribute reads/writes to callbacks                          */
/*   - Support ESP32 (Wi-Fi), ESP32-C6 (Wi-Fi/Thread), ESP32-H2 (Thread) */
#error "CONFIG_SKY_MATTER_ENABLED requires the esp-matter SDK. See docs."
#endif

/* ── Skeleton — compiles without esp-matter SDK ─────────────────────── */

static sky_matter_config_t s_config;
static bool s_initialized = false;

static esp_err_t svc_init(void *cfg) { (void)cfg; return ESP_OK; }
static esp_err_t svc_start(void)
{
    ESP_LOGW(TAG, "Matter disabled — enable CONFIG_SKY_MATTER_ENABLED "
                  "and integrate esp-matter SDK");
    return ESP_OK;
}
static esp_err_t svc_stop(void)  { return ESP_OK; }
static esp_err_t svc_deinit(void){ return ESP_OK; }

esp_err_t sky_matter_init(const sky_matter_config_t *config)
{
    if (s_initialized) return ESP_ERR_INVALID_STATE;
    s_config = config ? *config
                      : (sky_matter_config_t)SKY_MATTER_CONFIG_DEV_DEFAULT();

    static const sky_service_lifecycle_t lc = {
        .init = svc_init, .start = svc_start,
        .stop = svc_stop, .deinit = svc_deinit,
    };
    sky_registry_register("sky_matter", &lc, NULL);

    s_initialized = true;
    ESP_LOGI(TAG, "initialized (skeleton — Matter SDK not linked)");
    return ESP_OK;
}

esp_err_t sky_matter_start(void)   { return ESP_OK; }
esp_err_t sky_matter_stop(void)    { return ESP_OK; }
bool sky_matter_is_commissioned(void) { return false; }

esp_err_t sky_matter_get_qr_code(char *buf, size_t buf_size)
{
    if (!buf) return ESP_ERR_INVALID_ARG;
    strncpy(buf, "MT:NOT-AVAILABLE", buf_size - 1);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sky_matter_get_manual_code(char *buf, size_t buf_size)
{
    if (!buf) return ESP_ERR_INVALID_ARG;
    strncpy(buf, "N/A", buf_size - 1);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sky_matter_factory_reset(void)
{
    ESP_LOGW(TAG, "factory_reset: Matter not integrated");
    return ESP_ERR_NOT_SUPPORTED;
}

void sky_matter_deinit(void)
{
    s_initialized = false;
}
