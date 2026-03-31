#include "sky_registry.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "sky_registry";

typedef struct {
    sky_service_id_t         id;
    sky_service_lifecycle_t  lifecycle;
    void                    *config;
    void                    *instance;
    bool                     initialized;
    bool                     running;
    bool                     used;
} registry_entry_t;

static registry_entry_t   *s_entries     = NULL;
static uint8_t             s_max         = 0;
static uint8_t             s_count       = 0;
static SemaphoreHandle_t   s_mutex       = NULL;
static bool                s_initialized = false;

static registry_entry_t *find_entry(sky_service_id_t id)
{
    for (uint8_t i = 0; i < s_max; i++) {
        if (s_entries[i].used && strcmp(s_entries[i].id, id) == 0) {
            return &s_entries[i];
        }
    }
    return NULL;
}

esp_err_t sky_registry_init(const sky_registry_config_t *config)
{
    if (s_initialized) return ESP_ERR_INVALID_STATE;

    s_max = config ? config->max_services : 16;
    s_entries = calloc(s_max, sizeof(registry_entry_t));
    if (!s_entries) return ESP_ERR_NO_MEM;

    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) {
        free(s_entries);
        s_entries = NULL;
        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "initialized (max_services=%u)", s_max);
    return ESP_OK;
}

esp_err_t sky_registry_register(sky_service_id_t id,
                                const sky_service_lifecycle_t *lifecycle,
                                void *config)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    if (!id || !lifecycle) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    if (find_entry(id)) {
        xSemaphoreGive(s_mutex);
        ESP_LOGW(TAG, "'%s' already registered", id);
        return ESP_ERR_INVALID_STATE;
    }

    for (uint8_t i = 0; i < s_max; i++) {
        if (!s_entries[i].used) {
            s_entries[i].id          = id;
            s_entries[i].lifecycle   = *lifecycle;
            s_entries[i].config      = config;
            s_entries[i].instance    = NULL;
            s_entries[i].initialized = false;
            s_entries[i].running     = false;
            s_entries[i].used        = true;
            s_count++;
            xSemaphoreGive(s_mutex);
            ESP_LOGI(TAG, "registered '%s'", id);
            return ESP_OK;
        }
    }

    xSemaphoreGive(s_mutex);
    ESP_LOGE(TAG, "registry full (max=%u)", s_max);
    return ESP_ERR_NO_MEM;
}

esp_err_t sky_registry_set_instance(sky_service_id_t id, void *instance)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    registry_entry_t *entry = find_entry(id);
    if (!entry) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    entry->instance = instance;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

void *sky_registry_get(sky_service_id_t id)
{
    if (!s_initialized || !id) return NULL;

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    registry_entry_t *entry = find_entry(id);
    void *result = NULL;
    if (entry) {
        result = entry->instance ? entry->instance : entry->config;
    }
    xSemaphoreGive(s_mutex);
    return result;
}

esp_err_t sky_registry_start_all(void)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    for (uint8_t i = 0; i < s_max; i++) {
        if (s_entries[i].used && !s_entries[i].initialized) {
            if (s_entries[i].lifecycle.init) {
                ESP_LOGI(TAG, "init '%s'", s_entries[i].id);
                esp_err_t err = s_entries[i].lifecycle.init(s_entries[i].config);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "'%s' init failed: %s",
                             s_entries[i].id, esp_err_to_name(err));
                    xSemaphoreGive(s_mutex);
                    return err;
                }
            }
            s_entries[i].initialized = true;
        }
    }

    for (uint8_t i = 0; i < s_max; i++) {
        if (s_entries[i].used && s_entries[i].initialized &&
            !s_entries[i].running) {
            if (s_entries[i].lifecycle.start) {
                ESP_LOGI(TAG, "start '%s'", s_entries[i].id);
                esp_err_t err = s_entries[i].lifecycle.start();
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "'%s' start failed: %s",
                             s_entries[i].id, esp_err_to_name(err));
                    xSemaphoreGive(s_mutex);
                    return err;
                }
            }
            s_entries[i].running = true;
        }
    }

    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "all services started (%u total)", s_count);
    return ESP_OK;
}

esp_err_t sky_registry_stop_all(void)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    for (int i = s_max - 1; i >= 0; i--) {
        if (s_entries[i].used && s_entries[i].running) {
            if (s_entries[i].lifecycle.stop) {
                ESP_LOGI(TAG, "stop '%s'", s_entries[i].id);
                s_entries[i].lifecycle.stop();
            }
            s_entries[i].running = false;
        }
    }

    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

void sky_registry_deinit(void)
{
    if (!s_initialized) return;

    sky_registry_stop_all();

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (int i = s_max - 1; i >= 0; i--) {
        if (s_entries[i].used && s_entries[i].initialized) {
            if (s_entries[i].lifecycle.deinit) {
                s_entries[i].lifecycle.deinit();
            }
            s_entries[i].initialized = false;
            s_entries[i].used = false;
        }
    }
    s_count = 0;
    xSemaphoreGive(s_mutex);

    vSemaphoreDelete(s_mutex);
    s_mutex = NULL;
    free(s_entries);
    s_entries = NULL;
    s_initialized = false;
}
