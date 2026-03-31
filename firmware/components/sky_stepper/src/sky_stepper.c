#include "sky_stepper.h"
#include "sky_config.h"
#include "sky_event.h"
#include "sky_events.h"
#include "sky_error.h"
#include "sky_crash.h"
#include "sky_registry.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "sky_stepper";

/* ── Kconfig defaults ───────────────────────────────────────────────── */

#ifndef CONFIG_SKY_STEPPER_MAX_MOTORS
#define CONFIG_SKY_STEPPER_MAX_MOTORS 4
#endif
#ifndef CONFIG_SKY_STEPPER_MAX_HOOKS
#define CONFIG_SKY_STEPPER_MAX_HOOKS 8
#endif
#ifndef CONFIG_SKY_STEPPER_DEFAULT_TASK_PRIORITY
#define CONFIG_SKY_STEPPER_DEFAULT_TASK_PRIORITY 5
#endif
#ifndef CONFIG_SKY_STEPPER_DEFAULT_STACK_SIZE
#define CONFIG_SKY_STEPPER_DEFAULT_STACK_SIZE 4096
#endif
#ifndef CONFIG_SKY_STEPPER_CMD_QUEUE_SIZE
#define CONFIG_SKY_STEPPER_CMD_QUEUE_SIZE 8
#endif
#ifndef CONFIG_SKY_STEPPER_POSITION_EVENT_INTERVAL
#define CONFIG_SKY_STEPPER_POSITION_EVENT_INTERVAL 100
#endif
#ifndef CONFIG_SKY_STEPPER_CALIBRATION_TIMEOUT_S
#define CONFIG_SKY_STEPPER_CALIBRATION_TIMEOUT_S 120
#endif
#ifndef CONFIG_SKY_STEPPER_MAX_PATTERNS
#define CONFIG_SKY_STEPPER_MAX_PATTERNS 8
#endif

#define STEPPER_NAME_MAX 32

/* ── Command types (internal) ───────────────────────────────────────── */

typedef enum {
    CMD_MOVE,
    CMD_MOVE_TO,
    CMD_SET_HEIGHT,
    CMD_STOP,
    CMD_EMERGENCY_STOP,
    CMD_CALIBRATE_START,
    CMD_CALIBRATE_MARK_OPEN,
    CMD_CALIBRATE_MARK_CLOSED,
    CMD_CALIBRATE_CANCEL,
    CMD_SEQUENCE_PLAY,
} stepper_cmd_type_t;

typedef struct {
    stepper_cmd_type_t type;
    union {
        struct { uint32_t steps; sky_direction_t dir; } move;
        struct { int32_t position; }                     move_to;
        struct { uint8_t percent; }                      height;
        struct { sky_direction_t dir; }                   cal_start;
        struct { char name[SKY_PATTERN_NAME_MAX]; }      seq;
    };
} stepper_cmd_t;

/* ── Motor instance (opaque) ────────────────────────────────────────── */

struct sky_stepper {
    bool    used;
    uint8_t index;
    char    name[STEPPER_NAME_MAX];

    sky_stepper_config_t       config;
    sky_stepper_gpio_handle_t  gpio;
    sky_step_engine_handle_t   engine;

    sky_stepper_state_t state;
    sky_direction_t     direction;

    int32_t current_position;
    int32_t target_position;

    bool    calibrated;
    int32_t cal_open;
    int32_t cal_closed;
    int32_t cal_total;
    int32_t cal_step_counter;

    bool                      ramp_enabled;
    sky_stepper_ramp_config_t ramp;

    struct {
        sky_stepper_hook_t      handler;
        void                   *ctx;
        sky_stepper_hook_type_t type;
    } hooks[CONFIG_SKY_STEPPER_MAX_HOOKS];
    uint8_t hook_count;

    TaskHandle_t  task;
    QueueHandle_t cmd_queue;

    volatile bool emergency_stop_flag;
    volatile bool sequence_stop_flag;

    bool          recording;
    sky_pattern_t recording_pattern;
    int64_t       recording_last_us;

    sky_pattern_t patterns[CONFIG_SKY_STEPPER_MAX_PATTERNS];
    uint8_t       pattern_count;

    char nvs_ns[16];
};

/* ── Module state ───────────────────────────────────────────────────── */

static struct sky_stepper s_motors[CONFIG_SKY_STEPPER_MAX_MOTORS];
static SemaphoreHandle_t s_mutex       = NULL;
static bool              s_registered  = false;
static bool              s_started     = false;

/* ── Forward declarations ───────────────────────────────────────────── */

static void    stepper_motor_task(void *arg);
static void    stepper_safe_state(void *ctx);
static esp_err_t svc_init(void *cfg);
static esp_err_t svc_start(void);
static esp_err_t svc_stop(void);
static esp_err_t svc_deinit(void);

/* ═══════════════════════════════════════════════════════════════════════
 *  Helpers
 * ═══════════════════════════════════════════════════════════════════ */

static inline void step_delay_us(uint32_t us)
{
    if (us >= 2000) {
        vTaskDelay(pdMS_TO_TICKS(us / 1000));
    } else if (us >= 1000) {
        vTaskDelay(1);
    } else {
        int64_t target = esp_timer_get_time() + (int64_t)us;
        while (esp_timer_get_time() < target) { /* spin */ }
    }
}

