# Plan 02 — Stepper Motor : Phase 1 — Hardware Abstraction Layer

## Objectif

Créer la couche d'**abstraction matérielle** (HAL) du package `sky_stepper`. Cette couche isole complètement la logique moteur des GPIO physiques, permettant de supporter différents moteurs et drivers à l'avenir.

## Prérequis

- Plan 01, Phase 1 (Project Scaffolding) complétée

## Architecture

```
┌─────────────────────────────────────────────┐
│            Application Layer                 │
│     (stepper_move, stepper_set_position)     │
├─────────────────────────────────────────────┤
│            Motion Controller                 │   ← Phase 3
│     (state machine, position tracking)       │
├─────────────────────────────────────────────┤
│            Step Engine                       │   ← Cette phase
│     (séquences half/full/wave step)          │
├─────────────────────────────────────────────┤
│            GPIO HAL                          │   ← Cette phase
│     (abstraction pins, set_level)            │
├─────────────────────────────────────────────┤
│            Hardware (ESP32 GPIO)             │
└─────────────────────────────────────────────┘
```

## API — GPIO HAL

```c
typedef struct {
    gpio_num_t pins[4];      // IN1, IN2, IN3, IN4
    bool invert_logic;       // true si logique inversée (certains drivers)
} sky_stepper_gpio_config_t;

typedef struct sky_stepper_gpio *sky_stepper_gpio_handle_t;

esp_err_t sky_stepper_gpio_init(const sky_stepper_gpio_config_t *config,
                                      sky_stepper_gpio_handle_t *handle);
esp_err_t sky_stepper_gpio_set_phase(sky_stepper_gpio_handle_t handle,
                                           const int levels[4]);
esp_err_t sky_stepper_gpio_all_off(sky_stepper_gpio_handle_t handle);
void sky_stepper_gpio_deinit(sky_stepper_gpio_handle_t handle);
```

## API — Step Engine

Le step engine gère les **séquences d'activation des bobines** selon le mode choisi.

```c
typedef enum {
    SKY_STEP_MODE_WAVE,       // 4 phases, faible couple
    SKY_STEP_MODE_FULL,       // 4 phases, couple élevé
    SKY_STEP_MODE_HALF,       // 8 phases, haute résolution (défaut)
} sky_step_mode_t;

typedef enum {
    SKY_DIR_CW,   // Clockwise
    SKY_DIR_CCW,  // Counter-clockwise
} sky_direction_t;

typedef struct {
    sky_step_mode_t mode;
    uint32_t step_delay_us;            // Délai entre chaque step (min: ~1500us pour 28BYJ-48)
    uint16_t steps_per_revolution;     // 4096 pour half-step 28BYJ-48
} sky_step_engine_config_t;

#define SKY_STEP_ENGINE_28BYJ48_DEFAULT() { \
    .mode = SKY_STEP_MODE_HALF, \
    .step_delay_us = 2000, \
    .steps_per_revolution = 4096, \
}

typedef struct sky_step_engine *sky_step_engine_handle_t;

esp_err_t sky_step_engine_init(const sky_step_engine_config_t *config,
                                     sky_stepper_gpio_handle_t gpio,
                                     sky_step_engine_handle_t *handle);

// Avancer d'un step dans la direction donnée
esp_err_t sky_step_engine_step(sky_step_engine_handle_t handle,
                                     sky_direction_t direction);

// Obtenir la phase courante (0-7 pour half-step, 0-3 pour full/wave)
int sky_step_engine_get_phase(sky_step_engine_handle_t handle);

// Changer le mode à chaud
esp_err_t sky_step_engine_set_mode(sky_step_engine_handle_t handle,
                                         sky_step_mode_t mode);

// Changer la vitesse à chaud
esp_err_t sky_step_engine_set_delay(sky_step_engine_handle_t handle,
                                          uint32_t step_delay_us);

void sky_step_engine_deinit(sky_step_engine_handle_t handle);
```

## Séquences de stepping

