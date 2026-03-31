# Plan 02 — Stepper Motor : Phase 2 — Motor Registry & Configuration

## Objectif

Implémenter le système d'**enregistrement et de gestion multi-moteur**, inspiré du pattern Fortify de Laravel. Un ESP32 pourra gérer **plusieurs moteurs** simultanément, chacun avec sa propre configuration (pins, vitesse, mode, crash behavior).

## Prérequis

- Plan 02, Phase 1 (HAL) complétée
- Plan 01, Phase 2 (Core Package — registry, events) complétée

## Pattern d'utilisation visé (Fortify-like)

```c
// === main.c — Application Sky Motor ===

#include "sky_stepper.h"

void app_main(void) {
    sky_core_init();

    // Moteur principal (store salon)
    sky_stepper_config_t motor_salon = {
        .name = "salon",
        .gpio = {
            .pins = {GPIO_NUM_17, GPIO_NUM_16, GPIO_NUM_4, GPIO_NUM_0},
        },
        .engine = SKY_STEP_ENGINE_28BYJ48_DEFAULT(),
        .crash = {
            .on_task_watchdog = SKY_CRASH_ACTION_SAFE_STATE,
            .on_system_panic = SKY_CRASH_ACTION_SAFE_STATE,
        },
        .features = {
            .position_tracking = true,
            .calibration = true,
            .patterns = true,
        },
    };

    sky_stepper_handle_t salon;
    sky_stepper_register(&motor_salon, &salon);

    // Moteur secondaire (store chambre) — même ESP32
    sky_stepper_config_t motor_chambre = {
        .name = "chambre",
        .gpio = {
            .pins = {GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_14},
        },
        .engine = SKY_STEP_ENGINE_28BYJ48_DEFAULT(),
        .crash = SKY_CRASH_CONFIG_DEFAULT(),
        .features = {
            .position_tracking = true,
            .calibration = true,
            .patterns = false,
        },
    };

    sky_stepper_handle_t chambre;
    sky_stepper_register(&motor_chambre, &chambre);

    // Démarrer tout
    sky_core_start();
}
```

## API — Configuration complète

```c
typedef struct {
    bool position_tracking;   // Suivre la position courante
    bool calibration;         // Permettre la calibration (début/fin de course)
    bool patterns;            // Permettre l'enregistrement/lecture de patterns
    bool acceleration;        // Rampes d'accélération/décélération
} sky_stepper_features_t;

#define SKY_STEPPER_FEATURES_ALL() { \
    .position_tracking = true, \
    .calibration = true, \
    .patterns = true, \
    .acceleration = true, \
}

typedef struct {
    const char *name;                         // Identifiant unique du moteur
    sky_stepper_gpio_config_t gpio;      // Configuration GPIO (pins)
    sky_step_engine_config_t engine;     // Configuration step engine (mode, vitesse)
    sky_crash_config_t crash;            // Comportement en cas de crash
    sky_stepper_features_t features;     // Features activées
    uint8_t task_priority;                    // Priorité FreeRTOS (défaut: 5)
    uint16_t task_stack_size;                 // Stack size en words (défaut: 4096)
} sky_stepper_config_t;

#define SKY_STEPPER_CONFIG_DEFAULT() { \
    .name = "default", \
    .gpio = { 0 }, \
    .engine = SKY_STEP_ENGINE_28BYJ48_DEFAULT(), \
    .crash = SKY_CRASH_CONFIG_DEFAULT(), \
    .features = SKY_STEPPER_FEATURES_ALL(), \
    .task_priority = 5, \
    .task_stack_size = 4096, \
}
```

## API — Registration & Lifecycle

```c
typedef struct sky_stepper *sky_stepper_handle_t;

// Enregistrer un nouveau moteur
esp_err_t sky_stepper_register(const sky_stepper_config_t *config,
                                     sky_stepper_handle_t *handle);

// Retrouver un moteur par son nom
sky_stepper_handle_t sky_stepper_get(const char *name);

// Lister les moteurs enregistrés
esp_err_t sky_stepper_list(sky_stepper_handle_t *handles,
                                 uint8_t *count,
                                 uint8_t max);

// Désenregistrer un moteur
esp_err_t sky_stepper_unregister(sky_stepper_handle_t handle);
```

## API — Event callbacks (middleware-like)

Système de hooks pour intercepter les commandes moteur. Chaque hook peut modifier ou bloquer l'action.

