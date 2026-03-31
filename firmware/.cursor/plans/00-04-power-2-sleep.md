# Plan 04 — Power : Phase 2 — Sleep & Energy Management

## Objectif

Implémenter les stratégies d'**économie d'énergie** pour maximiser l'autonomie sur batterie : Wi-Fi power save, modem sleep, deep sleep avec sources de réveil, et gestion du cycle veille/réveil.

## Prérequis

- Plan 04, Phase 1 (Battery Management) complétée
- Plan 03, Phase 1 (Wi-Fi Manager) complétée

## Modes d'énergie

```
                Consommation
    250mA ┤ ████ Active (Wi-Fi + moteur)
          │
    150mA ┤ ████ Active (Wi-Fi idle, pas de moteur)
          │
     20mA ┤ ████ Modem Sleep (Wi-Fi power save)
          │
    800µA ┤ ████ Light Sleep (CPU paused, RAM alive)
          │
     10µA ┤ ████ Deep Sleep (RTC only)
          │
          └──────────────────────────────────
```

### Stratégie pour Sky Motor (store)

Le store n'a pas besoin d'être réactif en permanence. On peut adopter un cycle :
1. **Deep sleep** la plupart du temps (~10µA)
2. **Réveil** périodique (ex: toutes les 30s) pour vérifier les commandes Matter
3. **Active** uniquement pendant un mouvement moteur

**Problème** : Matter fonctionne avec des **subscriptions** TCP. En deep sleep, la connexion Wi-Fi est perdue et les subscriptions expirent. Deux solutions :
- **Option A** : rester en modem sleep (20mA) pour maintenir la connexion Wi-Fi → autonomie ~jours
- **Option B** : deep sleep avec réveil sur GPIO (bouton physique) ou timer → plus longue autonomie mais latence élevée pour les commandes réseau

## Configuration

```c
typedef enum {
    SKY_POWER_MODE_ACTIVE,       // Tout allumé
    SKY_POWER_MODE_MODEM_SLEEP,  // Wi-Fi power save, CPU actif
    SKY_POWER_MODE_LIGHT_SLEEP,  // CPU pausé, réveil rapide
    SKY_POWER_MODE_DEEP_SLEEP,   // Tout éteint sauf RTC
} sky_power_mode_t;

typedef struct {
    sky_power_mode_t idle_mode;        // Mode quand le moteur est idle
    uint32_t idle_timeout_ms;               // Délai avant d'entrer en mode idle (défaut: 30000)
    uint32_t deep_sleep_duration_ms;        // Durée du deep sleep (0 = jusqu'à wake source)
    gpio_num_t wake_gpio;                   // GPIO pour réveil external (bouton, etc.)
    bool wake_on_high;                      // Réveil sur front montant (true) ou descendant
    bool save_state_before_sleep;           // Sauvegarder l'état en NVS avant deep sleep
} sky_sleep_config_t;

#define SKY_SLEEP_CONFIG_DEFAULT() { \
    .idle_mode = SKY_POWER_MODE_MODEM_SLEEP, \
    .idle_timeout_ms = 30000, \
    .deep_sleep_duration_ms = 0, \
    .wake_gpio = GPIO_NUM_MAX, \
    .wake_on_high = true, \
    .save_state_before_sleep = true, \
}
```

## API

```c
// Configurer la stratégie de sleep
esp_err_t sky_sleep_init(const sky_sleep_config_t *config);

// Forcer un mode d'énergie
esp_err_t sky_sleep_set_mode(sky_power_mode_t mode);

// Obtenir le mode courant
sky_power_mode_t sky_sleep_get_mode(void);

// Demander de rester éveillé (empêche le passage en sleep)
// Retourne un token à passer à release()
typedef uint32_t sky_wakelock_t;
esp_err_t sky_sleep_acquire_wakelock(const char *reason,
                                           sky_wakelock_t *lock);
esp_err_t sky_sleep_release_wakelock(sky_wakelock_t lock);

// Vérifier si un wakelock est actif
bool sky_sleep_has_wakelocks(void);

// Entrer en deep sleep manuellement
esp_err_t sky_sleep_enter_deep_sleep(uint32_t duration_ms);

// Obtenir la cause du réveil (après deep sleep)
typedef enum {
    SKY_WAKE_REASON_POWERON,     // Premier boot
    SKY_WAKE_REASON_TIMER,       // Réveil par timer
    SKY_WAKE_REASON_GPIO,        // Réveil par GPIO externe
    SKY_WAKE_REASON_UNKNOWN,
} sky_wake_reason_t;

sky_wake_reason_t sky_sleep_get_wake_reason(void);
```

