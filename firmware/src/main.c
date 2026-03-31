#include "sky_core.h"
#include "sky_stepper.h"
#include "sky_wifi.h"
#include "sky_matter.h"
#include "sky_matter_wc.h"
#include "sky_ota.h"
#include "sky_power.h"
#include "sky_sleep.h"
#include "sky_http_server.h"
#include "luxia_cmd.h"
#include "luxia_http.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define FIRMWARE_VERSION "0.1.0"

static const char *TAG = "luxia";

static sky_stepper_handle_t   s_motor;
static sky_matter_wc_handle_t s_wc;

/* ── Battery → Stepper guard (blocks motor when critical) ───────────── */

static void battery_motor_guard(sky_stepper_hook_event_t *event, void *ctx)
{
    (void)ctx;
    if (sky_power_is_critical()) {
        event->cancel = true;
        ESP_LOGW(TAG, "motor blocked: battery critical (%d%%)",
                 sky_power_get_percent());
    }
}

/* ── Stepper → Matter bridge (position + status sync) ───────────────── */

static void on_position_event(const sky_event_t *event, void *ctx)
{
    (void)ctx;
    const sky_stepper_position_event_t *pos =
        (const sky_stepper_position_event_t *)event->data;

    if (pos->percent >= 0) {
        sky_matter_wc_update_position(s_wc, (uint8_t)pos->percent);
    }

    if (pos->moving) {
        sky_matter_wc_update_status(s_wc,
            pos->direction == SKY_DIR_CW
                ? SKY_MATTER_WC_STATUS_CLOSING
                : SKY_MATTER_WC_STATUS_OPENING);
    } else {
        sky_matter_wc_update_status(s_wc, SKY_MATTER_WC_STATUS_STOPPED);
    }
}

/* ── Matter → Stepper bridge (commands) ─────────────────────────────── */

static esp_err_t on_wc_move_to(uint8_t percent, void *ctx)
{
    (void)ctx;
    return sky_stepper_set_height(s_motor, percent);
}

static esp_err_t on_wc_open(void *ctx)
{
    (void)ctx;
    return sky_stepper_set_height(s_motor, 0);
}

static esp_err_t on_wc_close(void *ctx)
{
    (void)ctx;
    return sky_stepper_set_height(s_motor, 100);
}

static esp_err_t on_wc_stop(void *ctx)
{
    (void)ctx;
    return sky_stepper_stop(s_motor);
}

/* ── OTA self-test ──────────────────────────────────────────────────── */

static void luxia_self_test(bool *passed, void *ctx)
{
    (void)ctx;

    for (int i = 0; i < 100; i++) {
        if (sky_wifi_is_connected()) break;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (!sky_wifi_is_connected()) {
        ESP_LOGE(TAG, "self-test FAIL: WiFi not connected");
        *passed = false;
        return;
    }

    if (!sky_stepper_get("motor")) {
        ESP_LOGE(TAG, "self-test FAIL: motor not found");
        *passed = false;
        return;
    }

    *passed = true;
    ESP_LOGI(TAG, "self-test PASS");
}

/* ── Application entry point ────────────────────────────────────────── */

void app_main(void)
{
    ESP_LOGI(TAG, "Luxia v%s starting... (heap: %lu)",
             FIRMWARE_VERSION, (unsigned long)esp_get_free_heap_size());

    /* 1. Core framework */
    sky_core_init();

    /* 2. Power management (early — battery status needed before motors) */
    sky_power_config_t power_cfg = SKY_POWER_CONFIG_LIPO_DEFAULT();
    ESP_ERROR_CHECK(sky_power_init(&power_cfg));

    /* 3. Wi-Fi */
    sky_wifi_config_t wifi_cfg = SKY_WIFI_CONFIG_DEFAULT();
    wifi_cfg.hostname = "luxia";
    ESP_ERROR_CHECK(sky_wifi_init(&wifi_cfg));

    /* 4. Stepper motor (default name, client renames later) */
    sky_stepper_config_t motor_cfg = SKY_STEPPER_CONFIG_DEFAULT();
    motor_cfg.gpio.pins[0] = GPIO_NUM_17;
    motor_cfg.gpio.pins[1] = GPIO_NUM_16;
    motor_cfg.gpio.pins[2] = GPIO_NUM_4;
    motor_cfg.gpio.pins[3] = GPIO_NUM_0;
    ESP_ERROR_CHECK(sky_stepper_register(&motor_cfg, &s_motor));

    sky_stepper_add_hook(s_motor, SKY_STEPPER_HOOK_BEFORE_MOVE,
                         battery_motor_guard, NULL);

    /* 5. Matter (skeleton — enable when SDK is integrated) */
    sky_matter_config_t matter_cfg = SKY_MATTER_CONFIG_DEV_DEFAULT();
    matter_cfg.device_name      = "Luxia Store";
    matter_cfg.firmware_version = FIRMWARE_VERSION;
    sky_matter_init(&matter_cfg);

    sky_matter_wc_callbacks_t wc_cb = {
        .on_move_to_percent = on_wc_move_to,
        .on_open            = on_wc_open,
        .on_close           = on_wc_close,
        .on_stop            = on_wc_stop,
    };
    sky_matter_add_window_covering(&wc_cb, &s_wc);

    /* 6. OTA */
    sky_ota_config_t ota_cfg = SKY_OTA_CONFIG_DEFAULT();
    ota_cfg.self_test = luxia_self_test;
    sky_ota_init(&ota_cfg);

    /* 7. Sleep (starts idle timer after all services are ready) */
    sky_sleep_config_t sleep_cfg = SKY_SLEEP_CONFIG_DEFAULT();
    sleep_cfg.wake_gpio   = GPIO_NUM_35;
    sleep_cfg.wake_on_high = true;
    ESP_ERROR_CHECK(sky_sleep_init(&sleep_cfg));

    /* 8. HTTP Server */
    size_t route_count = 0;
    const sky_http_route_t *routes = luxia_http_get_routes(&route_count);
    sky_http_server_config_t http_cfg = SKY_HTTP_SERVER_CONFIG_DEFAULT();
    http_cfg.routes      = routes;
    http_cfg.route_count = route_count;
    ESP_ERROR_CHECK(sky_http_server_init(&http_cfg));

    /* 9. Command module (custom API: calibration, patterns, diagnostics) */
    luxia_cmd_init(s_motor);

    /* 10. Event bridges */
    sky_event_subscribe(SKY_EVENT_STEPPER_POSITION,
                        on_position_event, NULL);

    /* 11. Start everything */
    sky_core_start();

    ESP_LOGI(TAG, "Luxia v%s ready (battery: %d%%, wake: %s, heap: %lu)",
             FIRMWARE_VERSION,
             sky_power_get_percent(),
             sky_sleep_get_wake_reason() == SKY_WAKE_REASON_POWERON
                 ? "poweron"
                 : sky_sleep_get_wake_reason() == SKY_WAKE_REASON_TIMER
                     ? "timer"
                     : sky_sleep_get_wake_reason() == SKY_WAKE_REASON_GPIO
                         ? "gpio" : "unknown",
             (unsigned long)esp_get_free_heap_size());
}