Stockées en const pour chaque mode, identiques à celles documentées dans `learning/00-stepper-motor.md` :

```c
// Half-step (8 phases) — résolution maximale
static const int half_step_sequence[8][4] = {
    {1, 0, 0, 0},  // A+
    {1, 0, 1, 0},  // A+ A-
    {0, 0, 1, 0},  //    A-
    {0, 1, 1, 0},  // B+ A-
    {0, 1, 0, 0},  // B+
    {0, 1, 0, 1},  // B+ B-
    {0, 0, 0, 1},  //    B-
    {1, 0, 0, 1},  // A+ B-
};

// Full-step (4 phases) — couple maximal
static const int full_step_sequence[4][4] = {
    {1, 0, 1, 0},
    {0, 1, 1, 0},
    {0, 1, 0, 1},
    {1, 0, 0, 1},
};

// Wave drive (4 phases) — consommation minimale
static const int wave_step_sequence[4][4] = {
    {1, 0, 0, 0},
    {0, 0, 1, 0},
    {0, 1, 0, 0},
    {0, 0, 0, 1},
};
```

## Timing

Le timing entre steps est géré par `esp_timer` (haute résolution, non-bloquant) plutôt que `ets_delay_us` (bloquant) pour ne pas bloquer le scheduler FreeRTOS.

```c
// Approche avec esp_timer (recommandée pour le HAL)
esp_timer_handle_t step_timer;
esp_timer_create_args_t timer_args = {
    .callback = step_timer_callback,
    .arg = engine_handle,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "stepper_step",
};
esp_timer_create(&timer_args, &step_timer);
esp_timer_start_periodic(step_timer, config->step_delay_us);
```

## Structure des fichiers

```
components/sky_stepper/
├── CMakeLists.txt
├── Kconfig
├── include/
│   ├── sky_stepper.h          # API publique complète (créée en Phase 2)
│   ├── sky_stepper_gpio.h     # HAL GPIO
│   ├── sky_stepper_engine.h   # Step engine
│   └── sky_stepper_types.h    # Types partagés (enums, configs)
└── src/
    ├── sky_stepper_gpio.c     # Implémentation GPIO HAL
    └── sky_stepper_engine.c   # Implémentation step engine
```

## Tâches

- [ ] Créer `sky_stepper_types.h` (enums, structs de config)
- [ ] Implémenter `sky_stepper_gpio` (init, set_phase, all_off)
- [ ] Implémenter `sky_stepper_engine` (step sequences, phase tracking)
- [ ] Intégrer `esp_timer` pour le timing non-bloquant
- [ ] Créer le CMakeLists.txt et Kconfig du composant
- [ ] Valider avec le 28BYJ-48 sur le hardware actuel

## Critères de validation

- [ ] Le moteur 28BYJ-48 tourne correctement dans les 3 modes (half, full, wave)
- [ ] On peut changer le mode et la vitesse à chaud sans arrêter le moteur
- [ ] `sky_stepper_gpio_all_off()` coupe immédiatement toutes les bobines
- [ ] Le timing est précis (vérifié à l'oscilloscope ou via les logs)
- [ ] Le step engine ne bloque pas le scheduler FreeRTOS

## Notes techniques

- **`esp_timer` vs `ets_delay_us`** : `ets_delay_us` est bloquant (busy-wait), ce qui empêche FreeRTOS de scheduler d'autres tasks. `esp_timer` utilise une task dédiée haute priorité et des interruptions timer. Pour un stepping à 2ms, `esp_timer` est largement suffisant en précision.
- **Opaque handles** : les structures internes (`struct sky_stepper_gpio`, `struct sky_step_engine`) sont forward-déclarées dans les headers et définies uniquement dans les `.c`. L'application ne manipule que des handles (pointeurs opaques). Cela permet de modifier l'implémentation sans casser l'API.
- **28BYJ-48 spécifique** : le `steps_per_revolution` de 4096 est spécifique au réducteur 1:64 du 28BYJ-48. Un autre moteur aura une valeur différente. C'est pour ça que c'est configurable.
