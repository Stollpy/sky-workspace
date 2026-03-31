# Plan 01 — Architecture : Phase 3 — Crash Safety & Watchdog

## Objectif

Implémenter un système de **sécurité au crash configurable** par composant. Quand le firmware crashe ou qu'un composant échoue, le système doit réagir de manière **déterministe et sûre** — notamment couper les moteurs pour éviter de casser les stores.

## Prérequis

- Phase 2 (Core Package) complétée

## Philosophie

Chaque package Sky déclare son **comportement en cas de crash** via sa configuration. Le système de crash safety est un composant transversal intégré à `sky_core`.

## Architecture

```
                    ┌──────────────┐
                    │  ESP32 Panic │
                    │   Handler    │
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │  sky         │
                    │  crash_mgr  │
                    └──────┬───────┘
                           │
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
    ┌──────────┐    ┌──────────┐    ┌──────────┐
    │ stepper  │    │  matter  │    │  power   │
    │ on_crash │    │ on_crash │    │ on_crash │
    │ = STOP   │    │ = LOG    │    │ = SLEEP  │
    └──────────┘    └──────────┘    └──────────┘
```

## API

### Configuration par composant

```c
typedef enum {
    SKY_CRASH_ACTION_NONE,       // Ne rien faire
    SKY_CRASH_ACTION_LOG,        // Logger l'erreur uniquement
    SKY_CRASH_ACTION_STOP,       // Arrêter proprement le composant
    SKY_CRASH_ACTION_SAFE_STATE, // Mettre en état sûr (ex: couper moteur)
    SKY_CRASH_ACTION_RESTART,    // Redémarrer le composant
    SKY_CRASH_ACTION_REBOOT,     // Reboot complet de l'ESP32
} sky_crash_action_t;

typedef struct {
    sky_crash_action_t on_task_watchdog;   // Quand une task ne répond plus
    sky_crash_action_t on_component_error; // Quand le composant retourne une erreur
    sky_crash_action_t on_system_panic;    // Quand l'ESP32 panic (crash fatal)
    void (*safe_state_handler)(void *ctx);      // Callback custom pour SAFE_STATE
    void *ctx;
    uint32_t watchdog_timeout_ms;               // Timeout watchdog par composant (0 = défaut système)
} sky_crash_config_t;

#define SKY_CRASH_CONFIG_DEFAULT() { \
    .on_task_watchdog = SKY_CRASH_ACTION_SAFE_STATE, \
    .on_component_error = SKY_CRASH_ACTION_LOG, \
    .on_system_panic = SKY_CRASH_ACTION_SAFE_STATE, \
    .safe_state_handler = NULL, \
    .ctx = NULL, \
    .watchdog_timeout_ms = 0, \
}
```

### Crash Manager API

```c
esp_err_t sky_crash_init(void);
esp_err_t sky_crash_register(sky_service_id_t service_id,
                                   const sky_crash_config_t *config);
esp_err_t sky_crash_trigger(sky_service_id_t service_id,
                                  sky_error_severity_t severity);
esp_err_t sky_crash_enter_safe_state_all(void);
```

### Integration avec le Stepper

```c
// Le package stepper enregistre son comportement crash :
static void stepper_safe_state(void *ctx) {
    stepper_handle_t *motors = (stepper_handle_t *)ctx;
    // Couper immédiatement toutes les bobines de tous les moteurs
    for (int i = 0; i < motor_count; i++) {
        stepper_emergency_stop(&motors[i]);
    }
}

sky_crash_config_t stepper_crash = {
    .on_task_watchdog = SKY_CRASH_ACTION_SAFE_STATE,
    .on_component_error = SKY_CRASH_ACTION_SAFE_STATE,
    .on_system_panic = SKY_CRASH_ACTION_SAFE_STATE,
    .safe_state_handler = stepper_safe_state,
    .ctx = motor_handles,
    .watchdog_timeout_ms = 5000,
};

sky_crash_register("stepper", &stepper_crash);
```

## Task Watchdog

Chaque composant qui crée une FreeRTOS task peut l'enregistrer auprès du task watchdog ESP-IDF. Si la task ne feed pas le watchdog dans le timeout configuré, le crash manager est notifié.

```c
// Le composant enregistre sa task auprès du watchdog
esp_task_wdt_add(stepper_task_handle);

// Dans la boucle principale de la task
while (running) {
    // ... travail ...
    esp_task_wdt_reset();  // Feed le watchdog
}
```

## Panic Handler

ESP-IDF permet d'enregistrer un **panic handler custom** appelé juste avant le reboot en cas de crash fatal :

```c
void __attribute__((noreturn)) esp_system_abort(const char *details);

// On hook dans le core_dump pour appeler nos safe_state_handlers
// avant le reboot automatique
```

**Limitation** : dans un panic handler, seules les opérations GPIO basiques sont sûres (pas de FreeRTOS, pas de malloc, pas de log). C'est suffisant pour couper les bobines du moteur (simple `gpio_set_level`).

## Tâches

- [ ] Implémenter `sky_crash` (crash manager avec registry de configs)
- [ ] Intégrer avec le task watchdog ESP-IDF (`esp_task_wdt`)
- [ ] Implémenter le panic handler custom (GPIO-safe uniquement)
- [ ] Ajouter Kconfig pour les timeouts watchdog par défaut
- [ ] Intégrer avec `sky_error` (les erreurs FATAL déclenchent le crash manager)
- [ ] Intégrer avec `sky_registry` (crash config fait partie du lifecycle)

## Critères de validation

- [ ] Un composant qui timeout sur le watchdog déclenche le `safe_state_handler`
- [ ] Un panic (ex: stack overflow provoqué) coupe les GPIO moteur avant reboot
- [ ] La configuration crash est indépendante par composant
- [ ] Les actions RESTART relancent uniquement le composant fautif, pas tout le système
- [ ] Le comportement est configurable via Kconfig et via la config runtime

## Notes techniques

- **Panic handler** : dans le contexte panic, l'ESP32 est dans un état instable. Ne faire que des opérations GPIO directes (`REG_WRITE` si possible, ou `gpio_set_level` au minimum). Pas de FreeRTOS, pas de log, pas de malloc.
- **Safe state pour le stepper** : couper les 4 GPIO (IN1-IN4) à LOW = toutes les bobines off = moteur libre. C'est le comportement le plus sûr (le store peut descendre légèrement par gravité, mais pas de surchauffe ni de blocage).
- **Ordre d'exécution** : en cas de panic, exécuter les safe_state_handlers dans l'ordre **inverse** d'enregistrement (le dernier enregistré est le premier arrêté).
