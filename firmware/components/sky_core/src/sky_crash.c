#include "sky_crash.h"
#include "esp_log.h"
#include "esp_system.h"
#include <string.h>

static const char *TAG = "sky_crash";

#define CRASH_MAX_ENTRIES 16

typedef struct {
    sky_service_id_t   service_id;
    sky_crash_config_t config;
    bool               used;
} crash_entry_t;

static crash_entry_t s_entries[CRASH_MAX_ENTRIES];
static uint8_t       s_count       = 0;
static bool          s_initialized = false;

static void shutdown_handler(void)
{
    for (int i = (int)s_count - 1; i >= 0; i--) {
        if (s_entries[i].used &&
            s_entries[i].config.safe_state_handler) {
            s_entries[i].config.safe_state_handler(
                s_entries[i].config.ctx);
        }
    }
}

esp_err_t sky_crash_init(void)
{
    if (s_initialized) return ESP_ERR_INVALID_STATE;

    memset(s_entries, 0, sizeof(s_entries));
    s_count = 0;

    esp_err_t err = esp_register_shutdown_handler(shutdown_handler);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "initialized (max=%d)", CRASH_MAX_ENTRIES);
    return ESP_OK;
}

esp_err_t sky_crash_register(sky_service_id_t service_id,
                             const sky_crash_config_t *config)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    if (!service_id || !config) return ESP_ERR_INVALID_ARG;
    if (s_count >= CRASH_MAX_ENTRIES) return ESP_ERR_NO_MEM;

    crash_entry_t *entry = &s_entries[s_count];
    entry->service_id = service_id;
    entry->config     = *config;
    entry->used       = true;
    s_count++;

    ESP_LOGI(TAG, "registered '%s'", service_id);
    return ESP_OK;
}

esp_err_t sky_crash_trigger(sky_service_id_t service_id,
                            sky_error_severity_t severity)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    for (uint8_t i = 0; i < s_count; i++) {
        if (!s_entries[i].used ||
            strcmp(s_entries[i].service_id, service_id) != 0) {
            continue;
        }

        sky_crash_action_t action;
        if (severity >= SKY_SEVERITY_FATAL) {
            action = s_entries[i].config.on_system_panic;
        } else {
            action = s_entries[i].config.on_component_error;
        }

        switch (action) {
        case SKY_CRASH_ACTION_NONE:
            break;
        case SKY_CRASH_ACTION_LOG:
            ESP_LOGW(TAG, "'%s' crash action: log only", service_id);
            break;
        case SKY_CRASH_ACTION_SAFE_STATE:
            ESP_LOGW(TAG, "'%s' entering safe state", service_id);
            if (s_entries[i].config.safe_state_handler) {
                s_entries[i].config.safe_state_handler(
                    s_entries[i].config.ctx);
            }
            break;
        case SKY_CRASH_ACTION_STOP:
            ESP_LOGW(TAG, "'%s' crash action: stop", service_id);
            if (s_entries[i].config.safe_state_handler) {
                s_entries[i].config.safe_state_handler(
                    s_entries[i].config.ctx);
            }
            break;
        case SKY_CRASH_ACTION_REBOOT:
            ESP_LOGE(TAG, "'%s' crash action: reboot", service_id);
            sky_crash_enter_safe_state_all();
            esp_restart();
            break;
        case SKY_CRASH_ACTION_RESTART:
            ESP_LOGW(TAG, "'%s' crash action: restart", service_id);
            if (s_entries[i].config.safe_state_handler) {
                s_entries[i].config.safe_state_handler(
                    s_entries[i].config.ctx);
            }
            break;
        }
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t sky_crash_enter_safe_state_all(void)
{
    ESP_LOGW(TAG, "entering safe state for all components");
    for (int i = (int)s_count - 1; i >= 0; i--) {
        if (s_entries[i].used &&
            s_entries[i].config.safe_state_handler) {
            s_entries[i].config.safe_state_handler(
                s_entries[i].config.ctx);
        }
    }
    return ESP_OK;
}