```c
typedef enum {
    SKY_STEPPER_HOOK_BEFORE_MOVE,   // Avant un mouvement
    SKY_STEPPER_HOOK_AFTER_MOVE,    // Après un mouvement
    SKY_STEPPER_HOOK_BEFORE_STOP,   // Avant un arrêt
    SKY_STEPPER_HOOK_AFTER_STOP,    // Après un arrêt
    SKY_STEPPER_HOOK_ON_ERROR,      // Quand une erreur survient
    SKY_STEPPER_HOOK_ON_POSITION,   // Quand la position change
} sky_stepper_hook_type_t;

typedef struct {
    sky_stepper_hook_type_t type;
    sky_stepper_handle_t motor;
    int32_t target_steps;
    sky_direction_t direction;
    bool cancel;   // Le hook peut mettre cancel=true pour annuler l'action
} sky_stepper_hook_event_t;

typedef void (*sky_stepper_hook_t)(sky_stepper_hook_event_t *event, void *ctx);

// Ajouter un hook (middleware)
esp_err_t sky_stepper_add_hook(sky_stepper_handle_t handle,
                                     sky_stepper_hook_type_t type,
                                     sky_stepper_hook_t hook,
                                     void *ctx);

// Retirer un hook
esp_err_t sky_stepper_remove_hook(sky_stepper_handle_t handle,
                                        sky_stepper_hook_type_t type,
                                        sky_stepper_hook_t hook);
```

**Exemple d'utilisation middleware** :

```c
// Empêcher tout mouvement si la batterie est trop basse
void battery_guard_hook(sky_stepper_hook_event_t *event, void *ctx) {
    if (battery_level < 10) {
        ESP_LOGW(TAG, "Battery too low (%d%%), blocking motor movement", battery_level);
        event->cancel = true;
    }
}

sky_stepper_add_hook(salon, SKY_STEPPER_HOOK_BEFORE_MOVE,
                           battery_guard_hook, NULL);
```

## Integration avec sky_core

Le package stepper s'enregistre automatiquement dans le service registry :

```c
// Interne au package — appelé par sky_stepper_register()
static esp_err_t stepper_service_init(void *config) {
    // Init GPIO HAL + step engine pour chaque moteur enregistré
}

static esp_err_t stepper_service_start(void *config) {
    // Créer les FreeRTOS tasks pour chaque moteur
}

sky_service_lifecycle_t stepper_lifecycle = {
    .init = stepper_service_init,
    .start = stepper_service_start,
    .stop = stepper_service_stop,
    .deinit = stepper_service_deinit,
};

// Auto-enregistrement au premier appel de sky_stepper_register()
sky_registry_register("sky_stepper", &stepper_lifecycle, NULL);
```

## Kconfig

```kconfig
menu "Sky Stepper"
    config SKY_STEPPER_MAX_MOTORS
        int "Maximum number of stepper motors"
        default 4
        range 1 8
        help
            Maximum number of stepper motors that can be registered
            simultaneously on a single ESP32.

    config SKY_STEPPER_DEFAULT_TASK_PRIORITY
        int "Default FreeRTOS task priority for motor tasks"
        default 5
        range 1 24

    config SKY_STEPPER_DEFAULT_STACK_SIZE
        int "Default stack size for motor tasks (bytes)"
        default 4096
        range 2048 8192
endmenu
```

## Tâches

- [ ] Définir `sky_stepper_config_t` complète avec tous les champs
- [ ] Implémenter le motor registry (tableau statique de handles, taille `MAX_MOTORS`)
- [ ] Implémenter `sky_stepper_register()` (alloc handle, init HAL, init engine)
- [ ] Implémenter `sky_stepper_get()` (lookup par nom)
- [ ] Implémenter le système de hooks (middleware chain)
- [ ] Intégrer avec `sky_registry` (service lifecycle)
- [ ] Intégrer avec `sky_crash` (crash config par moteur)
- [ ] Intégrer avec `sky_event` (publier les événements stepper)
- [ ] Créer le Kconfig avec les options de configuration

## Critères de validation

- [ ] On peut enregistrer 2+ moteurs avec des configs différentes sur le même ESP32
- [ ] `sky_stepper_get("salon")` retourne le bon handle
- [ ] Les hooks BEFORE_MOVE avec `cancel=true` empêchent effectivement le mouvement
- [ ] Le service lifecycle (init→start→stop→deinit) fonctionne via `sky_core_start()`
- [ ] Un crash d'un moteur n'affecte pas les autres (isolation)

## Notes techniques

- **Allocation statique** : les handles moteur sont stockés dans un tableau statique de taille `SKY_STEPPER_MAX_MOTORS` pour éviter le malloc dynamique (prédictibilité mémoire).
- **FreeRTOS task par moteur** : chaque moteur a sa propre task FreeRTOS pour le stepping. Cela permet le mouvement simultané de plusieurs moteurs sans interférence de timing.
- **Hooks comme middleware** : les hooks BEFORE sont exécutés **avant** l'action. Si un hook met `cancel=true`, les hooks suivants ne sont pas exécutés et l'action est annulée. C'est le même pattern que les middleware Express/Laravel.
- **Thread safety** : les opérations sur le registry (register, get, list) sont protégées par un mutex. Les hooks sont appelés dans le contexte de la task du moteur concerné.
