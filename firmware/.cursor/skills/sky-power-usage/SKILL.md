---
name: sky-power-usage
description: >-
  Manage battery monitoring and sleep modes with sky_power.
  Use when configuring battery ADC, reading charge level, setting up sleep modes,
  using wakelocks, or entering deep sleep.
---

# Sky Power & Sleep Management

## Part 1: Battery Monitoring

### Initialize

```c
#include "sky_power.h"

sky_power_config_t cfg = SKY_POWER_CONFIG_LIPO_DEFAULT();
// Defaults: GPIO 34, atten 12dB, divider 2.0, thresholds 20%/5%, 60s sampling
ESP_ERROR_CHECK(sky_power_init(&cfg));
```

### Read Battery

```c
uint8_t pct   = sky_power_get_percent();   // 0–100 (LiPo curve)
float volts   = sky_power_get_voltage();   // after voltage divider correction
bool low      = sky_power_is_low();        // <= threshold_low (20%)
bool critical = sky_power_is_critical();   // <= threshold_critical (5%)

sky_power_status_t status;
sky_power_get_status(&status);
// status.voltage, status.percent, status.charging, status.low, status.critical
```

### Battery Events

| Event | Data | Trigger |
|-------|------|---------|
| `SKY_EVENT_BATTERY_LOW` | `{percent, voltage}` | Drops below low threshold |
| `SKY_EVENT_BATTERY_CRITICAL` | `{percent, voltage}` | Drops below critical threshold |
| `SKY_EVENT_BATTERY_OK` | `{percent, voltage}` | Recovers above low threshold |
| `SKY_EVENT_BATTERY_CHARGING` | `{percent, voltage}` | Voltage increase >0.05V |

### Motor Guard Pattern

Block motor movement when battery is critical:

```c
static void battery_guard(sky_stepper_hook_event_t *event, void *ctx) {
    if (sky_power_is_critical()) event->cancel = true;
}
sky_stepper_add_hook(motor, SKY_STEPPER_HOOK_BEFORE_MOVE, battery_guard, NULL);
```

### ADC Notes
- Use ADC1 pins only (GPIO 32–39); ADC2 blocked by Wi-Fi
- GPIO 34–39 are input-only (ideal for battery sensing)
- Voltage divider: R1=R2 (ratio 2.0) maps 4.2V → 2.1V within ADC range
- 16-sample averaging + calibration (curve/line fitting) for noise reduction

---

## Part 2: Sleep Management

### Initialize

```c
#include "sky_sleep.h"

sky_sleep_config_t cfg = SKY_SLEEP_CONFIG_DEFAULT();
cfg.wake_gpio   = GPIO_NUM_35;   // wake button
cfg.wake_on_high = true;         // wake on rising edge
ESP_ERROR_CHECK(sky_sleep_init(&cfg));
```

### Power Modes

| Mode | Function | Consumption |
|------|----------|-------------|
| `SKY_POWER_MODE_ACTIVE` | Everything on | ~150–250mA |
| `SKY_POWER_MODE_MODEM_SLEEP` | Wi-Fi power save | ~20mA |
| `SKY_POWER_MODE_LIGHT_SLEEP` | Aggressive Wi-Fi PS | ~800µA |
| `SKY_POWER_MODE_DEEP_SLEEP` | RTC only | ~10µA |

```c
sky_sleep_set_mode(SKY_POWER_MODE_MODEM_SLEEP);
sky_power_mode_t current = sky_sleep_get_mode();
```

### Wakelocks

Prevent sleep during critical operations:

```c
sky_wakelock_t lock;
sky_sleep_acquire_wakelock("stepper_moving", &lock);
// ... motor movement ...
sky_sleep_release_wakelock(lock);
```

- Max 8 simultaneous wakelocks
- Acquiring wakelock → cancels idle timer, forces ACTIVE mode
- Releasing last wakelock → restarts idle timer

### Deep Sleep

```c
sky_sleep_enter_deep_sleep(30000);  // sleep 30s, then timer wake
// 0 = sleep until GPIO wake only
```

Before deep sleep: publishes `SKY_EVENT_SLEEP_ENTER`, calls `sky_core_stop()`.
Refused if wakelocks are active.

### Wake Reason

```c
sky_wake_reason_t reason = sky_sleep_get_wake_reason();
// SKY_WAKE_REASON_POWERON, _TIMER, _GPIO, _UNKNOWN
```

### Idle Timer Behavior
1. All wakelocks released → idle timer starts (`idle_timeout_ms`, default 30s)
2. Timer fires → enters configured `idle_mode` (default: modem sleep)
3. New wakelock acquired → timer cancelled, back to ACTIVE

### Sleep Events

| Event | Data |
|-------|------|
| `SKY_EVENT_SLEEP_ENTER` | `{mode, duration_ms}` |
| `SKY_EVENT_SLEEP_EXIT` | `{reason, prev_mode}` |
| `SKY_EVENT_POWER_MODE_CHANGED` | `{old_mode, new_mode}` |
