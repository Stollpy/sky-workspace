---
name: luxia-firmware-assembly
description: >-
  Assemble and wire the Luxia product firmware from Sky components.
  Use when editing main.c, adding bridges between components, configuring GPIO pinout,
  or understanding how all Sky packages connect in a product.
---

# Luxia Firmware Assembly

## Architecture

```
main.c (product glue)
├── sky_core       (event bus, registry, config, crash)
├── sky_power      (battery ADC, thresholds)
├── sky_wifi       (STA, auto-reconnect)
├── sky_stepper    (motor HAL, registry, motion)
├── sky_matter     (Window Covering, Power Source)
├── sky_ota        (HTTPS update, self-test, rollback)
├── sky_sleep      (modes, wakelocks, deep sleep)
└── luxia_cmd      (custom command handlers)
```

## Startup Order

**Critical — do not reorder.** Dependencies flow downward:

```c
void app_main(void) {
    sky_core_init();                        // 1. Framework
    sky_power_init(&power_cfg);             // 2. Battery (needed early)
    sky_wifi_init(&wifi_cfg);               // 3. Network
    sky_stepper_register(&motor_cfg, &h);   // 4. Motor + hooks
    sky_matter_init(&matter_cfg);           // 5. Protocol
    sky_matter_add_window_covering(&wc_cb); // 5b. Endpoint
    sky_ota_init(&ota_cfg);                 // 6. Updates
    sky_sleep_init(&sleep_cfg);             // 7. Power save (last)
    luxia_cmd_init(h);                      // 8. Command API
    sky_event_subscribe(...);               // 9. Event bridges
    sky_core_start();                       // 10. Start all services
}
```

## Inter-Component Bridges

### Matter → Stepper (commands)

```c
sky_matter_wc_callbacks_t wc_cb = {
    .on_move_to_percent = on_wc_move_to,  // → sky_stepper_set_height
    .on_open  = on_wc_open,               // → set_height(0)
    .on_close = on_wc_close,              // → set_height(100)
    .on_stop  = on_wc_stop,               // → sky_stepper_stop
};
```

### Stepper → Matter (state)

```c
sky_event_subscribe(SKY_EVENT_STEPPER_POSITION, on_position_event, NULL);
// on_position_event → sky_matter_wc_update_position + update_status
```

### Power → Stepper (guard)

```c
sky_stepper_add_hook(motor, SKY_STEPPER_HOOK_BEFORE_MOVE,
                     battery_motor_guard, NULL);
// battery_motor_guard → event->cancel if sky_power_is_critical()
```

## GPIO Pinout

| Function | GPIO | Notes |
|----------|------|-------|
| Stepper IN1 | 17 | |
| Stepper IN2 | 16 | |
| Stepper IN3 | 4 | |
| Stepper IN4 | 0 | Boot mode pin — needs attention |
| Battery ADC | 34 | ADC1, input-only |
| Wake button | 35 | input-only, external pull-up |

## luxia_cmd Module

Product-specific command handlers:

| Category | IDs | Commands |
|----------|-----|----------|
| Calibration | 0x01–0x05 | start, mark_open, mark_closed, cancel, status |
| Presets | 0x10–0x13 | save, apply, delete, list |
| Sequences | 0x20–0x28 | record, play, stop, list, export, import |
| Diagnostics | 0x40–0x46 | status, reboot, factory_reset, version, ota, set_name |

```c
luxia_cmd_init(motor_handle);  // stores motor ref for all handlers
luxia_status_t s; luxia_cmd_get_status(&s);  // full device snapshot
```

## Build Targets

```bash
pio run -e luxia           # Build
pio run -e luxia -t upload # Flash
pio run -e luxia -t monitor # Serial monitor
```

Current: RAM 14.8% | Flash 43.9% (well within 1.9MB OTA slot).
