# Plan 02 — Stepper Motor : Phase 3 — Motion Controller

## Objectif

Implémenter le **contrôleur de mouvement** : la machine à états qui gère les déplacements, le suivi de position, les rampes d'accélération et les commandes de haut niveau (move, stop, go_to).

## Prérequis

- Plan 02, Phase 2 (Motor Registry) complétée

## Machine à états

```
                              ┌──────────┐
                    init()    │          │  error
                ┌────────────►│   IDLE   │◄──────────────┐
                │             │          │               │
                │             └────┬─────┘               │
                │                  │                     │
                │           move() │ go_to()             │
                │                  │                     │
                │             ┌────▼─────┐               │
                │             │          │  watchdog      │
                │             │  MOVING  │──────────►┌───┴────┐
                │             │          │           │        │
                │             └──┬───┬───┘           │ ERROR  │
                │                │   │               │        │
                │      target    │   │ stop()        └────────┘
                │      reached   │   │
                │                │   │
                │             ┌──▼───▼───┐
                │             │          │
                │             │ STOPPING │
                │             │          │
                │             └────┬─────┘
                │                  │
                │                  │ decel done
                └──────────────────┘

                    calibrate()
                ┌────────────────────────┐
                │                        │
           ┌────▼──────┐          ┌──────┴──────┐
           │           │ client   │             │
           │CALIBRATING│─────────►│ CALIBRATED  │
           │           │ stop     │             │
           └───────────┘          └─────────────┘
```

### États

