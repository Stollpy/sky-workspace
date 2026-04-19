---
name: sky-wifi-usage
description: >-
  Configure and manage Wi-Fi connectivity with sky_wifi.
  Use when setting up Wi-Fi, managing credentials, handling reconnection,
  or subscribing to network events.
---

# Sky Wi-Fi Manager

## 1. Initialize & Start

```c
#include "sky_wifi.h"

sky_wifi_config_t cfg = SKY_WIFI_CONFIG_DEFAULT();
cfg.hostname = "luxia";
// Optionally set SSID/password directly (overrides NVS/Kconfig)
// cfg.ssid = "MyNetwork";
// cfg.password = "secret";
ESP_ERROR_CHECK(sky_wifi_init(&cfg));
```

`sky_core_start()` calls `sky_wifi_start()` automatically via the registry.

### Credential Resolution Order
1. `config.ssid` / `config.password` (if provided)
2. Kconfig defaults (`CONFIG_SKY_WIFI_DEFAULT_SSID` / `CONFIG_SKY_WIFI_DEFAULT_PASSWORD`)
3. NVS stored credentials (namespace `"sky_wifi"`, keys `"ssid"` / `"pass"`)

## 2. Connection Status

```c
if (sky_wifi_is_connected()) {
    sky_wifi_status_t status;
    sky_wifi_get_status(&status);
    ESP_LOGI(TAG, "IP: %s, RSSI: %d", status.ip, status.rssi);
}
```

## 3. Auto-Reconnect

Enabled by default (`auto_reconnect = true`).

- Base interval: `retry_interval_ms` (default 5000ms)
- Backoff: exponential `base × 2^min(retry_count, 5)` → max 32× base (160s)
- `max_retry = 0` means unlimited retries
- Resets on successful connection

### Manual Reconnect

```c
sky_wifi_reconnect();  // resets retry counter and reconnects immediately
```

## 4. Credential Management

```c
sky_wifi_reset_credentials();  // erases NVS namespace "sky_wifi"
```

New credentials are saved to NVS automatically on each successful `sky_wifi_start()`.

## 5. Events

| Event | Data | When |
|-------|------|------|
| `SKY_EVENT_WIFI_CONNECTED` | `sky_wifi_connected_event_t` (ssid, ip, rssi) | Got IP |
| `SKY_EVENT_WIFI_DISCONNECTED` | `sky_wifi_disconnected_event_t` (reason) | Lost connection |
| `SKY_EVENT_WIFI_RECONNECTING` | `sky_wifi_reconnecting_event_t` (attempt, max) | Retry attempt |

## 6. Kconfig Options

| Option | Default | Description |
|--------|---------|-------------|
| `SKY_WIFI_DEFAULT_SSID` | `""` | Dev SSID |
| `SKY_WIFI_DEFAULT_PASSWORD` | `""` | Dev password |
| `SKY_WIFI_HOSTNAME` | `"sky-device"` | mDNS hostname (deferred) |
| `SKY_WIFI_MAX_RETRY` | `0` | 0 = unlimited |
| `SKY_WIFI_RETRY_INTERVAL_MS` | `5000` | Base retry interval |

## 7. Notes
- Uses ADC2 pins are unavailable when Wi-Fi is active → use ADC1 (GPIO 32–39) for battery
- mDNS functionality deferred (external component in ESP-IDF v5.x)
- Provisioning modes (BLE, SoftAP) declared but not yet implemented
