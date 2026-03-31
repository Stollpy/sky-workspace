#include "sky_ota.h"
#include "sky_core.h"
#include "sky_event.h"
#include "sky_events.h"
#include "sky_registry.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_app_desc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "sky_ota";

#ifndef CONFIG_SKY_OTA_SELF_TEST_TIMEOUT_MS
#define CONFIG_SKY_OTA_SELF_TEST_TIMEOUT_MS 30000
#endif
#ifndef CONFIG_SKY_OTA_CHECK_INTERVAL_MS
#define CONFIG_SKY_OTA_CHECK_INTERVAL_MS 0
#endif
#ifndef CONFIG_SKY_OTA_AUTO_UPDATE
#define CONFIG_SKY_OTA_AUTO_UPDATE 0
#endif
#ifndef CONFIG_SKY_OTA_SKIP_SERVER_CERT
#define CONFIG_SKY_OTA_SKIP_SERVER_CERT 1
#endif

/* ── Module state ───────────────────────────────────────────────────── */

static struct {
    sky_ota_config_t   config;
    bool               initialized;
    bool               started;
    bool               registered;
    bool               update_in_progress;
    uint8_t            download_progress;
    esp_timer_handle_t check_timer;
} s_ota;

/* ── Forward declarations ───────────────────────────────────────────── */

static esp_err_t svc_init(void *cfg);
static esp_err_t svc_start(void);
static esp_err_t svc_stop(void);
static esp_err_t svc_deinit(void);
static void ota_task(void *arg);
static void check_timer_cb(void *arg);

/* ── Self-test engine ───────────────────────────────────────────────── */

static void self_test_timeout_cb(void *arg)
{
    (void)arg;
    ESP_LOGE(TAG, "self-test TIMEOUT — rolling back");
    esp_ota_mark_app_invalid_rollback_and_reboot();
}

static void run_self_test(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;

    if (esp_ota_get_state_partition(running, &state) != ESP_OK) return;
    if (state != ESP_OTA_IMG_PENDING_VERIFY) return;

    ESP_LOGI(TAG, "firmware pending verification — running self-test...");

    esp_timer_handle_t timeout = NULL;
    esp_timer_create_args_t args = {
        .callback = self_test_timeout_cb,
        .name     = "ota_st_to",
    };
    esp_timer_create(&args, &timeout);

    uint32_t timeout_ms = s_ota.config.self_test_timeout_ms
                              ? s_ota.config.self_test_timeout_ms
                              : CONFIG_SKY_OTA_SELF_TEST_TIMEOUT_MS;
    esp_timer_start_once(timeout, (uint64_t)timeout_ms * 1000);

    bool passed = true;
    if (s_ota.config.self_test) {
        s_ota.config.self_test(&passed, s_ota.config.self_test_ctx);
    }

    esp_timer_stop(timeout);
    esp_timer_delete(timeout);

    if (passed) {
        ESP_LOGI(TAG, "self-test PASSED — marking firmware valid");
        esp_ota_mark_app_valid_cancel_rollback();
    } else {
        ESP_LOGE(TAG, "self-test FAILED — rolling back");
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
}

/* ── Service lifecycle ──────────────────────────────────────────────── */

static esp_err_t svc_init(void *cfg) { (void)cfg; return ESP_OK; }

static esp_err_t svc_start(void)
{
    run_self_test();
    s_ota.started = true;

    if (s_ota.config.check_interval_ms > 0 && s_ota.check_timer) {
        esp_timer_start_periodic(s_ota.check_timer,
                                 (uint64_t)s_ota.config.check_interval_ms * 1000);
    }
    return ESP_OK;
}

static esp_err_t svc_stop(void)
{
    if (s_ota.check_timer) esp_timer_stop(s_ota.check_timer);
    s_ota.started = false;
    return ESP_OK;
}

static esp_err_t svc_deinit(void)
{
    svc_stop();
    if (s_ota.check_timer) {
        esp_timer_delete(s_ota.check_timer);
        s_ota.check_timer = NULL;
    }
    return ESP_OK;
}

static esp_err_t ensure_registered(void)
{
    if (s_ota.registered) return ESP_OK;
    static const sky_service_lifecycle_t lc = {
        .init = svc_init, .start = svc_start,
        .stop = svc_stop, .deinit = svc_deinit,
    };
    esp_err_t err = sky_registry_register("sky_ota", &lc, NULL);
    if (err != ESP_OK) return err;
    s_ota.registered = true;
    return ESP_OK;
}

/* ── OTA download task ──────────────────────────────────────────────── */

static void ota_task(void *arg)
{
    (void)arg;
    const char *url = s_ota.config.update_url;

    if (!url || !url[0]) {
        ESP_LOGE(TAG, "no update URL configured");
        s_ota.update_in_progress = false;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "starting OTA from %s", url);

    esp_http_client_config_t http_cfg = {
        .url            = url,
        .cert_pem       = s_ota.config.server_cert_pem,
        .timeout_ms     = 60000,
        .keep_alive_enable = true,
    };

#if CONFIG_SKY_OTA_SKIP_SERVER_CERT
    if (!http_cfg.cert_pem) {
        http_cfg.skip_cert_common_name_check = true;
        http_cfg.transport_type = HTTP_TRANSPORT_OVER_SSL;
    }
#endif

    esp_https_ota_config_t ota_cfg = { .http_config = &http_cfg };
    esp_https_ota_handle_t handle = NULL;

    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        goto done;
    }

    int image_size = esp_https_ota_get_image_size(handle);
    ESP_LOGI(TAG, "image size: %d bytes", image_size);

    while (1) {
        err = esp_https_ota_perform(handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) break;

        if (image_size > 0) {
            int read = esp_https_ota_get_image_len_read(handle);
            uint8_t pct = (uint8_t)((int64_t)read * 100 / image_size);
            if (pct != s_ota.download_progress) {
                s_ota.download_progress = pct;
                sky_ota_progress_event_t evt = { .percent = pct };
                sky_event_publish(SKY_EVENT_OTA_PROGRESS, &evt, sizeof(evt));
                if (pct % 10 == 0) {
                    ESP_LOGI(TAG, "download: %d%%", pct);
                }
            }
        }
    }

    bool success = false;
    if (err == ESP_OK && esp_https_ota_is_complete_data_received(handle)) {
        err = esp_https_ota_finish(handle);
        handle = NULL;
        success = (err == ESP_OK);
    } else {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(err));
        if (handle) esp_https_ota_abort(handle);
    }

    sky_ota_complete_event_t cev = { .success = success };
    const esp_app_desc_t *desc = esp_app_get_description();
    if (desc) strncpy(cev.version, desc->version, sizeof(cev.version) - 1);
    sky_event_publish(SKY_EVENT_OTA_COMPLETE, &cev, sizeof(cev));

    if (success) {
        ESP_LOGI(TAG, "OTA complete — rebooting in 2s...");
        sky_core_stop();
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    }

