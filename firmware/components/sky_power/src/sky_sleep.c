#include "sky_sleep.h"
#include "sky_core.h"
#include "sky_event.h"
#include "sky_events.h"
#include "sky_registry.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "sky_sleep";

#define MAX_WAKELOCKS 8
#define WAKELOCK_REASON_LEN 16

/* ── RTC-persistent data (survives deep sleep) ──────────────────────── */

RTC_DATA_ATTR static uint32_t s_wake_count = 0;

/* ── Module state ───────────────────────────────────────────────────── */

static struct {
    sky_sleep_config_t config;
    bool               initialized;
    sky_power_mode_t   current_mode;
    struct {
        bool active;
        char reason[WAKELOCK_REASON_LEN];
    } wakelocks[MAX_WAKELOCKS];
    uint8_t            wakelock_count;
    esp_timer_handle_t idle_timer;
} s_sleep;

/* ── Forward declarations ───────────────────────────────────────────── */

static void idle_timer_cb(void *arg);

/* ── Internal helpers ───────────────────────────────────────────────── */

static void apply_mode(sky_power_mode_t mode)
{
    sky_power_mode_t old = s_sleep.current_mode;
    if (mode == old) return;

    switch (mode) {
    case SKY_POWER_MODE_ACTIVE:
        esp_wifi_set_ps(WIFI_PS_NONE);
        break;

    case SKY_POWER_MODE_MODEM_SLEEP:
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        ESP_LOGI(TAG, "modem sleep enabled (~20mA idle)");
        break;

    case SKY_POWER_MODE_LIGHT_SLEEP:
        esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
        ESP_LOGI(TAG, "light sleep enabled (~800uA idle)");
        break;

    case SKY_POWER_MODE_DEEP_SLEEP:
        break;
    }

    s_sleep.current_mode = mode;

    sky_power_mode_changed_event_t evt = {
        .old_mode = old,
        .new_mode = mode,
    };
    sky_event_publish(SKY_EVENT_POWER_MODE_CHANGED, &evt, sizeof(evt));
}

static void restart_idle_timer(void)
{
    if (!s_sleep.idle_timer || s_sleep.config.idle_timeout_ms == 0) return;

    esp_timer_stop(s_sleep.idle_timer);

    if (s_sleep.wakelock_count == 0) {
        esp_timer_start_once(s_sleep.idle_timer,
                             (uint64_t)s_sleep.config.idle_timeout_ms * 1000);
    }
}

