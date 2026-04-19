---
name: sky-core-usage
description: >-
  Use the Sky Core framework: event bus, service registry, NVS config, error handling, and crash safety.
  Use when creating or editing Sky components, wiring services, publishing/subscribing events,
  persisting data in NVS, or configuring crash handlers.
---

# Sky Core Framework

## Quick Start

```c
#include "sky_core.h"  // includes event, registry, config, crash, error

sky_core_init();
// ... register components ...
sky_core_start();  // calls init() then start() on all registered services
```

## 1. Event Bus

### Publish

```c
sky_stepper_position_event_t evt = { .name = "motor", .percent = 50 };
sky_event_publish(SKY_EVENT_STEPPER_POSITION, &evt, sizeof(evt));
```

### Subscribe

```c
static void on_position(const sky_event_t *event, void *ctx) {
    const sky_stepper_position_event_t *pos = event->data;
    // handle position update
}
sky_event_subscribe(SKY_EVENT_STEPPER_POSITION, on_position, NULL);
```

**Rules:**
- Never block in event handlers (async dispatch via FreeRTOS queue)
- Keep event data small (<64 bytes, copied on publish)
- Event ID ranges: core 0x00xx, stepper 0x01xx, network 0x02xx, OTA 0x028x, power 0x03xx

## 2. Service Registry

### Register a service

```c
static esp_err_t my_init(void *cfg)  { /* setup */ return ESP_OK; }
static esp_err_t my_start(void)      { /* start */ return ESP_OK; }
static esp_err_t my_stop(void)       { /* stop */  return ESP_OK; }
static esp_err_t my_deinit(void)     { /* cleanup */ return ESP_OK; }

static const sky_service_lifecycle_t lifecycle = {
    .init = my_init, .start = my_start,
    .stop = my_stop, .deinit = my_deinit,
};
sky_registry_register("my_service", &lifecycle, &my_config);
```

`sky_core_start()` calls `init` then `start` in registration order.
`sky_core_stop()` calls `stop` in reverse order.

## 3. NVS Config

```c
// Write
sky_config_set_i32("my_ns", "position", 1234);
sky_config_set_str("my_ns", "name", "motor");
sky_config_set_blob("my_ns", "cal", &cal_data, sizeof(cal_data));

// Read
int32_t pos;
sky_config_get_i32("my_ns", "position", &pos);
char name[32]; size_t len = sizeof(name);
sky_config_get_str("my_ns", "name", name, &len);

// Erase
sky_config_erase("my_ns", "position");
sky_config_erase_ns("my_ns");
```

**Constraints:** namespace ≤15 chars, key ≤15 chars.

## 4. Error Handling

```c
sky_error_report(ESP_ERR_TIMEOUT, SKY_SEVERITY_WARNING,
                 "stepper", "Movement timeout");
```

Severities: `INFO`, `WARNING`, `ERROR`, `FATAL`.
`FATAL` auto-triggers `sky_crash_trigger()`.

## 5. Crash Safety

```c
static void my_safe_state(void *ctx) {
    // ISR-safe: no malloc, no logging, no FreeRTOS calls
    gpio_set_level(PIN, 0);
}

sky_crash_config_t crash = SKY_CRASH_CONFIG_DEFAULT();
crash.safe_state_handler = my_safe_state;
sky_crash_register("my_service", &crash);
```

Actions: `NONE`, `LOG`, `STOP`, `SAFE_STATE`, `RESTART`, `REBOOT`.
