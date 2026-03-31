# Plan 05 — Luxia : Phase 1 — Firmware Assembly

## Objectif

Assembler tous les packages Sky pour créer le firmware **Luxia** : le produit final qui contrôle des stores/rideaux. Cette phase connecte les composants entre eux et crée la configuration spécifique au produit.

## Prérequis

- Tous les plans précédents (01 à 04) complétés

## Architecture finale

```
┌────────────────────────────────────────────────────────┐
│                    Luxia Firmware                    │
│                      (main.c)                           │
├────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  sky         │  │  sky          │  │  sky         │  │
│  │  _stepper    │  │  _matter      │  │  _power     │  │
│  │  (moteur)    │  │  (protocole)  │  │  (batterie) │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
│         │                 │                  │          │
│  ┌──────▼─────────────────▼──────────────────▼───────┐ │
│  │                  sky_core                      │ │
│  │  (event bus, registry, config, crash safety)       │ │
│  └──────────────────────┬────────────────────────────┘ │
│                         │                               │
│  ┌──────────────────────▼────────────────────────────┐ │
│  │       sky_wifi        sky_ota            │ │
│  └───────────────────────────────────────────────────┘ │
│                                                         │
└────────────────────────────────────────────────────────┘
```

## `main.c` — Point d'entrée

```c
#include "sky_core.h"
#include "sky_stepper.h"
#include "sky_wifi.h"
#include "sky_matter.h"
#include "sky_matter_wc.h"
#include "sky_power.h"
#include "sky_ota.h"

static const char *TAG = "sky-motor";

static sky_stepper_handle_t motor;
static sky_matter_wc_handle_t wc_handle;

// === Matter ↔ Stepper bridge ===

static esp_err_t on_wc_move_to_percent(uint8_t percent, void *ctx) {
    return sky_stepper_set_height(motor, percent);
}

static esp_err_t on_wc_open(void *ctx) {
    return sky_stepper_set_height(motor, 0);
}

static esp_err_t on_wc_close(void *ctx) {
    return sky_stepper_set_height(motor, 100);
}

static esp_err_t on_wc_stop(void *ctx) {
    return sky_stepper_stop(motor);
}

// Écouter les changements de position pour notifier Matter
static void on_position_event(const sky_event_t *event, void *ctx) {
    sky_stepper_position_event_t *pos = event->data;
    sky_matter_wc_update_position(wc_handle, pos->percent);

    if (pos->moving) {
        sky_matter_wc_update_status(wc_handle,
            pos->direction == SKY_DIR_CW
                ? SKY_MATTER_WC_STATUS_CLOSING
                : SKY_MATTER_WC_STATUS_OPENING);
    } else {
        sky_matter_wc_update_status(wc_handle,
            SKY_MATTER_WC_STATUS_STOPPED);
    }
}

// === OTA Self-test ===

static void sky_motor_self_test(bool *passed, void *ctx) {
    *passed = true;

    if (!sky_wifi_is_connected()) {
        ESP_LOGE(TAG, "Self-test FAILED: Wi-Fi");
        *passed = false;
        return;
    }

    if (sky_stepper_get("store") == NULL) {
        ESP_LOGE(TAG, "Self-test FAILED: motor not found");
        *passed = false;
        return;
    }

    ESP_LOGI(TAG, "Self-test PASSED");
}

// === Main ===

void app_main(void) {
    ESP_LOGI(TAG, "Luxia v%s starting...", FIRMWARE_VERSION);

    // 1. Core framework
    sky_core_init();

    // 2. Wi-Fi
    sky_wifi_config_t wifi_cfg = SKY_WIFI_CONFIG_DEFAULT();
    wifi_cfg.hostname = "sky-motor";
    sky_wifi_init(&wifi_cfg);

    // 3. Stepper motor
    sky_stepper_config_t motor_cfg = SKY_STEPPER_CONFIG_DEFAULT();
    motor_cfg.name = "store";
    motor_cfg.gpio.pins[0] = GPIO_NUM_17;
    motor_cfg.gpio.pins[1] = GPIO_NUM_16;
    motor_cfg.gpio.pins[2] = GPIO_NUM_4;
    motor_cfg.gpio.pins[3] = GPIO_NUM_0;
    sky_stepper_register(&motor_cfg, &motor);

    // 4. Matter (Window Covering)
    sky_matter_config_t matter_cfg = SKY_MATTER_CONFIG_DEV_DEFAULT();
    matter_cfg.device_name = "Sky Store";
    sky_matter_init(&matter_cfg);

    sky_matter_wc_callbacks_t wc_cb = {
        .on_move_to_percent = on_wc_move_to_percent,
        .on_open = on_wc_open,
        .on_close = on_wc_close,
        .on_stop = on_wc_stop,
        .ctx = NULL,
    };
    sky_matter_add_window_covering(&wc_cb, &wc_handle);

    // 5. Power management
    sky_power_config_t power_cfg = SKY_POWER_CONFIG_LIPO_DEFAULT();
    sky_power_init(&power_cfg);

    sky_sleep_config_t sleep_cfg = SKY_SLEEP_CONFIG_DEFAULT();
    sleep_cfg.idle_mode = SKY_POWER_MODE_MODEM_SLEEP;
    sky_sleep_init(&sleep_cfg);

    // 6. OTA
    sky_ota_config_t ota_cfg = SKY_OTA_CONFIG_DEFAULT();
    ota_cfg.self_test = sky_motor_self_test;
    sky_ota_init(&ota_cfg);

    // 7. Event listeners
    sky_event_subscribe(SKY_EVENT_STEPPER_POSITION,
                              on_position_event, NULL);

    // 8. Start everything
    sky_core_start();

    ESP_LOGI(TAG, "Luxia ready.");
}
```

