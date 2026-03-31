#include "sky_wifi.h"
#include "sky_config.h"
#include "sky_event.h"
#include "sky_events.h"
#include "sky_registry.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
/* mdns is an external component in ESP-IDF v5.x — add via idf_component.yml */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <string.h>

static const char *TAG = "sky_wifi";

#ifndef CONFIG_SKY_WIFI_MAX_RETRY
#define CONFIG_SKY_WIFI_MAX_RETRY 0
#endif
#ifndef CONFIG_SKY_WIFI_RETRY_INTERVAL_MS
#define CONFIG_SKY_WIFI_RETRY_INTERVAL_MS 5000
#endif
#ifndef CONFIG_SKY_WIFI_HOSTNAME
#define CONFIG_SKY_WIFI_HOSTNAME "sky-device"
#endif

#define NVS_NS "sky_wifi"
#define BACKOFF_CAP 5

/* ── Module state ───────────────────────────────────────────────────── */

static struct {
    sky_wifi_config_t  config;
    esp_netif_t       *netif;
    bool               initialized;
    bool               started;
    bool               connected;
    bool               registered;
    uint8_t            retry_count;
    esp_timer_handle_t retry_timer;
    char               ip_str[16];
    char               ssid[33];
    int8_t             rssi;
    int64_t            connected_at;
} s_wifi;

/* ── Forward declarations ───────────────────────────────────────────── */

static esp_err_t svc_init(void *cfg);
static esp_err_t svc_start(void);
static esp_err_t svc_stop(void);
static esp_err_t svc_deinit(void);
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data);
static void ip_event_handler(void *arg, esp_event_base_t base,
                             int32_t id, void *data);
static void retry_timer_cb(void *arg);

/* ── Credentials NVS ────────────────────────────────────────────────── */

static bool load_nvs_credentials(char *ssid, size_t ssid_len,
                                 char *pass, size_t pass_len)
{
    if (sky_config_get_str(NVS_NS, "ssid", ssid, ssid_len) != ESP_OK)
        return false;
    sky_config_get_str(NVS_NS, "pass", pass, pass_len);
    return strlen(ssid) > 0;
}

static void save_nvs_credentials(const char *ssid, const char *pass)
{
    sky_config_set_str(NVS_NS, "ssid", ssid);
    if (pass) sky_config_set_str(NVS_NS, "pass", pass);
}

/* ── Service lifecycle ──────────────────────────────────────────────── */

static esp_err_t svc_init(void *cfg) { (void)cfg; return ESP_OK; }

static esp_err_t svc_start(void)
{
    return sky_wifi_start();
}

static esp_err_t svc_stop(void)
{
    return sky_wifi_stop();
}

static esp_err_t svc_deinit(void)
{
    sky_wifi_deinit();
    return ESP_OK;
}

static esp_err_t ensure_registered(void)
{
    if (s_wifi.registered) return ESP_OK;
    static const sky_service_lifecycle_t lc = {
        .init = svc_init, .start = svc_start,
        .stop = svc_stop, .deinit = svc_deinit,
    };
    esp_err_t err = sky_registry_register("sky_wifi", &lc, NULL);
    if (err != ESP_OK) return err;
    s_wifi.registered = true;
    return ESP_OK;
}

/* ── Event handlers ─────────────────────────────────────────────────── */

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    (void)arg; (void)base;

    switch (id) {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "STA started — connecting...");
        esp_wifi_connect();
        break;

    case WIFI_EVENT_STA_DISCONNECTED: {
        s_wifi.connected = false;
        wifi_event_sta_disconnected_t *info = (wifi_event_sta_disconnected_t *)data;
        int reason = info ? info->reason : 0;
        ESP_LOGW(TAG, "disconnected (reason=%d)", reason);

        sky_wifi_disconnected_event_t evt = { .reason = reason };
        sky_event_publish(SKY_EVENT_WIFI_DISCONNECTED, &evt, sizeof(evt));

        if (s_wifi.config.auto_reconnect && s_wifi.started) {
            uint32_t shift = s_wifi.retry_count < BACKOFF_CAP
                                 ? s_wifi.retry_count : BACKOFF_CAP;
            uint32_t delay = s_wifi.config.retry_interval_ms * (1U << shift);
            esp_timer_start_once(s_wifi.retry_timer, (uint64_t)delay * 1000);
        }
        break;
    }

    default:
        break;
    }
}