static uint32_t calculate_delay(struct sky_stepper *m,
                                uint32_t step, uint32_t total)
{
    if (!m->ramp_enabled || total == 0) {
        return sky_step_engine_get_delay(m->engine);
    }
    sky_stepper_ramp_config_t *r = &m->ramp;
    uint32_t range     = r->max_delay_us - r->min_delay_us;
    uint32_t remaining = total - step - 1;

    if (step < r->accel_steps) {
        return r->max_delay_us - range * step / r->accel_steps;
    }
    if (remaining < r->decel_steps) {
        return r->max_delay_us - range * remaining / r->decel_steps;
    }
    return r->min_delay_us;
}

/* ── Hooks ──────────────────────────────────────────────────────────── */

static void run_hooks(struct sky_stepper *m, sky_stepper_hook_event_t *evt)
{
    for (uint8_t i = 0; i < m->hook_count; i++) {
        if (m->hooks[i].type == evt->type) {
            m->hooks[i].handler(evt, m->hooks[i].ctx);
            if (evt->cancel) return;
        }
    }
}

/* ── Event publishing ───────────────────────────────────────────────── */

static void publish_started(struct sky_stepper *m, sky_direction_t dir, int32_t target)
{
    sky_stepper_started_event_t e = { .name = m->name, .direction = dir, .target = target };
    sky_event_publish(SKY_EVENT_STEPPER_STARTED, &e, sizeof(e));
}

static void publish_stopped(struct sky_stepper *m, const char *reason)
{
    sky_stepper_stopped_event_t e = { .name = m->name, .position = m->current_position, .reason = reason };
    sky_event_publish(SKY_EVENT_STEPPER_STOPPED, &e, sizeof(e));
}

static void publish_position(struct sky_stepper *m)
{
    int8_t pct = -1;
    if (m->calibrated && m->cal_total > 0) {
        pct = (int8_t)((int64_t)m->current_position * 100 / m->cal_total);
    }
    sky_stepper_position_event_t e = {
        .name      = m->name,
        .position  = m->current_position,
        .percent   = pct,
        .moving    = m->state == SKY_STEPPER_STATE_MOVING,
        .direction = m->direction,
    };
    sky_event_publish(SKY_EVENT_STEPPER_POSITION, &e, sizeof(e));
}

static void publish_calibrated(struct sky_stepper *m)
{
    sky_stepper_calibration_t e = {
        .open_position   = 0,
        .closed_position = m->cal_total,
        .total_steps     = m->cal_total,
        .timestamp       = (uint32_t)(esp_timer_get_time() / 1000000),
    };
    sky_event_publish(SKY_EVENT_STEPPER_CALIBRATED, &e, sizeof(e));
}

/* ── NVS persistence ────────────────────────────────────────────────── */

static void save_position(struct sky_stepper *m)
{
    sky_config_set_i32(m->nvs_ns, "pos", m->current_position);
}

static void load_position(struct sky_stepper *m)
{
    int32_t v;
    if (sky_config_get_i32(m->nvs_ns, "pos", &v) == ESP_OK) {
        m->current_position = v;
    }
}

static void save_calibration(struct sky_stepper *m)
{
    sky_config_set_i32(m->nvs_ns, "co", m->cal_open);
    sky_config_set_i32(m->nvs_ns, "cc", m->cal_closed);
    sky_config_set_i32(m->nvs_ns, "ct", m->cal_total);
    sky_config_set_u32(m->nvs_ns, "cts",
                       (uint32_t)(esp_timer_get_time() / 1000000));
}

static void load_calibration(struct sky_stepper *m)
{
    if (!m->config.features.calibration) return;
    int32_t co, cc, ct;
    if (sky_config_get_i32(m->nvs_ns, "co", &co) == ESP_OK &&
        sky_config_get_i32(m->nvs_ns, "cc", &cc) == ESP_OK &&
        sky_config_get_i32(m->nvs_ns, "ct", &ct) == ESP_OK) {
        m->cal_open   = co;
        m->cal_closed = cc;
        m->cal_total  = ct;
        m->calibrated = true;
        ESP_LOGI(TAG, "[%s] loaded calibration: total=%ld", m->name, (long)ct);
    }
}

static void save_patterns(struct sky_stepper *m)
{
    sky_config_set_u32(m->nvs_ns, "pcnt", m->pattern_count);
    if (m->pattern_count > 0) {
        sky_config_set_blob(m->nvs_ns, "pdat",
                            m->patterns,
                            m->pattern_count * sizeof(sky_pattern_t));
    } else {
        sky_config_erase(m->nvs_ns, "pdat");
    }
}

static void load_patterns(struct sky_stepper *m)
{
    if (!m->config.features.patterns) return;
    uint32_t cnt = 0;
    if (sky_config_get_u32(m->nvs_ns, "pcnt", &cnt) == ESP_OK && cnt > 0) {
        if (cnt > CONFIG_SKY_STEPPER_MAX_PATTERNS) cnt = CONFIG_SKY_STEPPER_MAX_PATTERNS;
        size_t len = cnt * sizeof(sky_pattern_t);
        if (sky_config_get_blob(m->nvs_ns, "pdat", m->patterns, &len) == ESP_OK) {
            m->pattern_count = (uint8_t)cnt;
            ESP_LOGI(TAG, "[%s] loaded %u patterns", m->name, cnt);
        }
    }
}

/* ── Pattern helpers ────────────────────────────────────────────────── */

