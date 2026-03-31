#include "sky_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "sky_config";
static bool s_initialized = false;

esp_err_t sky_config_init(void)
{
    if (s_initialized) return ESP_ERR_INVALID_STATE;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;

    s_initialized = true;
    ESP_LOGI(TAG, "initialized (NVS)");
    return ESP_OK;
}

/* ── u32 ────────────────────────────────────────────────────────────── */

esp_err_t sky_config_set_u32(sky_config_ns_t ns, const char *key, uint32_t value)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_u32(h, key, value);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t sky_config_get_u32(sky_config_ns_t ns, const char *key, uint32_t *value)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    err = nvs_get_u32(h, key, value);
    nvs_close(h);
    return err;
}

/* ── i32 ────────────────────────────────────────────────────────────── */

esp_err_t sky_config_set_i32(sky_config_ns_t ns, const char *key, int32_t value)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_i32(h, key, value);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t sky_config_get_i32(sky_config_ns_t ns, const char *key, int32_t *value)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    err = nvs_get_i32(h, key, value);
    nvs_close(h);
    return err;
}

/* ── string ─────────────────────────────────────────────────────────── */

esp_err_t sky_config_set_str(sky_config_ns_t ns, const char *key, const char *value)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_str(h, key, value);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t sky_config_get_str(sky_config_ns_t ns, const char *key,
                             char *buf, size_t len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    err = nvs_get_str(h, key, buf, &len);
    nvs_close(h);
    return err;
}

/* ── blob ───────────────────────────────────────────────────────────── */

esp_err_t sky_config_set_blob(sky_config_ns_t ns, const char *key,
                              const void *data, size_t len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, key, data, len);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t sky_config_get_blob(sky_config_ns_t ns, const char *key,
                              void *data, size_t *len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    err = nvs_get_blob(h, key, data, len);
    nvs_close(h);
    return err;
}

/* ── erase ──────────────────────────────────────────────────────────── */

esp_err_t sky_config_erase(sky_config_ns_t ns, const char *key)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_erase_key(h, key);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t sky_config_erase_ns(sky_config_ns_t ns)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_erase_all(h);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

void sky_config_deinit(void)
{
    if (!s_initialized) return;
    s_initialized = false;
}