static void ip_event_handler(void *arg, esp_event_base_t base,
                             int32_t id, void *data)
{
    (void)arg; (void)base;

    if (id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        snprintf(s_wifi.ip_str, sizeof(s_wifi.ip_str), IPSTR,
                 IP2STR(&ev->ip_info.ip));

        wifi_ap_record_t ap;
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            s_wifi.rssi = ap.rssi;
            memcpy(s_wifi.ssid, ap.ssid, sizeof(s_wifi.ssid));
        }

        s_wifi.connected    = true;
        s_wifi.retry_count  = 0;
        s_wifi.connected_at = esp_timer_get_time();
        ESP_LOGI(TAG, "connected — IP: %s  RSSI: %d", s_wifi.ip_str, s_wifi.rssi);

        sky_wifi_connected_event_t evt;
        memcpy(evt.ssid, s_wifi.ssid, sizeof(evt.ssid));
        memcpy(evt.ip,   s_wifi.ip_str, sizeof(evt.ip));
        evt.rssi = s_wifi.rssi;
        sky_event_publish(SKY_EVENT_WIFI_CONNECTED, &evt, sizeof(evt));
    }
}

static void retry_timer_cb(void *arg)
{
    (void)arg;
    s_wifi.retry_count++;

    if (s_wifi.config.max_retry > 0 &&
        s_wifi.retry_count >= s_wifi.config.max_retry) {
        ESP_LOGW(TAG, "max retries (%d) reached", s_wifi.config.max_retry);
        return;
    }

    sky_wifi_reconnecting_event_t evt = {
        .attempt   = s_wifi.retry_count,
        .max_retry = s_wifi.config.max_retry,
    };
    sky_event_publish(SKY_EVENT_WIFI_RECONNECTING, &evt, sizeof(evt));

    ESP_LOGI(TAG, "reconnecting (attempt %d)...", s_wifi.retry_count);
    esp_wifi_connect();
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t sky_wifi_init(const sky_wifi_config_t *config)
{
    if (s_wifi.initialized) return ESP_ERR_INVALID_STATE;

    s_wifi.config = config ? *config : (sky_wifi_config_t)SKY_WIFI_CONFIG_DEFAULT();

    if (!s_wifi.config.retry_interval_ms)
        s_wifi.config.retry_interval_ms = CONFIG_SKY_WIFI_RETRY_INTERVAL_MS;
    if (!s_wifi.config.hostname)
        s_wifi.config.hostname = CONFIG_SKY_WIFI_HOSTNAME;

    esp_err_t err;

    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;

    s_wifi.netif = esp_netif_create_default_wifi_sta();
    if (!s_wifi.netif) return ESP_FAIL;

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wifi_cfg);
    if (err != ESP_OK) return err;

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                               wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                               ip_event_handler, NULL);

    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_timer_create_args_t timer_args = {
        .callback = retry_timer_cb,
        .name     = "wifi_retry",
    };
    esp_timer_create(&timer_args, &s_wifi.retry_timer);

    /* TODO: integrate mdns via idf_component.yml for hostname resolution */

    s_wifi.initialized = true;

    ensure_registered();
    ESP_LOGI(TAG, "initialized (hostname=%s)", s_wifi.config.hostname);
    return ESP_OK;
}

