#include "sky_core.h"
#include "esp_log.h"

static const char *TAG = "sky_core";
static bool s_initialized = false;

esp_err_t sky_core_init(void)
{
    sky_core_config_t config = SKY_CORE_DEFAULT_CONFIG();
    return sky_core_init_with_config(&config);
}

esp_err_t sky_core_init_with_config(const sky_core_config_t *config)
{
    if (s_initialized) return ESP_ERR_INVALID_STATE;

    esp_err_t err;

    err = sky_config_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "config init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = sky_error_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "error init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = sky_event_bus_init(&config->event_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "event bus init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = sky_registry_init(&config->registry);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "registry init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = sky_crash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "crash init failed: %s", esp_err_to_name(err));
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "initialized");
    return ESP_OK;
}

esp_err_t sky_core_start(void)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    esp_err_t err = sky_registry_start_all();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "service startup failed: %s", esp_err_to_name(err));
        return err;
    }

    sky_event_publish(SKY_EVENT_SYSTEM_READY, NULL, 0);
    ESP_LOGI(TAG, "all services started");
    return ESP_OK;
}

esp_err_t sky_core_stop(void)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    sky_event_publish(SKY_EVENT_SYSTEM_SHUTDOWN, NULL, 0);
    return sky_registry_stop_all();
}

void sky_core_deinit(void)
{
    if (!s_initialized) return;

    sky_registry_deinit();
    sky_event_bus_deinit();
    sky_config_deinit();
    s_initialized = false;
}
