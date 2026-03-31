#include "sky_http_server.h"
#include "sky_registry.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "sky_http";

/* ── Module state ──────────────────────────────────────────────────── */

static struct {
    sky_http_server_config_t config;
    httpd_handle_t           handle;
    bool                     initialized;
    bool                     started;
    bool                     registered;
} s_http;

/* ── Internal: register a single route on httpd ────────────────────── */

static esp_err_t register_route(const sky_http_route_t *route)
{
    httpd_uri_t uri_def = {
        .uri      = route->uri,
        .method   = route->method,
        .handler  = route->handler,
        .user_ctx = route->user_ctx,
    };
    return httpd_register_uri_handler(s_http.handle, &uri_def);
}

/* ── Service lifecycle ─────────────────────────────────────────────── */

static esp_err_t svc_init(void *cfg)  { (void)cfg; return ESP_OK; }
static esp_err_t svc_start(void)      { return sky_http_server_start(); }
static esp_err_t svc_stop(void)       { return sky_http_server_stop(); }
static esp_err_t svc_deinit(void)     { sky_http_server_deinit(); return ESP_OK; }

static esp_err_t ensure_registered(void)
{
    if (s_http.registered) {
        return ESP_OK;
    }
    static const sky_service_lifecycle_t lc = {
        .init = svc_init, .start = svc_start,
        .stop = svc_stop, .deinit = svc_deinit,
    };
    esp_err_t err = sky_registry_register("sky_http", &lc, NULL);
    if (err != ESP_OK) {
        return err;
    }
    s_http.registered = true;
    return ESP_OK;
}

/* ── Public API ────────────────────────────────────────────────────── */

esp_err_t sky_http_server_init(const sky_http_server_config_t *config)
{
    if (s_http.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_http.config = config ? *config
                           : (sky_http_server_config_t)SKY_HTTP_SERVER_CONFIG_DEFAULT();

    s_http.handle      = NULL;
    s_http.initialized = true;

    ensure_registered();
    ESP_LOGI(TAG, "initialized (port=%d, max_routes=%d)",
             s_http.config.port, (int)s_http.config.max_uri_handlers);
    return ESP_OK;
}

esp_err_t sky_http_server_start(void)
{
    if (!s_http.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_http.started) {
        return ESP_OK;
    }

    httpd_config_t httpd_cfg    = HTTPD_DEFAULT_CONFIG();
    httpd_cfg.server_port       = s_http.config.port;
    httpd_cfg.max_uri_handlers  = s_http.config.max_uri_handlers;
    httpd_cfg.stack_size        = s_http.config.stack_size;
    httpd_cfg.lru_purge_enable  = true;

    esp_err_t err = httpd_start(&s_http.handle, &httpd_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "failed to start httpd: %s", esp_err_to_name(err));
        return err;
    }

    for (size_t i = 0; i < s_http.config.route_count; i++) {
        err = register_route(&s_http.config.routes[i]);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "failed to register route %s: %s",
                     s_http.config.routes[i].uri, esp_err_to_name(err));
        }
    }

    s_http.started = true;
    ESP_LOGI(TAG, "started on port %d (%d routes)",
             s_http.config.port, (int)s_http.config.route_count);
    return ESP_OK;
}

esp_err_t sky_http_server_stop(void)
{
    if (!s_http.started) {
        return ESP_OK;
    }
    if (s_http.handle) {
        httpd_stop(s_http.handle);
        s_http.handle = NULL;
    }
    s_http.started = false;
    ESP_LOGI(TAG, "stopped");
    return ESP_OK;
}

void sky_http_server_deinit(void)
{
    if (!s_http.initialized) {
        return;
    }
    sky_http_server_stop();
    s_http.initialized = false;
    ESP_LOGI(TAG, "deinitialized");
}

esp_err_t sky_http_server_add_route(const sky_http_route_t *route)
{
    if (!route || !route->uri || !route->handler) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_http.started && s_http.handle) {
        return register_route(route);
    }

    ESP_LOGW(TAG, "add_route called before start — use config.routes instead");
    return ESP_ERR_INVALID_STATE;
}
