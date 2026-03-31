#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include "sky_crash.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Step modes ─────────────────────────────────────────────────────── */

typedef enum {
    SKY_STEP_MODE_WAVE,
    SKY_STEP_MODE_FULL,
    SKY_STEP_MODE_HALF,
} sky_step_mode_t;

typedef enum {
    SKY_DIR_CW,
    SKY_DIR_CCW,
} sky_direction_t;

/* ── Motor states ───────────────────────────────────────────────────── */

typedef enum {
    SKY_STEPPER_STATE_IDLE,
    SKY_STEPPER_STATE_MOVING,
    SKY_STEPPER_STATE_STOPPING,
    SKY_STEPPER_STATE_CALIBRATING,
    SKY_STEPPER_STATE_ERROR,
} sky_stepper_state_t;

/* ── GPIO config ────────────────────────────────────────────────────── */

typedef struct {
    gpio_num_t pins[4];
    bool       invert_logic;
} sky_stepper_gpio_config_t;

/* ── Step engine config ─────────────────────────────────────────────── */

typedef struct {
    sky_step_mode_t mode;
    uint32_t        step_delay_us;
    uint16_t        steps_per_revolution;
} sky_step_engine_config_t;

#define SKY_STEP_ENGINE_28BYJ48_DEFAULT() { \
    .mode                = SKY_STEP_MODE_HALF, \
    .step_delay_us       = 2000,              \
    .steps_per_revolution = 4096,              \
}

/* ── Acceleration ramp ──────────────────────────────────────────────── */

typedef struct {
    uint32_t min_delay_us;
    uint32_t max_delay_us;
    uint16_t accel_steps;
    uint16_t decel_steps;
} sky_stepper_ramp_config_t;

#define SKY_STEPPER_RAMP_DEFAULT() { \
    .min_delay_us = 1500, \
    .max_delay_us = 4000, \
    .accel_steps  = 200,  \
    .decel_steps  = 200,  \
}

/* ── Features ───────────────────────────────────────────────────────── */

typedef struct {
    bool position_tracking;
    bool calibration;
    bool patterns;
    bool acceleration;
} sky_stepper_features_t;

#define SKY_STEPPER_FEATURES_ALL() { \
    .position_tracking = true,  \
    .calibration       = true,  \
    .patterns          = true,  \
    .acceleration      = true,  \
}

/* ── Full motor config ──────────────────────────────────────────────── */

typedef struct {
    const char                *name;
    sky_stepper_gpio_config_t  gpio;
    sky_step_engine_config_t   engine;
    sky_crash_config_t         crash;
    sky_stepper_features_t     features;
    uint8_t                    task_priority;
    uint16_t                   task_stack_size;
} sky_stepper_config_t;

#define SKY_STEPPER_CONFIG_DEFAULT() {                \
    .name            = "motor",                       \
    .gpio            = { .pins = {0}, .invert_logic = false }, \
    .engine          = SKY_STEP_ENGINE_28BYJ48_DEFAULT(), \
    .crash           = SKY_CRASH_CONFIG_DEFAULT(),    \
    .features        = SKY_STEPPER_FEATURES_ALL(),    \
    .task_priority   = 5,                             \
    .task_stack_size = 4096,                          \
}

/* ── Status ─────────────────────────────────────────────────────────── */

typedef struct {
    int32_t             current_position;
    int32_t             target_position;
    int32_t             min_position;
    int32_t             max_position;
    bool                calibrated;
    sky_stepper_state_t state;
    sky_direction_t     direction;
} sky_stepper_status_t;

/* ── Calibration ────────────────────────────────────────────────────── */

typedef struct {
    int32_t  open_position;
    int32_t  closed_position;
    int32_t  total_steps;
    uint32_t timestamp;
} sky_stepper_calibration_t;

/* ── Hooks ──────────────────────────────────────────────────────────── */

typedef struct sky_stepper *sky_stepper_handle_t;

typedef enum {
    SKY_STEPPER_HOOK_BEFORE_MOVE,
    SKY_STEPPER_HOOK_AFTER_MOVE,
    SKY_STEPPER_HOOK_BEFORE_STOP,
    SKY_STEPPER_HOOK_AFTER_STOP,
    SKY_STEPPER_HOOK_ON_ERROR,
    SKY_STEPPER_HOOK_ON_POSITION,
} sky_stepper_hook_type_t;

typedef struct {
    sky_stepper_hook_type_t type;
    sky_stepper_handle_t    motor;
    int32_t                 target_steps;
    sky_direction_t         direction;
    bool                    cancel;
} sky_stepper_hook_event_t;

typedef void (*sky_stepper_hook_t)(sky_stepper_hook_event_t *event, void *ctx);

/* ── Patterns ───────────────────────────────────────────────────────── */

typedef enum {
    SKY_PATTERN_PRESET,
    SKY_PATTERN_SEQUENCE,
} sky_pattern_type_t;

typedef struct {
    uint8_t  target_percent;
    uint32_t delay_after_ms;
} sky_pattern_step_t;

#define SKY_PATTERN_MAX_STEPS 16
#define SKY_PATTERN_NAME_MAX  16

typedef struct {
    char               name[SKY_PATTERN_NAME_MAX];
    sky_pattern_type_t type;
    union {
        uint8_t preset_percent;
        struct {
            sky_pattern_step_t steps[SKY_PATTERN_MAX_STEPS];
            uint8_t            step_count;
        } sequence;
    };
} sky_pattern_t;

/* ── Event data structs (published on the event bus) ────────────────── */

typedef struct {
    const char     *name;
    sky_direction_t direction;
    int32_t         target;
} sky_stepper_started_event_t;

typedef struct {
    const char *name;
    int32_t     position;
    int8_t      percent;
    bool        moving;
    sky_direction_t direction;
} sky_stepper_position_event_t;

typedef struct {
    const char *name;
    int32_t     position;
    const char *reason;
} sky_stepper_stopped_event_t;

#ifdef __cplusplus
}
#endif