| État | Description |
|------|------------|
| `IDLE` | Moteur arrêté, bobines coupées, en attente de commande |
| `MOVING` | Moteur en mouvement vers une cible (steps ou position) |
| `STOPPING` | Décélération en cours avant arrêt |
| `CALIBRATING` | Mode calibration actif (mouvement continu jusqu'à stop client) |
| `CALIBRATED` | Calibration terminée, limites enregistrées |
| `ERROR` | Erreur détectée (watchdog, dépassement limites, etc.) |

## API — Commandes de mouvement

```c
// Mouvement relatif (N steps dans une direction)
esp_err_t sky_stepper_move(sky_stepper_handle_t handle,
                                 uint32_t steps,
                                 sky_direction_t direction);

// Mouvement absolu (aller à une position en steps depuis 0)
esp_err_t sky_stepper_move_to(sky_stepper_handle_t handle,
                                    int32_t target_position);

// Aller à un pourcentage (0% = ouvert, 100% = fermé)
// Nécessite calibration
esp_err_t sky_stepper_set_height(sky_stepper_handle_t handle,
                                       uint8_t percent);

// Arrêter le mouvement (avec décélération si rampe activée)
esp_err_t sky_stepper_stop(sky_stepper_handle_t handle);

// Arrêt d'urgence (immédiat, sans décélération, bobines coupées)
esp_err_t sky_stepper_emergency_stop(sky_stepper_handle_t handle);

// Obtenir l'état courant
sky_stepper_state_t sky_stepper_get_state(sky_stepper_handle_t handle);

// Obtenir la position courante (en steps depuis la position 0)
int32_t sky_stepper_get_position(sky_stepper_handle_t handle);

// Obtenir la position en pourcentage (nécessite calibration)
int8_t sky_stepper_get_height_percent(sky_stepper_handle_t handle);
```

## Position Tracking

Le contrôleur maintient un compteur de position absolue :

```c
typedef struct {
    int32_t current_position;     // Position actuelle (steps depuis home)
    int32_t target_position;      // Position cible
    int32_t min_position;         // Limite basse (0 après calibration)
    int32_t max_position;         // Limite haute (total steps après calibration)
    bool calibrated;              // true si calibration effectuée
    sky_stepper_state_t state;
    sky_direction_t direction;
} sky_stepper_status_t;

esp_err_t sky_stepper_get_status(sky_stepper_handle_t handle,
                                       sky_stepper_status_t *status);
```

La position est persistée en NVS (via `sky_config`) pour survivre aux reboots. Namespace : `stp_<nom>` (ex: `stp_salon`).

## Rampes d'accélération / décélération

Pour un mouvement fluide et silencieux, le moteur accélère progressivement au démarrage et décélère avant l'arrêt :

```
Vitesse (steps/s)
    │
max ├─────────────────────────────┐
    │        /                     \
    │       /                       \
    │      /                         \
min ├─────/                           \─────
    │
    └──────────────────────────────────────── Temps
         accel    constant speed    decel
```

```c
typedef struct {
    uint32_t min_delay_us;        // Vitesse max (délai min entre steps)
    uint32_t max_delay_us;        // Vitesse min (délai max, vitesse de démarrage)
    uint16_t accel_steps;         // Nombre de steps pour atteindre la vitesse max
    uint16_t decel_steps;         // Nombre de steps pour la décélération
} sky_stepper_ramp_config_t;

#define SKY_STEPPER_RAMP_DEFAULT() { \
    .min_delay_us = 1500, \
    .max_delay_us = 4000, \
    .accel_steps = 200, \
    .decel_steps = 200, \
}

// Activer/configurer les rampes
esp_err_t sky_stepper_set_ramp(sky_stepper_handle_t handle,
                                     const sky_stepper_ramp_config_t *ramp);
```

Le calcul du délai courant utilise une interpolation linéaire :

```
delay(step) = max_delay - (max_delay - min_delay) * (step / accel_steps)
```

## FreeRTOS Task — Boucle moteur

Chaque moteur a sa propre task FreeRTOS qui consomme des commandes depuis une queue :

```c
typedef enum {
    STEPPER_CMD_MOVE,
    STEPPER_CMD_MOVE_TO,
    STEPPER_CMD_SET_HEIGHT,
    STEPPER_CMD_STOP,
    STEPPER_CMD_EMERGENCY_STOP,
    STEPPER_CMD_CALIBRATE_START,
    STEPPER_CMD_CALIBRATE_STOP,
} stepper_cmd_type_t;

typedef struct {
    stepper_cmd_type_t type;
    union {
        struct { uint32_t steps; sky_direction_t dir; } move;
        struct { int32_t position; } move_to;
        struct { uint8_t percent; } set_height;
    };
} stepper_cmd_t;

// Task principale du moteur
static void stepper_motor_task(void *arg) {
    stepper_cmd_t cmd;
    while (1) {
        if (xQueueReceive(motor->cmd_queue, &cmd, portMAX_DELAY)) {
            // Exécuter les hooks BEFORE
            // Transition state machine
            // Exécuter le mouvement step par step
            // Exécuter les hooks AFTER
            // Publier événement position
        }
    }
}
```

## Événements publiés

Via `sky_event_publish()` :

| Événement | Données | Quand |
|-----------|---------|-------|
| `STEPPER_STARTED` | `{name, direction, target}` | Début de mouvement |
| `STEPPER_STOPPED` | `{name, position, reason}` | Arrêt (normal ou urgence) |
| `STEPPER_POSITION` | `{name, position, percent}` | Tous les N steps (configurable) |
| `STEPPER_ERROR` | `{name, error_code, message}` | Erreur détectée |
| `STEPPER_CALIBRATED` | `{name, min, max, total}` | Calibration terminée |

## Tâches

- [ ] Implémenter la machine à états (transitions, validation)
- [ ] Implémenter les commandes de mouvement (move, move_to, set_height)
- [ ] Implémenter le position tracking (compteur + persistance NVS)
- [ ] Implémenter les rampes d'accélération/décélération
- [ ] Créer la FreeRTOS task par moteur avec command queue
- [ ] Publier les événements stepper via l'event bus
- [ ] Intégrer les hooks dans chaque transition
- [ ] Implémenter `emergency_stop` (bypass tout, GPIO off immédiat)

## Critères de validation

- [ ] Le moteur suit correctement la machine à états (pas de transition invalide)
- [ ] `set_height(50)` après calibration amène le moteur à mi-course
- [ ] La position persiste après un reboot (NVS)
- [ ] Les rampes produisent un mouvement fluide (vérifiable auditivement)
- [ ] `emergency_stop` coupe le moteur en < 1ms
- [ ] Deux moteurs peuvent se déplacer simultanément sans interférence de timing

## Notes techniques

- **Position signée (int32_t)** : la position 0 est définie lors de la calibration (position ouverte). Les positions positives vont vers la fermeture. Cela permet des dépassements temporaires si nécessaire.
- **Persistance NVS** : la position n'est écrite en NVS que quand le moteur s'arrête (pas à chaque step) pour limiter l'usure flash. En cas de crash pendant un mouvement, la position est approximative — une re-calibration peut être nécessaire.
- **Yield FreeRTOS** : pendant le stepping, la task doit yield périodiquement (`vTaskDelay(1)` tous les ~500 steps) pour ne pas starve les autres tasks (Wi-Fi, Matter, etc.).