esp_err_t sky_wifi_start(void)
{
    if (!s_wifi.initialized) return ESP_ERR_INVALID_STATE;
    if (s_wifi.started) return ESP_OK;

    wifi_config_t wcfg;
    memset(&wcfg, 0, sizeof(wcfg));

    const char *ssid = s_wifi.config.ssid;
    const char *pass = s_wifi.config.password;

#ifdef CONFIG_SKY_WIFI_DEFAULT_SSID
    if ((!ssid || !ssid[0]) && strlen(CONFIG_SKY_WIFI_DEFAULT_SSID) > 0) {
        ssid = CONFIG_SKY_WIFI_DEFAULT_SSID;
    }
#endif
#ifdef CONFIG_SKY_WIFI_DEFAULT_PASSWORD
    if ((!pass || !pass[0]) && strlen(CONFIG_SKY_WIFI_DEFAULT_PASSWORD) > 0) {
        pass = CONFIG_SKY_WIFI_DEFAULT_PASSWORD;
    }
#endif

    char nvs_ssid[33] = {0};
    char nvs_pass[65] = {0};
    if (!ssid || !ssid[0]) {
        if (load_nvs_credentials(nvs_ssid, sizeof(nvs_ssid),
                                 nvs_pass, sizeof(nvs_pass))) {
            ssid = nvs_ssid;
            pass = nvs_pass;
            ESP_LOGI(TAG, "loaded credentials from NVS (SSID=%s)", ssid);
        }
    }

    if (!ssid || !ssid[0]) {
        ESP_LOGW(TAG, "no credentials — WiFi will stay disconnected");
        s_wifi.started = true;
        return ESP_OK;
    }

    strncpy((char *)wcfg.sta.ssid, ssid, sizeof(wcfg.sta.ssid) - 1);
    if (pass && pass[0]) {
        strncpy((char *)wcfg.sta.password, pass, sizeof(wcfg.sta.password) - 1);
    }
    wcfg.sta.threshold.authmode = pass && pass[0]
                                      ? WIFI_AUTH_WPA2_PSK
                                      : WIFI_AUTH_OPEN;

    esp_wifi_set_config(WIFI_IF_STA, &wcfg);
    esp_err_t err = esp_wifi_start();
    if (err != ESP_OK) return err;

    save_nvs_credentials(ssid, pass);

    s_wifi.started = true;
    ESP_LOGI(TAG, "started — connecting to '%s'", ssid);
    return ESP_OK;
}

esp_err_t sky_wifi_stop(void)
{
    if (!s_wifi.started) return ESP_OK;
    esp_timer_stop(s_wifi.retry_timer);
    esp_wifi_disconnect();
    esp_wifi_stop();
    s_wifi.started   = false;
    s_wifi.connected = false;
    return ESP_OK;
}

bool sky_wifi_is_connected(void)
{
    return s_wifi.connected;
}

esp_err_t sky_wifi_get_status(sky_wifi_status_t *status)
{
    if (!status) return ESP_ERR_INVALID_ARG;
    status->connected = s_wifi.connected;
    status->rssi      = s_wifi.rssi;
    memcpy(status->ssid, s_wifi.ssid, sizeof(status->ssid));
    memcpy(status->ip,   s_wifi.ip_str, sizeof(status->ip));
    if (s_wifi.connected && s_wifi.connected_at > 0) {
        status->uptime_ms = (uint32_t)(
            (esp_timer_get_time() - s_wifi.connected_at) / 1000);
    } else {
        status->uptime_ms = 0;
    }
    return ESP_OK;
}

esp_err_t sky_wifi_reconnect(void)
{
    if (!s_wifi.started) return ESP_ERR_INVALID_STATE;
    esp_timer_stop(s_wifi.retry_timer);
    s_wifi.retry_count = 0;
    esp_wifi_disconnect();
    esp_wifi_connect();
    return ESP_OK;
}

esp_err_t sky_wifi_reset_credentials(void)
{
    sky_config_erase_ns(NVS_NS);
    ESP_LOGI(TAG, "credentials erased — re-provision on next boot");
    return ESP_OK;
}

void sky_wifi_deinit(void)
{
    if (!s_wifi.initialized) return;
    sky_wifi_stop();
    esp_timer_delete(s_wifi.retry_timer);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_event_handler);
    esp_wifi_deinit();
    esp_netif_destroy(s_wifi.netif);
    s_wifi.netif       = NULL;
    s_wifi.initialized = false;
    ESP_LOGI(TAG, "deinitialized");
}