static sky_pattern_t *find_pattern(struct sky_stepper *m, const char *name)
{
    for (uint8_t i = 0; i < m->pattern_count; i++) {
        if (strncmp(m->patterns[i].name, name, SKY_PATTERN_NAME_MAX) == 0) {
            return &m->patterns[i];
        }
    }
    return NULL;
}

static bool wait_interruptible(struct sky_stepper *m, uint32_t ms)
{
    uint32_t waited = 0;
    while (waited < ms) {
        if (m->sequence_stop_flag || m->emergency_stop_flag) return false;
        uint32_t chunk = (ms - waited > 100) ? 100 : (ms - waited);
        vTaskDelay(pdMS_TO_TICKS(chunk));
        waited += chunk;
    }
    return true;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Exec functions (called from motor task context)
 * ═══════════════════════════════════════════════════════════════════ */

static void exec_move(struct sky_stepper *m, uint32_t steps, sky_direction_t dir)
{
    if (steps == 0) return;

    sky_stepper_hook_event_t hk = {
        .type         = SKY_STEPPER_HOOK_BEFORE_MOVE,
        .motor        = m,
        .target_steps = (int32_t)steps,
        .direction    = dir,
        .cancel       = false,
    };
    run_hooks(m, &hk);
    if (hk.cancel) return;

    m->state     = SKY_STEPPER_STATE_MOVING;
    m->direction = dir;
    int32_t delta = (dir == SKY_DIR_CW) ? 1 : -1;
    uint32_t total = steps;

    publish_started(m, dir, m->current_position + delta * (int32_t)steps);

    bool     stopping       = false;
    uint32_t decel_remaining = 0;
    uint32_t step_idx       = 0;
    uint32_t yield_counter  = 0;

    while (steps > 0 && !m->emergency_stop_flag) {
        stepper_cmd_t peek;
        if (xQueueReceive(m->cmd_queue, &peek, 0) == pdTRUE) {
            if (peek.type == CMD_EMERGENCY_STOP) {
                sky_stepper_gpio_all_off(m->gpio);
                m->state = SKY_STEPPER_STATE_IDLE;
                save_position(m);
                publish_stopped(m, "emergency");
                return;
            }
            if (peek.type == CMD_STOP && !stopping) {
                stopping = true;
                m->state = SKY_STEPPER_STATE_STOPPING;
                decel_remaining = m->ramp_enabled ? m->ramp.decel_steps : 0;
                if (decel_remaining == 0) break;
            }
        }

        uint32_t delay;
        if (stopping) {
            if (decel_remaining == 0) break;
            uint32_t range = m->ramp.max_delay_us - m->ramp.min_delay_us;
            delay = m->ramp.min_delay_us +
                    range * (m->ramp.decel_steps - decel_remaining) / m->ramp.decel_steps;
            decel_remaining--;
        } else {
            delay = calculate_delay(m, step_idx, total);
        }

        sky_step_engine_step(m->engine, dir);
        m->current_position += delta;
        step_idx++;
        steps--;

        if (step_idx % CONFIG_SKY_STEPPER_POSITION_EVENT_INTERVAL == 0) {
            publish_position(m);
        }

        step_delay_us(delay);

        if (delay < 2000 && ++yield_counter >= 500) {
            vTaskDelay(1);
            yield_counter = 0;
        }
    }

    sky_stepper_gpio_all_off(m->gpio);
    m->state = SKY_STEPPER_STATE_IDLE;
    save_position(m);
    publish_stopped(m, stopping ? "stopped" : "complete");

    hk.type = SKY_STEPPER_HOOK_AFTER_MOVE;
    hk.cancel = false;
    run_hooks(m, &hk);
}

static void exec_move_to(struct sky_stepper *m, int32_t target)
{
    int32_t diff = target - m->current_position;
    if (diff == 0) return;
    sky_direction_t dir = diff > 0 ? SKY_DIR_CW : SKY_DIR_CCW;
    uint32_t steps = (uint32_t)(diff > 0 ? diff : -diff);
    m->target_position = target;
    exec_move(m, steps, dir);
}

static void exec_set_height(struct sky_stepper *m, uint8_t percent)
{
    if (!m->calibrated || m->cal_total == 0) {
        ESP_LOGW(TAG, "[%s] not calibrated — set_height ignored", m->name);
        return;
    }
    if (percent > 100) percent = 100;

    if (m->recording &&
        m->recording_pattern.sequence.step_count < SKY_PATTERN_MAX_STEPS) {
        int64_t now = esp_timer_get_time();
        uint32_t delay_ms = 0;
        if (m->recording_last_us > 0) {
            delay_ms = (uint32_t)((now - m->recording_last_us) / 1000);
        }
        sky_pattern_step_t *s =
            &m->recording_pattern.sequence.steps[m->recording_pattern.sequence.step_count];
        s->target_percent = percent;
        s->delay_after_ms = delay_ms;
        m->recording_pattern.sequence.step_count++;
        m->recording_last_us = now;
    }

    int32_t target = (int32_t)((int64_t)m->cal_total * percent / 100);
    exec_move_to(m, target);
}

/* ── Calibration ────────────────────────────────────────────────────── */

static void exec_calibrate(struct sky_stepper *m, sky_direction_t dir)
{
    m->state            = SKY_STEPPER_STATE_CALIBRATING;
    m->direction        = dir;
    m->cal_step_counter = 0;
    m->cal_open         = 0;
    m->cal_closed       = 0;
    m->cal_total        = 0;
    m->calibrated       = false;

    int32_t delta = (dir == SKY_DIR_CW) ? 1 : -1;
    TickType_t timeout = pdMS_TO_TICKS(
        (uint32_t)CONFIG_SKY_STEPPER_CALIBRATION_TIMEOUT_S * 1000);
    TickType_t start = xTaskGetTickCount();
    uint32_t   delay = sky_step_engine_get_delay(m->engine);

    ESP_LOGI(TAG, "[%s] calibration started dir=%d", m->name, dir);

    while (m->state == SKY_STEPPER_STATE_CALIBRATING) {
        if (m->emergency_stop_flag) {
            m->emergency_stop_flag = false;
            break;
        }

        stepper_cmd_t cmd;
        if (xQueueReceive(m->cmd_queue, &cmd, 0) == pdTRUE) {
            switch (cmd.type) {
            case CMD_CALIBRATE_MARK_OPEN:
                m->cal_open = m->cal_step_counter;
                ESP_LOGI(TAG, "[%s] open @ %ld steps",
                         m->name, (long)m->cal_open);
                break;
            case CMD_CALIBRATE_MARK_CLOSED:
                m->cal_closed = m->cal_step_counter;
                m->cal_total  = m->cal_closed - m->cal_open;
                m->calibrated = true;
                m->current_position = m->cal_total;
                sky_stepper_gpio_all_off(m->gpio);
                m->state = SKY_STEPPER_STATE_IDLE;
                save_calibration(m);
                save_position(m);
                publish_calibrated(m);
                ESP_LOGI(TAG, "[%s] calibrated total=%ld",
                         m->name, (long)m->cal_total);
                return;
            case CMD_CALIBRATE_CANCEL:
            case CMD_EMERGENCY_STOP:
                ESP_LOGI(TAG, "[%s] calibration cancelled", m->name);
                goto cal_exit;
            default:
                break;
            }
        }

        if ((xTaskGetTickCount() - start) > timeout) {
            ESP_LOGW(TAG, "[%s] calibration timeout", m->name);
            break;
        }

        sky_step_engine_step(m->engine, dir);
        m->cal_step_counter += delta;
        step_delay_us(delay);
    }

cal_exit:
    sky_stepper_gpio_all_off(m->gpio);
    m->state = SKY_STEPPER_STATE_IDLE;
}

/* ── Sequence playback ──────────────────────────────────────────────── */

static void exec_sequence_play(struct sky_stepper *m, const char *name)
{
    sky_pattern_t *pat = find_pattern(m, name);
    if (!pat) {
        ESP_LOGW(TAG, "[%s] pattern '%s' not found", m->name, name);
        return;
    }

    if (pat->type == SKY_PATTERN_PRESET) {
        exec_set_height(m, pat->preset_percent);
        return;
    }

    m->sequence_stop_flag = false;
    ESP_LOGI(TAG, "[%s] playing sequence '%s' (%d steps)",
             m->name, name, pat->sequence.step_count);

    for (uint8_t i = 0; i < pat->sequence.step_count; i++) {
        if (m->sequence_stop_flag || m->emergency_stop_flag) break;

        exec_set_height(m, pat->sequence.steps[i].target_percent);

        if (pat->sequence.steps[i].delay_after_ms > 0) {
            if (!wait_interruptible(m, pat->sequence.steps[i].delay_after_ms))
                break;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Motor task
 * ═══════════════════════════════════════════════════════════════════ */

static void stepper_motor_task(void *arg)
{
    struct sky_stepper *m = (struct sky_stepper *)arg;
    stepper_cmd_t cmd;

    ESP_LOGI(TAG, "[%s] task started", m->name);

    while (1) {
        if (xQueueReceive(m->cmd_queue, &cmd, portMAX_DELAY) != pdTRUE)
            continue;

        switch (cmd.type) {
        case CMD_MOVE:
            exec_move(m, cmd.move.steps, cmd.move.dir);
            break;
        case CMD_MOVE_TO:
            exec_move_to(m, cmd.move_to.position);
            break;
        case CMD_SET_HEIGHT:
            exec_set_height(m, cmd.height.percent);
            break;
        case CMD_STOP:
            if (m->state != SKY_STEPPER_STATE_IDLE) {
                sky_stepper_gpio_all_off(m->gpio);
                m->state = SKY_STEPPER_STATE_IDLE;
                publish_stopped(m, "stopped");
            }
            break;
        case CMD_EMERGENCY_STOP:
            sky_stepper_gpio_all_off(m->gpio);
            m->state = SKY_STEPPER_STATE_IDLE;
            m->emergency_stop_flag = false;
            publish_stopped(m, "emergency");
            break;
        case CMD_CALIBRATE_START:
            exec_calibrate(m, cmd.cal_start.dir);
            break;
        case CMD_CALIBRATE_MARK_OPEN:
        case CMD_CALIBRATE_MARK_CLOSED:
        case CMD_CALIBRATE_CANCEL:
            break;
        case CMD_SEQUENCE_PLAY:
            exec_sequence_play(m, cmd.seq.name);
            break;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Service lifecycle
 * ═══════════════════════════════════════════════════════════════════ */

static void stepper_safe_state(void *ctx)
{
    (void)ctx;
    for (int i = 0; i < CONFIG_SKY_STEPPER_MAX_MOTORS; i++) {
        if (s_motors[i].used && s_motors[i].gpio) {
            sky_stepper_gpio_all_off(s_motors[i].gpio);
        }
    }
}

static esp_err_t svc_init(void *cfg)
{
    (void)cfg;
    return ESP_OK;
}

static esp_err_t svc_start(void)
{
    for (int i = 0; i < CONFIG_SKY_STEPPER_MAX_MOTORS; i++) {
        struct sky_stepper *m = &s_motors[i];
        if (!m->used || m->task) continue;

        m->cmd_queue = xQueueCreate(CONFIG_SKY_STEPPER_CMD_QUEUE_SIZE,
                                    sizeof(stepper_cmd_t));
        if (!m->cmd_queue) return ESP_ERR_NO_MEM;

        uint16_t stack = m->config.task_stack_size
                             ? m->config.task_stack_size
                             : CONFIG_SKY_STEPPER_DEFAULT_STACK_SIZE;
        uint8_t  prio  = m->config.task_priority
                             ? m->config.task_priority
                             : CONFIG_SKY_STEPPER_DEFAULT_TASK_PRIORITY;

        BaseType_t ok = xTaskCreate(stepper_motor_task, m->name,
                                    stack, m, prio, &m->task);
        if (ok != pdPASS) return ESP_ERR_NO_MEM;
    }
    s_started = true;
    return ESP_OK;
}

static esp_err_t svc_stop(void)
{
    for (int i = CONFIG_SKY_STEPPER_MAX_MOTORS - 1; i >= 0; i--) {
        struct sky_stepper *m = &s_motors[i];
        if (!m->used || !m->task) continue;
        sky_stepper_gpio_all_off(m->gpio);
        vTaskDelete(m->task);
        m->task = NULL;
        if (m->cmd_queue) {
            vQueueDelete(m->cmd_queue);
            m->cmd_queue = NULL;
        }
    }
    s_started = false;
    return ESP_OK;
}

static esp_err_t svc_deinit(void)
{
    svc_stop();
    for (int i = 0; i < CONFIG_SKY_STEPPER_MAX_MOTORS; i++) {
        struct sky_stepper *m = &s_motors[i];
        if (!m->used) continue;
        if (m->engine) { sky_step_engine_deinit(m->engine); m->engine = NULL; }
        if (m->gpio)   { sky_stepper_gpio_deinit(m->gpio);  m->gpio   = NULL; }
        m->used = false;
    }
    if (s_mutex) { vSemaphoreDelete(s_mutex); s_mutex = NULL; }
    s_registered = false;
    return ESP_OK;
}

/* ── Auto-registration ──────────────────────────────────────────────── */

static esp_err_t ensure_registered(void)
{
    if (s_registered) return ESP_OK;

    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) return ESP_ERR_NO_MEM;

    memset(s_motors, 0, sizeof(s_motors));

    static const sky_service_lifecycle_t lc = {
        .init   = svc_init,
        .start  = svc_start,
        .stop   = svc_stop,
        .deinit = svc_deinit,
    };

    esp_err_t err = sky_registry_register("sky_stepper", &lc, NULL);
    if (err != ESP_OK) return err;

    sky_crash_config_t cc = {
        .on_task_watchdog   = SKY_CRASH_ACTION_SAFE_STATE,
        .on_system_panic    = SKY_CRASH_ACTION_SAFE_STATE,
        .safe_state_handler = stepper_safe_state,
    };
    sky_crash_register("sky_stepper", &cc);

    s_registered = true;
    return ESP_OK;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Registration API
 * ═══════════════════════════════════════════════════════════════════ */

esp_err_t sky_stepper_register(const sky_stepper_config_t *config,
                               sky_stepper_handle_t *handle)
{
    if (!config || !handle) return ESP_ERR_INVALID_ARG;

    esp_err_t err = ensure_registered();
    if (err != ESP_OK) return err;

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    struct sky_stepper *slot = NULL;
    for (int i = 0; i < CONFIG_SKY_STEPPER_MAX_MOTORS; i++) {
        if (!s_motors[i].used) { slot = &s_motors[i]; break; }
    }
    if (!slot) {
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "motor registry full");
        return ESP_ERR_NO_MEM;
    }

    memset(slot, 0, sizeof(*slot));
    slot->used  = true;
    slot->index = (uint8_t)(slot - s_motors);
    strncpy(slot->name, config->name ? config->name : "motor",
            STEPPER_NAME_MAX - 1);
    slot->config = *config;
    slot->state  = SKY_STEPPER_STATE_IDLE;

    snprintf(slot->nvs_ns, sizeof(slot->nvs_ns), "stp%d", slot->index);

    err = sky_stepper_gpio_init(&config->gpio, &slot->gpio);
    if (err != ESP_OK) {
        slot->used = false;
        xSemaphoreGive(s_mutex);
        return err;
    }

    err = sky_step_engine_init(&config->engine, slot->gpio, &slot->engine);
    if (err != ESP_OK) {
        sky_stepper_gpio_deinit(slot->gpio);
        slot->used = false;
        xSemaphoreGive(s_mutex);
        return err;
    }

    load_position(slot);
    load_calibration(slot);
    load_patterns(slot);

    if (s_started && !slot->task) {
        slot->cmd_queue = xQueueCreate(CONFIG_SKY_STEPPER_CMD_QUEUE_SIZE,
                                       sizeof(stepper_cmd_t));
        uint16_t stack = config->task_stack_size
                             ? config->task_stack_size
                             : CONFIG_SKY_STEPPER_DEFAULT_STACK_SIZE;
        uint8_t  prio  = config->task_priority
                             ? config->task_priority
                             : CONFIG_SKY_STEPPER_DEFAULT_TASK_PRIORITY;
        xTaskCreate(stepper_motor_task, slot->name, stack, slot, prio, &slot->task);
    }

    *handle = slot;
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "registered motor '%s' (idx=%d)", slot->name, slot->index);
    return ESP_OK;
}

sky_stepper_handle_t sky_stepper_get(const char *name)
{
    if (!name || !s_registered) return NULL;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (int i = 0; i < CONFIG_SKY_STEPPER_MAX_MOTORS; i++) {
        if (s_motors[i].used && strcmp(s_motors[i].name, name) == 0) {
            xSemaphoreGive(s_mutex);
            return &s_motors[i];
        }
    }
    xSemaphoreGive(s_mutex);
    return NULL;
}

esp_err_t sky_stepper_list(sky_stepper_handle_t *handles,
                           uint8_t *count, uint8_t max)
{
    if (!handles || !count) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    uint8_t n = 0;
    for (int i = 0; i < CONFIG_SKY_STEPPER_MAX_MOTORS && n < max; i++) {
        if (s_motors[i].used) handles[n++] = &s_motors[i];
    }
    *count = n;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t sky_stepper_unregister(sky_stepper_handle_t handle)
{
    if (!handle || !handle->used) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (handle->task) {
        sky_stepper_gpio_all_off(handle->gpio);
        vTaskDelete(handle->task);
        handle->task = NULL;
    }
    if (handle->cmd_queue) {
        vQueueDelete(handle->cmd_queue);
        handle->cmd_queue = NULL;
    }
    if (handle->engine) { sky_step_engine_deinit(handle->engine); handle->engine = NULL; }
    if (handle->gpio)   { sky_stepper_gpio_deinit(handle->gpio);  handle->gpio   = NULL; }
    handle->used = false;
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "unregistered '%s'", handle->name);
    return ESP_OK;
}

esp_err_t sky_stepper_set_name(sky_stepper_handle_t handle, const char *name)
{
    if (!handle || !name) return ESP_ERR_INVALID_ARG;
    strncpy(handle->name, name, STEPPER_NAME_MAX - 1);
    handle->name[STEPPER_NAME_MAX - 1] = '\0';
    return ESP_OK;
}

const char *sky_stepper_get_name(sky_stepper_handle_t handle)
{
    return handle ? handle->name : NULL;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Public API — send commands
 * ═══════════════════════════════════════════════════════════════════ */

static esp_err_t send_cmd(sky_stepper_handle_t h, const stepper_cmd_t *cmd)
{
    if (!h || !h->used || !h->cmd_queue) return ESP_ERR_INVALID_STATE;
    if (xQueueSend(h->cmd_queue, cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t sky_stepper_move(sky_stepper_handle_t handle,
                           uint32_t steps, sky_direction_t direction)
{
    stepper_cmd_t c = { .type = CMD_MOVE, .move = { steps, direction } };
    return send_cmd(handle, &c);
}

esp_err_t sky_stepper_move_to(sky_stepper_handle_t handle, int32_t target)
{
    stepper_cmd_t c = { .type = CMD_MOVE_TO, .move_to = { target } };
    return send_cmd(handle, &c);
}

esp_err_t sky_stepper_set_height(sky_stepper_handle_t handle, uint8_t percent)
{
    stepper_cmd_t c = { .type = CMD_SET_HEIGHT, .height = { percent } };
    return send_cmd(handle, &c);
}

esp_err_t sky_stepper_stop(sky_stepper_handle_t handle)
{
    stepper_cmd_t c = { .type = CMD_STOP };
    return send_cmd(handle, &c);
}

esp_err_t sky_stepper_emergency_stop(sky_stepper_handle_t handle)
{
    if (!handle) return ESP_ERR_INVALID_ARG;
    handle->emergency_stop_flag = true;
    if (handle->gpio) sky_stepper_gpio_all_off(handle->gpio);
    stepper_cmd_t c = { .type = CMD_EMERGENCY_STOP };
    send_cmd(handle, &c);
    return ESP_OK;
}

/* ── Status ─────────────────────────────────────────────────────────── */

sky_stepper_state_t sky_stepper_get_state(sky_stepper_handle_t handle)
{
    return handle ? handle->state : SKY_STEPPER_STATE_IDLE;
}

int32_t sky_stepper_get_position(sky_stepper_handle_t handle)
{
    return handle ? handle->current_position : 0;
}

int8_t sky_stepper_get_height_percent(sky_stepper_handle_t handle)
{
    if (!handle || !handle->calibrated || handle->cal_total == 0) return -1;
    return (int8_t)((int64_t)handle->current_position * 100 / handle->cal_total);
}

esp_err_t sky_stepper_get_status(sky_stepper_handle_t handle,
                                 sky_stepper_status_t *status)
{
    if (!handle || !status) return ESP_ERR_INVALID_ARG;
    status->current_position = handle->current_position;
    status->target_position  = handle->target_position;
    status->min_position     = 0;
    status->max_position     = handle->calibrated ? handle->cal_total : 0;
    status->calibrated       = handle->calibrated;
    status->state            = handle->state;
    status->direction        = handle->direction;
    return ESP_OK;
}

/* ── Ramp ───────────────────────────────────────────────────────────── */

esp_err_t sky_stepper_set_ramp(sky_stepper_handle_t handle,
                               const sky_stepper_ramp_config_t *ramp)
{
    if (!handle || !ramp) return ESP_ERR_INVALID_ARG;
    handle->ramp         = *ramp;
    handle->ramp_enabled = true;
    return ESP_OK;
}

/* ── Hooks ──────────────────────────────────────────────────────────── */

esp_err_t sky_stepper_add_hook(sky_stepper_handle_t handle,
                               sky_stepper_hook_type_t type,
                               sky_stepper_hook_t hook, void *ctx)
{
    if (!handle || !hook) return ESP_ERR_INVALID_ARG;
    if (handle->hook_count >= CONFIG_SKY_STEPPER_MAX_HOOKS) return ESP_ERR_NO_MEM;
    uint8_t idx = handle->hook_count++;
    handle->hooks[idx].type    = type;
    handle->hooks[idx].handler = hook;
    handle->hooks[idx].ctx     = ctx;
    return ESP_OK;
}

esp_err_t sky_stepper_remove_hook(sky_stepper_handle_t handle,
                                  sky_stepper_hook_type_t type,
                                  sky_stepper_hook_t hook)
{
    if (!handle || !hook) return ESP_ERR_INVALID_ARG;
    for (uint8_t i = 0; i < handle->hook_count; i++) {
        if (handle->hooks[i].type == type && handle->hooks[i].handler == hook) {
            memmove(&handle->hooks[i], &handle->hooks[i + 1],
                    (handle->hook_count - i - 1) * sizeof(handle->hooks[0]));
            handle->hook_count--;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

/* ── Calibration ────────────────────────────────────────────────────── */

esp_err_t sky_stepper_calibrate_start(sky_stepper_handle_t handle,
                                      sky_direction_t direction)
{
    stepper_cmd_t c = { .type = CMD_CALIBRATE_START, .cal_start = { direction } };
    return send_cmd(handle, &c);
}

esp_err_t sky_stepper_calibrate_mark_open(sky_stepper_handle_t handle)
{
    stepper_cmd_t c = { .type = CMD_CALIBRATE_MARK_OPEN };
    return send_cmd(handle, &c);
}

esp_err_t sky_stepper_calibrate_mark_closed(sky_stepper_handle_t handle)
{
    stepper_cmd_t c = { .type = CMD_CALIBRATE_MARK_CLOSED };
    return send_cmd(handle, &c);
}

esp_err_t sky_stepper_calibrate_cancel(sky_stepper_handle_t handle)
{
    stepper_cmd_t c = { .type = CMD_CALIBRATE_CANCEL };
    return send_cmd(handle, &c);
}

bool sky_stepper_is_calibrated(sky_stepper_handle_t handle)
{
    return handle ? handle->calibrated : false;
}

esp_err_t sky_stepper_get_calibration(sky_stepper_handle_t handle,
                                      sky_stepper_calibration_t *cal)
{
    if (!handle || !cal) return ESP_ERR_INVALID_ARG;
    if (!handle->calibrated) return ESP_ERR_INVALID_STATE;
    cal->open_position   = 0;
    cal->closed_position = handle->cal_total;
    cal->total_steps     = handle->cal_total;
    uint32_t ts = 0;
    sky_config_get_u32(handle->nvs_ns, "cts", &ts);
    cal->timestamp = ts;
    return ESP_OK;
}

esp_err_t sky_stepper_clear_calibration(sky_stepper_handle_t handle)
{
    if (!handle) return ESP_ERR_INVALID_ARG;
    handle->calibrated = false;
    handle->cal_open   = 0;
    handle->cal_closed = 0;
    handle->cal_total  = 0;
    sky_config_erase(handle->nvs_ns, "co");
    sky_config_erase(handle->nvs_ns, "cc");
    sky_config_erase(handle->nvs_ns, "ct");
    sky_config_erase(handle->nvs_ns, "cts");
    return ESP_OK;
}

/* ── Patterns — presets ─────────────────────────────────────────────── */

esp_err_t sky_stepper_preset_save(sky_stepper_handle_t handle,
                                  const char *name, uint8_t percent)
{
    if (!handle || !name) return ESP_ERR_INVALID_ARG;

    sky_pattern_t *existing = find_pattern(handle, name);
    if (existing) {
        existing->type           = SKY_PATTERN_PRESET;
        existing->preset_percent = percent;
        save_patterns(handle);
        return ESP_OK;
    }

    if (handle->pattern_count >= CONFIG_SKY_STEPPER_MAX_PATTERNS)
        return ESP_ERR_NO_MEM;

    sky_pattern_t *p = &handle->patterns[handle->pattern_count++];
    memset(p, 0, sizeof(*p));
    strncpy(p->name, name, SKY_PATTERN_NAME_MAX - 1);
    p->type           = SKY_PATTERN_PRESET;
    p->preset_percent = percent;
    save_patterns(handle);
    return ESP_OK;
}

esp_err_t sky_stepper_preset_apply(sky_stepper_handle_t handle,
                                   const char *name)
{
    if (!handle || !name) return ESP_ERR_INVALID_ARG;
    sky_pattern_t *p = find_pattern(handle, name);
    if (!p || p->type != SKY_PATTERN_PRESET) return ESP_ERR_NOT_FOUND;
    return sky_stepper_set_height(handle, p->preset_percent);
}

esp_err_t sky_stepper_preset_delete(sky_stepper_handle_t handle,
                                    const char *name)
{
    if (!handle || !name) return ESP_ERR_INVALID_ARG;
    for (uint8_t i = 0; i < handle->pattern_count; i++) {
        if (strncmp(handle->patterns[i].name, name, SKY_PATTERN_NAME_MAX) == 0) {
            memmove(&handle->patterns[i], &handle->patterns[i + 1],
                    (handle->pattern_count - i - 1) * sizeof(sky_pattern_t));
            handle->pattern_count--;
            save_patterns(handle);
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

/* ── Patterns — sequences ───────────────────────────────────────────── */

esp_err_t sky_stepper_sequence_record_start(sky_stepper_handle_t handle,
                                            const char *name)
{
    if (!handle || !name) return ESP_ERR_INVALID_ARG;
    if (handle->recording) return ESP_ERR_INVALID_STATE;
    memset(&handle->recording_pattern, 0, sizeof(handle->recording_pattern));
    strncpy(handle->recording_pattern.name, name, SKY_PATTERN_NAME_MAX - 1);
    handle->recording_pattern.type = SKY_PATTERN_SEQUENCE;
    handle->recording          = true;
    handle->recording_last_us  = esp_timer_get_time();
    return ESP_OK;
}

esp_err_t sky_stepper_sequence_record_step(sky_stepper_handle_t handle,
                                           uint8_t target_percent,
                                           uint32_t delay_after_ms)
{
    if (!handle || !handle->recording) return ESP_ERR_INVALID_STATE;
    if (handle->recording_pattern.sequence.step_count >= SKY_PATTERN_MAX_STEPS)
        return ESP_ERR_NO_MEM;
    sky_pattern_step_t *s = &handle->recording_pattern.sequence
                                 .steps[handle->recording_pattern.sequence.step_count++];
    s->target_percent = target_percent;
    s->delay_after_ms = delay_after_ms;
    return ESP_OK;
}

esp_err_t sky_stepper_sequence_record_stop(sky_stepper_handle_t handle)
{
    if (!handle || !handle->recording) return ESP_ERR_INVALID_STATE;
    handle->recording = false;

    sky_pattern_t *existing = find_pattern(handle, handle->recording_pattern.name);
    if (existing) {
        *existing = handle->recording_pattern;
    } else {
        if (handle->pattern_count >= CONFIG_SKY_STEPPER_MAX_PATTERNS)
            return ESP_ERR_NO_MEM;
        handle->patterns[handle->pattern_count++] = handle->recording_pattern;
    }
    save_patterns(handle);
    ESP_LOGI(TAG, "[%s] saved sequence '%s' (%d steps)",
             handle->name, handle->recording_pattern.name,
             handle->recording_pattern.sequence.step_count);
    return ESP_OK;
}

esp_err_t sky_stepper_sequence_play(sky_stepper_handle_t handle,
                                    const char *name)
{
    if (!handle || !name) return ESP_ERR_INVALID_ARG;
    stepper_cmd_t c = { .type = CMD_SEQUENCE_PLAY };
    strncpy(c.seq.name, name, SKY_PATTERN_NAME_MAX - 1);
    c.seq.name[SKY_PATTERN_NAME_MAX - 1] = '\0';
    return send_cmd(handle, &c);
}

esp_err_t sky_stepper_sequence_stop(sky_stepper_handle_t handle)
{
    if (!handle) return ESP_ERR_INVALID_ARG;
    handle->sequence_stop_flag = true;
    return ESP_OK;
}

/* ── Patterns — management ──────────────────────────────────────────── */

esp_err_t sky_stepper_pattern_list(sky_stepper_handle_t handle,
                                   sky_pattern_t *patterns,
                                   uint8_t *count, uint8_t max)
{
    if (!handle || !patterns || !count) return ESP_ERR_INVALID_ARG;
    uint8_t n = handle->pattern_count < max ? handle->pattern_count : max;
    memcpy(patterns, handle->patterns, n * sizeof(sky_pattern_t));
    *count = n;
    return ESP_OK;
}

esp_err_t sky_stepper_pattern_export(sky_stepper_handle_t handle,
                                     const char *name, sky_pattern_t *out)
{
    if (!handle || !name || !out) return ESP_ERR_INVALID_ARG;
    sky_pattern_t *p = find_pattern(handle, name);
    if (!p) return ESP_ERR_NOT_FOUND;
    *out = *p;
    return ESP_OK;
}

esp_err_t sky_stepper_pattern_import(sky_stepper_handle_t handle,
                                     const sky_pattern_t *pattern)
{
    if (!handle || !pattern) return ESP_ERR_INVALID_ARG;
    sky_pattern_t *existing = find_pattern(handle, pattern->name);
    if (existing) {
        *existing = *pattern;
    } else {
        if (handle->pattern_count >= CONFIG_SKY_STEPPER_MAX_PATTERNS)
            return ESP_ERR_NO_MEM;
        handle->patterns[handle->pattern_count++] = *pattern;
    }
    save_patterns(handle);
    return ESP_OK;
}