done:
    s_ota.update_in_progress = false;
    vTaskDelete(NULL);
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════ */

static void check_timer_cb(void *arg)
{
    (void)arg;
    sky_ota_update();
}

esp_err_t sky_ota_init(const sky_ota_config_t *config)
{
    if (s_ota.initialized) return ESP_ERR_INVALID_STATE;
    s_ota.config = config ? *config : (sky_ota_config_t)SKY_OTA_CONFIG_DEFAULT();

#ifdef CONFIG_SKY_OTA_UPDATE_URL
    if (!s_ota.config.update_url && strlen(CONFIG_SKY_OTA_UPDATE_URL) > 0)
        s_ota.config.update_url = CONFIG_SKY_OTA_UPDATE_URL;
#endif

    if (s_ota.config.check_interval_ms > 0) {
        esp_timer_create_args_t args = {
            .callback = check_timer_cb,
            .name     = "ota_check",
        };
        esp_timer_create(&args, &s_ota.check_timer);
    }

    s_ota.initialized = true;
    ensure_registered();

    const esp_app_desc_t *desc = esp_app_get_description();
    ESP_LOGI(TAG, "initialized (version=%s url=%s)",
             desc ? desc->version : "?",
             s_ota.config.update_url ? s_ota.config.update_url : "none");
    return ESP_OK;
}

esp_err_t sky_ota_start(void)
{
    return svc_start();
}

esp_err_t sky_ota_check(bool *update_available)
{
    if (update_available) *update_available = false;
    ESP_LOGW(TAG, "version check not yet implemented — use sky_ota_update()");
    return ESP_OK;
}

esp_err_t sky_ota_update(void)
{
    if (s_ota.update_in_progress) {
        ESP_LOGW(TAG, "update already in progress");
        return ESP_ERR_INVALID_STATE;
    }
    s_ota.update_in_progress  = true;
    s_ota.download_progress   = 0;

    BaseType_t ok = xTaskCreate(ota_task, "ota", 8192, NULL, 5, NULL);
    return ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t sky_ota_get_status(sky_ota_status_t *status)
{
    if (!status) return ESP_ERR_INVALID_ARG;
    memset(status, 0, sizeof(*status));

    const esp_app_desc_t *desc = esp_app_get_description();
    if (desc) strncpy(status->current_version, desc->version,
                      sizeof(status->current_version) - 1);

    status->update_in_progress = s_ota.update_in_progress;
    status->download_progress  = s_ota.download_progress;
    return ESP_OK;
}

void sky_ota_deinit(void)
{
    if (!s_ota.initialized) return;
    svc_deinit();
    s_ota.initialized = false;
}
