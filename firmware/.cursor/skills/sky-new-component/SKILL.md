---
name: sky-new-component
description: >-
  Create a new Sky framework component from scratch.
  Use when adding a new hardware driver, protocol, or feature module to the Sky ecosystem.
---

# Creating a New Sky Component

## Step 1: Create Directory Structure

```
components/sky_<name>/
├── CMakeLists.txt
├── Kconfig
├── include/
│   └── sky_<name>.h
└── src/
    └── sky_<name>.c
```

## Step 2: CMakeLists.txt

```cmake
idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "include"
    REQUIRES sky_core
    PRIV_REQUIRES driver esp_timer  # add what you need
)
```

## Step 3: Kconfig

```kconfig
menu "Sky <Name>"

    config SKY_<NAME>_OPTION
        int "Description"
        default 10
        range 1 100

endmenu
```

## Step 4: Public Header

```c
#pragma once
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // configuration fields
} sky_<name>_config_t;

#define SKY_<NAME>_CONFIG_DEFAULT() { \
    /* defaults */ \
}

esp_err_t sky_<name>_init(const sky_<name>_config_t *config);
esp_err_t sky_<name>_start(void);
esp_err_t sky_<name>_stop(void);
void      sky_<name>_deinit(void);

#ifdef __cplusplus
}
#endif
```

## Step 5: Implementation

```c
#include "sky_<name>.h"
#include "sky_registry.h"
#include "sky_event.h"
#include "sky_events.h"
#include "esp_log.h"

static const char *TAG = "sky_<name>";

static struct {
    sky_<name>_config_t config;
    bool initialized;
    bool started;
    bool registered;
} s_state;

// Service lifecycle
static esp_err_t svc_init(void *cfg)  { return ESP_OK; }
static esp_err_t svc_start(void)      { return sky_<name>_start(); }
static esp_err_t svc_stop(void)       { return sky_<name>_stop(); }
static esp_err_t svc_deinit(void)     { sky_<name>_deinit(); return ESP_OK; }

static esp_err_t ensure_registered(void) {
    if (s_state.registered) return ESP_OK;
    static const sky_service_lifecycle_t lc = {
        .init = svc_init, .start = svc_start,
        .stop = svc_stop, .deinit = svc_deinit,
    };
    esp_err_t err = sky_registry_register("sky_<name>", &lc, NULL);
    if (err != ESP_OK) return err;
    s_state.registered = true;
    return ESP_OK;
}

esp_err_t sky_<name>_init(const sky_<name>_config_t *config) {
    if (s_state.initialized) return ESP_ERR_INVALID_STATE;
    s_state.config = config ? *config
                            : (sky_<name>_config_t)SKY_<NAME>_CONFIG_DEFAULT();
    // ... setup ...
    s_state.initialized = true;
    ensure_registered();
    return ESP_OK;
}
```

## Step 6: Register Events

Add event IDs to `components/sky_core/include/sky_events.h`:

```c
/* <Name> events  0xXX00 – 0xXXFF */
#define SKY_EVENT_<NAME>_SOMETHING  0xXX00
```

## Step 7: Wire into main.c

```c
#include "sky_<name>.h"

// In app_main():
sky_<name>_config_t cfg = SKY_<NAME>_CONFIG_DEFAULT();
ESP_ERROR_CHECK(sky_<name>_init(&cfg));
```

Add to `src/CMakeLists.txt`:

```cmake
REQUIRES sky_core sky_stepper ... sky_<name>
```

## Checklist

- [ ] Header with config struct + default macro + full API
- [ ] Implementation with static state, idempotent init, registry registration
- [ ] Kconfig with configurable options
- [ ] CMakeLists with correct REQUIRES/PRIV_REQUIRES
- [ ] Events defined in sky_events.h (if component publishes events)
- [ ] Crash handler registered (if component controls hardware)
- [ ] Added to src/CMakeLists.txt REQUIRES
- [ ] `pio run` succeeds with 0 errors
