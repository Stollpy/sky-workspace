---
name: sky-stepper-usage
description: >-
  Register, control, calibrate, and create patterns for stepper motors using sky_stepper.
  Use when adding motors, controlling movement, calibrating positions, creating presets/sequences,
  or adding event hooks to stepper motors.
---

# Sky Stepper Motor

## 1. Register a Motor

```c
#include "sky_stepper.h"

sky_stepper_config_t cfg = SKY_STEPPER_CONFIG_DEFAULT();
cfg.name = "store";           // client-configurable name
cfg.gpio.pins[0] = GPIO_NUM_17;  // IN1
cfg.gpio.pins[1] = GPIO_NUM_16;  // IN2
cfg.gpio.pins[2] = GPIO_NUM_4;   // IN3
cfg.gpio.pins[3] = GPIO_NUM_0;   // IN4

sky_stepper_handle_t motor;
ESP_ERROR_CHECK(sky_stepper_register(&cfg, &motor));
```

Default engine: 28BYJ-48, half-step mode, 4096 steps/rev, 2000µs step delay.
Up to `CONFIG_SKY_STEPPER_MAX_MOTORS` (default 4) motors per ESP32.

## 2. Movement Commands

```c
sky_stepper_move(motor, 2048, SKY_DIR_CW);     // relative: 2048 steps clockwise
sky_stepper_move_to(motor, 1000);               // absolute: go to position 1000
sky_stepper_set_height(motor, 50);              // percentage (requires calibration)
sky_stepper_stop(motor);                        // graceful stop
sky_stepper_emergency_stop(motor);              // immediate GPIO off
```

All commands are **async** — sent to the motor's FreeRTOS queue.

## 3. Read Status

```c
sky_stepper_state_t state = sky_stepper_get_state(motor);   // IDLE, MOVING, etc.
int32_t pos = sky_stepper_get_position(motor);              // step count
int8_t pct = sky_stepper_get_height_percent(motor);         // -1 if uncalibrated

sky_stepper_status_t status;
sky_stepper_get_status(motor, &status);
```

## 4. Acceleration Ramp

```c
sky_stepper_ramp_config_t ramp = SKY_STEPPER_RAMP_DEFAULT();
// min_delay=1500µs, max_delay=4000µs, accel=200 steps, decel=200 steps
sky_stepper_set_ramp(motor, &ramp);
```

## 5. Event Hooks

```c
static void before_move(sky_stepper_hook_event_t *event, void *ctx) {
    if (should_block()) {
        event->cancel = true;  // cancels the movement
    }
}
sky_stepper_add_hook(motor, SKY_STEPPER_HOOK_BEFORE_MOVE, before_move, NULL);
sky_stepper_remove_hook(motor, SKY_STEPPER_HOOK_BEFORE_MOVE, before_move);
```

Hook types: `BEFORE_MOVE`, `AFTER_MOVE`, `BEFORE_STOP`, `AFTER_STOP`, `ON_ERROR`, `ON_POSITION`.
Currently only `BEFORE_MOVE` and `AFTER_MOVE` are invoked.

## 6. Calibration

```c
// Step 1: Start calibration (motor moves in given direction)
sky_stepper_calibrate_start(motor, SKY_DIR_CW);

// Step 2: When motor reaches open position, mark it
sky_stepper_calibrate_mark_open(motor);

// Step 3: When motor reaches closed position, mark it
sky_stepper_calibrate_mark_closed(motor);  // saves to NVS, calibration complete

// Query
bool cal = sky_stepper_is_calibrated(motor);
sky_stepper_calibration_t data;
sky_stepper_get_calibration(motor, &data);
// data.total_steps, data.open_position, data.closed_position

// Reset
sky_stepper_clear_calibration(motor);
```

Timeout: `CONFIG_SKY_STEPPER_CALIBRATION_TIMEOUT_S` (default 120s).

## 7. Presets & Sequences

### Presets

```c
sky_stepper_preset_save(motor, "half", 50);   // save 50% position
sky_stepper_preset_apply(motor, "half");       // move to saved position
sky_stepper_preset_delete(motor, "half");
```

### Sequences

```c
sky_stepper_sequence_record_start(motor, "morning");
sky_stepper_sequence_record_step(motor, 30, 0);       // go to 30%, no delay
sky_stepper_sequence_record_step(motor, 70, 5000);     // go to 70%, wait 5s
sky_stepper_sequence_record_stop(motor);                // saves to NVS

sky_stepper_sequence_play(motor, "morning");
sky_stepper_sequence_stop(motor);
```

### Pattern Management

```c
sky_pattern_t patterns[8]; uint8_t count;
sky_stepper_pattern_list(motor, patterns, &count, 8);

sky_pattern_t exported;
sky_stepper_pattern_export(motor, "morning", &exported);
sky_stepper_pattern_import(motor, &exported);
```

## 8. Events Published

| Event | Data struct | When |
|-------|------------|------|
| `SKY_EVENT_STEPPER_STARTED` | `sky_stepper_started_event_t` | Movement begins |
| `SKY_EVENT_STEPPER_POSITION` | `sky_stepper_position_event_t` | Every N steps (config) |
| `SKY_EVENT_STEPPER_STOPPED` | `sky_stepper_stopped_event_t` | Movement ends |
| `SKY_EVENT_STEPPER_CALIBRATED` | — | Calibration complete |