## Wakelocks

Le système de wakelocks empêche le passage en sleep tant qu'une opération critique est en cours :

```c
// Le stepper acquiert un wakelock pendant le mouvement
static esp_err_t stepper_move_internal(stepper_handle_t handle, ...) {
    sky_wakelock_t lock;
    sky_sleep_acquire_wakelock("stepper_moving", &lock);

    // ... mouvement moteur ...

    sky_sleep_release_wakelock(lock);
    return ESP_OK;
}

// L'OTA acquiert un wakelock pendant le téléchargement
sky_sleep_acquire_wakelock("ota_download", &lock);
// ... téléchargement ...
sky_sleep_release_wakelock(lock);
```

## Cycle veille/réveil avec Matter

```
              ┌─────────────────────────────────────┐
              │                                     │
              ▼                                     │
    ┌──────────────┐                                │
    │  Deep Sleep   │ ← 10µA                        │
    │  (30s timer)  │                                │
    └──────┬───────┘                                │
           │ timer expire / GPIO wake               │
    ┌──────▼───────┐                                │
    │   Boot       │                                │
    │   Wi-Fi on   │ ← ~2-3s pour connexion         │
    └──────┬───────┘                                │
           │                                        │
    ┌──────▼───────┐     ┌────────────────┐         │
    │ Check Matter │────►│ Commande ?     │         │
    │ messages     │     │ Oui → exécuter │         │
    └──────┬───────┘     └────────┬───────┘         │
           │ Non                  │ Terminé          │
           │◄─────────────────────┘                  │
           │                                        │
    ┌──────▼───────┐                                │
    │ Report status│ (batterie, position)            │
    └──────┬───────┘                                │
           │                                        │
           └────────────────────────────────────────┘
                    Retour en deep sleep
```

**Note** : ce cycle deep sleep n'est viable que si la latence de ~30s pour les commandes est acceptable. Pour un store, c'est probablement OK. Pour une serrure, non.

## Événements publiés

| Événement | Données | Quand |
|-----------|---------|-------|
| `SKY_EVENT_SLEEP_ENTER` | `{mode, duration_ms}` | Avant d'entrer en sleep |
| `SKY_EVENT_SLEEP_EXIT` | `{reason, mode}` | Après réveil |
| `SKY_EVENT_POWER_MODE_CHANGED` | `{old_mode, new_mode}` | Changement de mode |

## Tâches

- [ ] Implémenter les transitions entre modes d'énergie
- [ ] Implémenter le modem sleep (Wi-Fi power save via `esp_wifi_set_ps()`)
- [ ] Implémenter le deep sleep avec timer + GPIO wake
- [ ] Implémenter le système de wakelocks
- [ ] Sauvegarder/restaurer l'état via RTC memory ou NVS avant deep sleep
- [ ] Publier les événements sleep via l'event bus
- [ ] Intégrer avec `sky_registry` (appeler stop() sur tous les services avant sleep)
- [ ] Créer le Kconfig (idle mode, timeout, wake GPIO)

## Critères de validation

- [ ] Modem sleep : consommation mesurée ~20mA (multimètre en série)
- [ ] Deep sleep : consommation mesurée ~10-50µA
- [ ] Réveil GPIO : appui sur le bouton wake l'ESP32
- [ ] Les wakelocks empêchent effectivement le passage en sleep
- [ ] Le moteur ne se retrouve pas dans un état incohérent après deep sleep
- [ ] La position du moteur est correcte après un cycle deep sleep/wake

## Notes techniques

- **RTC memory** : l'ESP32 a 8KB de RTC fast memory qui survit au deep sleep. On peut y stocker l'état minimal (position moteur, wake count, etc.) au lieu d'écrire en NVS à chaque cycle.
- **Temps de boot** : après deep sleep, le boot complet (bootloader + Wi-Fi connect + Matter init) prend 2-5 secondes. C'est la latence incompressible.
- **Matter + deep sleep** : Matter n'est pas optimisé pour les appareils qui dorment. Le standard Matter a un concept de "Sleepy End Device" (SED) mais il est conçu pour Thread, pas Wi-Fi. Pour Wi-Fi, la solution recommandée est le modem sleep avec DTIM intervals élevés.
- **Consommation moteur** : pendant le mouvement, le 28BYJ-48 via ULN2003 tire ~240mA. C'est 12x plus que le Wi-Fi en active. L'optimisation de la consommation idle est donc plus impactante que l'optimisation pendant le mouvement.
