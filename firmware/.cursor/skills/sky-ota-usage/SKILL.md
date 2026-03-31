---
name: sky-ota-usage
description: >-
  Configure OTA firmware updates with sky_ota.
  Use when setting up HTTPS OTA, implementing self-tests, configuring rollback,
  or monitoring update progress.
---

# Sky OTA Updates

## 1. Initialize

```c
#include "sky_ota.h"

sky_ota_config_t cfg = SKY_OTA_CONFIG_DEFAULT();
cfg.self_test = my_self_test;
cfg.update_url = "https://update.sky.stollpy.com/firmware";
sky_ota_init(&cfg);
```

URL defaults to `CONFIG_SKY_OTA_UPDATE_URL` if not set in code.

## 2. Self-Test Callback

Runs automatically after an OTA reboot (when partition is `PENDING_VERIFY`):

```c
static void my_self_test(bool *passed, void *ctx) {
    // Wait for Wi-Fi
    for (int i = 0; i < 100; i++) {
        if (sky_wifi_is_connected()) break;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (!sky_wifi_is_connected()) {
        *passed = false;
        return;
    }
    // Verify critical components
    if (!sky_stepper_get("motor")) {
        *passed = false;
        return;
    }
    *passed = true;
}
```

### Self-Test Flow
1. Boot after OTA → partition state = `ESP_OTA_IMG_PENDING_VERIFY`
2. Timeout timer starts (`self_test_timeout_ms`, default 30s)
3. `self_test` callback runs **synchronously**
4. If `passed = true` → `esp_ota_mark_app_valid_cancel_rollback()`
5. If `passed = false` or timeout → `esp_ota_mark_app_invalid_rollback_and_reboot()`

## 3. Trigger an Update

```c
sky_ota_update();  // creates FreeRTOS task, downloads firmware via HTTPS
```

### Progress Monitoring

```c
static void on_progress(const sky_event_t *event, void *ctx) {
    sky_ota_progress_event_t *p = event->data;
    ESP_LOGI(TAG, "OTA: %d%%", p->percent);
}
sky_event_subscribe(SKY_EVENT_OTA_PROGRESS, on_progress, NULL);
```

### Completion

```c
static void on_complete(const sky_event_t *event, void *ctx) {
    sky_ota_complete_event_t *c = event->data;
    // c->success, c->version
}
sky_event_subscribe(SKY_EVENT_OTA_COMPLETE, on_complete, NULL);
```

On success: `sky_core_stop()` → 2s delay → `esp_restart()`.

## 4. Check for Updates

```c
bool available;
sky_ota_check(&available);  // currently a stub (always false)
```

## 5. Kconfig

| Option | Default | Description |
|--------|---------|-------------|
| `SKY_OTA_UPDATE_URL` | `https://update.sky.stollpy.com/firmware` | HTTPS URL |
| `SKY_OTA_SELF_TEST_TIMEOUT_MS` | `30000` | Rollback timeout |
| `SKY_OTA_CHECK_INTERVAL_MS` | `0` | Periodic check (0=disabled) |
| `SKY_OTA_SKIP_SERVER_CERT` | `y` | Skip TLS cert validation (dev) |

## 6. Partition Requirements

OTA requires dual app partitions (`ota_0`, `ota_1`) + `otadata`.
Current layout: ~1.9MB per slot (4MB flash total).

## 7. sdkconfig

```
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
```

This must be set for rollback to work.