static void idle_timer_cb(void *arg)
{
    (void)arg;
    if (s_sleep.wakelock_count > 0) return;

    ESP_LOGI(TAG, "idle timeout — entering %s",
             s_sleep.config.idle_mode == SKY_POWER_MODE_DEEP_SLEEP
                 ? "deep sleep" : "power save");

    if (s_sleep.config.idle_mode == SKY_POWER_MODE_DEEP_SLEEP) {
        sky_sleep_enter_deep_sleep(s_sleep.config.deep_sleep_duration_ms);
    } else {
        apply_mode(s_sleep.config.idle_mode);
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t sky_sleep_init(const sky_sleep_config_t *config)
{
    if (s_sleep.initialized) return ESP_ERR_INVALID_STATE;
    s_sleep.config = config ? *config
                            : (sky_sleep_config_t)SKY_SLEEP_CONFIG_DEFAULT();
    s_sleep.current_mode = SKY_POWER_MODE_ACTIVE;
    s_wake_count++;

    esp_timer_create_args_t args = {
        .callback = idle_timer_cb,
        .name     = "pwr_idle",
    };
    esp_timer_create(&args, &s_sleep.idle_timer);

    s_sleep.initialized = true;
    ESP_LOGI(TAG, "initialized (wake #%lu, reason=%d)",
             (unsigned long)s_wake_count,
             (int)sky_sleep_get_wake_reason());
    return ESP_OK;
}

esp_err_t sky_sleep_set_mode(sky_power_mode_t mode)
{
    if (mode == SKY_POWER_MODE_DEEP_SLEEP) {
        return sky_sleep_enter_deep_sleep(s_sleep.config.deep_sleep_duration_ms);
    }
    apply_mode(mode);
    return ESP_OK;
}

sky_power_mode_t sky_sleep_get_mode(void)
{
    return s_sleep.current_mode;
}

esp_err_t sky_sleep_acquire_wakelock(const char *reason,
                                     sky_wakelock_t *lock)
{
    if (!lock) return ESP_ERR_INVALID_ARG;

    for (int i = 0; i < MAX_WAKELOCKS; i++) {
        if (!s_sleep.wakelocks[i].active) {
            s_sleep.wakelocks[i].active = true;
            if (reason) {
                strncpy(s_sleep.wakelocks[i].reason, reason,
                        WAKELOCK_REASON_LEN - 1);
            }
            s_sleep.wakelock_count++;
            *lock = (sky_wakelock_t)i;

            if (s_sleep.idle_timer) esp_timer_stop(s_sleep.idle_timer);

            if (s_sleep.current_mode != SKY_POWER_MODE_ACTIVE) {
                apply_mode(SKY_POWER_MODE_ACTIVE);
            }

            ESP_LOGD(TAG, "wakelock acquired: %s (active=%d)",
                     reason ? reason : "?", s_sleep.wakelock_count);
            return ESP_OK;
        }
    }
    return ESP_ERR_NO_MEM;
}

esp_err_t sky_sleep_release_wakelock(sky_wakelock_t lock)
{
    if (lock >= MAX_WAKELOCKS) return ESP_ERR_INVALID_ARG;
    if (!s_sleep.wakelocks[lock].active) return ESP_ERR_INVALID_STATE;

    ESP_LOGD(TAG, "wakelock released: %s", s_sleep.wakelocks[lock].reason);
    s_sleep.wakelocks[lock].active = false;
    s_sleep.wakelocks[lock].reason[0] = '\0';
    if (s_sleep.wakelock_count > 0) s_sleep.wakelock_count--;

    if (s_sleep.wakelock_count == 0) {
        restart_idle_timer();
    }
    return ESP_OK;
}

bool sky_sleep_has_wakelocks(void)
{
    return s_sleep.wakelock_count > 0;
}

esp_err_t sky_sleep_enter_deep_sleep(uint32_t duration_ms)
{
    if (s_sleep.wakelock_count > 0) {
        ESP_LOGW(TAG, "cannot deep sleep — %d wakelocks active",
                 s_sleep.wakelock_count);
        return ESP_ERR_INVALID_STATE;
    }

    sky_sleep_enter_event_t evt = {
        .mode        = SKY_POWER_MODE_DEEP_SLEEP,
        .duration_ms = duration_ms,
    };
    sky_event_publish(SKY_EVENT_SLEEP_ENTER, &evt, sizeof(evt));

    ESP_LOGI(TAG, "entering deep sleep (%lums)...",
             (unsigned long)duration_ms);

    if (s_sleep.config.save_state_before_sleep) {
        sky_core_stop();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (duration_ms > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)duration_ms * 1000ULL);
    }

    if (s_sleep.config.wake_gpio != GPIO_NUM_MAX) {
        esp_sleep_enable_ext0_wakeup(s_sleep.config.wake_gpio,
                                     s_sleep.config.wake_on_high ? 1 : 0);
    }

    esp_deep_sleep_start();
    return ESP_OK; /* never reached */
}

sky_wake_reason_t sky_sleep_get_wake_reason(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER: return SKY_WAKE_REASON_TIMER;
    case ESP_SLEEP_WAKEUP_EXT0:
    case ESP_SLEEP_WAKEUP_EXT1:  return SKY_WAKE_REASON_GPIO;
    case ESP_SLEEP_WAKEUP_UNDEFINED:
    default:
        return s_wake_count <= 1 ? SKY_WAKE_REASON_POWERON
                                 : SKY_WAKE_REASON_UNKNOWN;
    }
}

void sky_sleep_deinit(void)
{
    if (!s_sleep.initialized) return;
    if (s_sleep.idle_timer) {
        esp_timer_stop(s_sleep.idle_timer);
        esp_timer_delete(s_sleep.idle_timer);
        s_sleep.idle_timer = NULL;
    }
    s_sleep.initialized = false;
}
