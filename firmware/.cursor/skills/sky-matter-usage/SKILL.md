---
name: sky-matter-usage
description: >-
  Integrate the Matter protocol with sky_matter.
  Use when adding Matter endpoints, configuring Window Covering callbacks,
  bridging Matter commands to stepper motors, or understanding the current skeleton status.
---

# Sky Matter Integration

**Status: SKELETON** — compiles without esp-matter SDK. All API functions are stubs.
Enable `CONFIG_SKY_MATTER_ENABLED` only after integrating the SDK.

## 1. Initialize

```c
#include "sky_matter.h"
#include "sky_matter_wc.h"

sky_matter_config_t cfg = SKY_MATTER_CONFIG_DEV_DEFAULT();
cfg.device_name      = "Luxia Store";
cfg.firmware_version = FIRMWARE_VERSION;
sky_matter_init(&cfg);
```

### Dev Defaults
| Field | Value |
|-------|-------|
| vendor_id | 0xFFF1 (test) |
| product_id | 0x0001 |
| setup_passcode | 20202021 |
| discriminator | 0xF00 |
| transport | AUTO |

## 2. Window Covering Endpoint

```c
static esp_err_t on_move_to(uint8_t percent, void *ctx) {
    return sky_stepper_set_height(motor, percent);
}
static esp_err_t on_open(void *ctx)  { return sky_stepper_set_height(motor, 0); }
static esp_err_t on_close(void *ctx) { return sky_stepper_set_height(motor, 100); }
static esp_err_t on_stop(void *ctx)  { return sky_stepper_stop(motor); }

sky_matter_wc_callbacks_t wc_cb = {
    .on_move_to_percent = on_move_to,
    .on_open  = on_open,
    .on_close = on_close,
    .on_stop  = on_stop,
};
sky_matter_wc_handle_t wc;
sky_matter_add_window_covering(&wc_cb, &wc);
```

### Push State to Matter

```c
sky_matter_wc_update_position(wc, percent);  // 0–100
sky_matter_wc_update_status(wc, SKY_MATTER_WC_STATUS_OPENING);
// Statuses: STOPPED, OPENING, CLOSING
```

## 3. Typical Bridge Pattern

Subscribe to stepper position events and push to Matter:

```c
static void on_position(const sky_event_t *event, void *ctx) {
    const sky_stepper_position_event_t *pos = event->data;
    if (pos->percent >= 0)
        sky_matter_wc_update_position(wc, (uint8_t)pos->percent);
    sky_matter_wc_update_status(wc,
        pos->moving ? (pos->direction == SKY_DIR_CW
            ? SKY_MATTER_WC_STATUS_CLOSING
            : SKY_MATTER_WC_STATUS_OPENING)
        : SKY_MATTER_WC_STATUS_STOPPED);
}
sky_event_subscribe(SKY_EVENT_STEPPER_POSITION, on_position, NULL);
```

## 4. Other API

```c
bool commissioned = sky_matter_is_commissioned();
char qr[64]; sky_matter_get_qr_code(qr, sizeof(qr));
char code[16]; sky_matter_get_manual_code(code, sizeof(code));
sky_matter_factory_reset();
```

## 5. Future: Real SDK Integration

When integrating `esp-matter`:
1. Add `esp-matter` to `idf_component.yml` or as a git submodule
2. Set `CONFIG_SKY_MATTER_ENABLED=y`
3. Replace stub implementations in `sky_matter.c` and `sky_matter_wc.c`
4. Create real node/endpoints using `esp_matter::node_t`
5. Map Window Covering cluster attributes to stepper state
6. Map Power Source cluster to battery status
