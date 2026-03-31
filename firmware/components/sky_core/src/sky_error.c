#include "sky_error.h"
#include "sky_crash.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "sky_error";

static sky_error_handler_t   s_handler       = NULL;
static sky_error_severity_t  s_min_severity  = SKY_SEVERITY_ERROR;
static void                 *s_handler_ctx   = NULL;
static bool                  s_initialized   = false;

esp_err_t sky_error_init(void)
{
    if (s_initialized) return ESP_ERR_INVALID_STATE;
    s_initialized = true;
    ESP_LOGI(TAG, "initialized");
    return ESP_OK;
}

esp_err_t sky_error_set_handler(sky_error_severity_t min_severity,
                                sky_error_handler_t handler,
                                void *ctx)
{
    s_handler      = handler;
    s_min_severity = min_severity;
    s_handler_ctx  = ctx;
    return ESP_OK;
}

void sky_error_report(esp_err_t code,
                      sky_error_severity_t severity,
                      const char *component,
                      const char *message)
{
    switch (severity) {
    case SKY_SEVERITY_INFO:
        ESP_LOGI(TAG, "[%s] %s", component, message);
        break;
    case SKY_SEVERITY_WARNING:
        ESP_LOGW(TAG, "[%s] %s", component, message);
        break;
    default:
        ESP_LOGE(TAG, "[%s] %s (0x%x)", component, message, (unsigned)code);
        break;
    }

    if (s_handler && severity >= s_min_severity) {
        sky_error_t error = {
            .code         = code,
            .severity     = severity,
            .component    = component,
            .message      = message,
            .timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000),
        };
        s_handler(&error, s_handler_ctx);
    }

    if (severity >= SKY_SEVERITY_FATAL) {
        sky_crash_trigger(component, severity);
    }
}