## Ordre de démarrage

Le `sky_core_start()` initialise et démarre les services dans cet ordre :

```
1. sky_core     (event bus, registry, config, crash)
2. sky_power    (monitoring batterie — doit être dispo tôt)
3. sky_wifi     (connexion réseau)
4. sky_stepper  (init GPIO, créer tasks moteur)
5. sky_matter   (attend WIFI_CONNECTED, puis init Matter stack)
6. sky_ota      (attend MATTER_READY, puis check updates)
7. sky_sleep    (démarre le timer idle)
```

## Configuration GPIO — Luxia PCB

| Fonction | GPIO | Notes |
|----------|------|-------|
| Stepper IN1 | GPIO 17 | Bobine A+ |
| Stepper IN2 | GPIO 16 | Bobine B+ |
| Stepper IN3 | GPIO 4 | Bobine A- |
| Stepper IN4 | GPIO 0 | Bobine B- (attention: GPIO 0 = boot mode) |
| Battery ADC | GPIO 34 | Input-only, ADC1 |
| Wake button | GPIO 35 | Input-only, pull-up externe |
| Status LED | GPIO 2 | LED intégrée (optionnel) |

**Attention GPIO 0** : GPIO 0 est utilisé pour le boot mode (LOW = download mode). Doit être HIGH au boot. Le pull-up du ULN2003 peut interférer. À surveiller et possiblement remapper sur un autre GPIO si problèmes.

## Partition Table finale

```csv
# Name,    Type, SubType,  Offset,   Size,    Flags
nvs,       data, nvs,      0x9000,   0x6000,
otadata,   data, ota,      0xf000,   0x2000,
phy_init,  data, phy,      0x11000,  0x1000,
fctry,     data, nvs,      0x12000,  0x6000,
ota_0,     app,  ota_0,    0x20000,  0x1E0000,
ota_1,     app,  ota_1,    0x200000, 0x1E0000,
```

## Tâches

- [ ] Créer le `main.c` final avec le wiring de tous les composants
- [ ] Configurer la partition table
- [ ] Connecter Matter callbacks ↔ stepper commands
- [ ] Connecter stepper events ↔ Matter attribute updates
- [ ] Connecter power events ↔ stepper hooks (block on critical)
- [ ] Configurer le self-test OTA spécifique au Luxia
- [ ] Définir la configuration GPIO finale du PCB
- [ ] Créer le `sdkconfig.defaults` avec toutes les options
- [ ] Tester le flux complet : commissioning → commande → mouvement → feedback

## Critères de validation

- [ ] Le firmware compile et tient dans le slot OTA (~1.9MB)
- [ ] Commissioning Matter réussi avec Google Home / Apple Home
- [ ] "Ouvrir" dans l'app → store s'ouvre → position mise à jour dans l'app
- [ ] "50%" dans l'app → store va à mi-course
- [ ] Batterie basse → moteur bloqué → notification dans l'app
- [ ] OTA réussie → nouveau firmware → self-test → opérationnel
- [ ] Crash moteur → bobines coupées → reboot propre

## Notes techniques

- **GPIO 0** : à évaluer sur le PCB final. Si problème de boot, remapper IN4 sur GPIO 5 ou GPIO 13.
- **Taille mémoire** : surveiller avec `heap_caps_get_free_size(MALLOC_CAP_DEFAULT)` au runtime. Il faut garder au moins ~50KB de heap libre pour la stabilité.
- **Ordre d'init** : l'ordre est critique. Si Matter démarre avant Wi-Fi, il crash. Si le moteur démarre avant le crash handler, un crash early ne coupe pas les bobines. L'ordre dans le registry doit être respecté.
