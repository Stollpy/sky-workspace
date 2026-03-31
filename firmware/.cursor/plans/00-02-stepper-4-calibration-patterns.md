# Plan 02 — Stepper Motor : Phase 4 — Calibration & Patterns

## Objectif

Implémenter le système de **calibration** (déterminer la course complète du store) et le système de **patterns** (enregistrer et rejouer des séquences de mouvement complexes).

## Prérequis

- Plan 02, Phase 3 (Motion Controller) complétée

## Calibration

### Flux de calibration (piloté par le client)

```
Client (App/HA)                    ESP32 (Firmware)
      │                                  │
      │  1. calibrate_start(CW)          │
      │─────────────────────────────────►│ État → CALIBRATING
      │                                  │ Moteur tourne CW en continu
      │                                  │ Compteur steps = 0
      │                                  │
      │  (utilisateur observe le store)   │
      │                                  │
      │  2. calibrate_mark_open()        │
      │─────────────────────────────────►│ Enregistre position open = current
      │                                  │ Continue de tourner
      │                                  │
      │  (utilisateur observe le store)   │
      │                                  │
      │  3. calibrate_mark_closed()      │
      │─────────────────────────────────►│ Enregistre position closed = current
      │                                  │ Arrête le moteur
      │                                  │ Calcule total_steps = closed - open
      │                                  │ Persiste en NVS
      │                                  │ État → CALIBRATED
      │                                  │
      │  4. calibration_result           │
      │◄─────────────────────────────────│ {open: 0, closed: 3500, total: 3500}
      │                                  │
```

### API Calibration

```c
// Démarrer la calibration (mouvement continu dans une direction)
esp_err_t sky_stepper_calibrate_start(sky_stepper_handle_t handle,
                                            sky_direction_t direction);

// Marquer la position ouverte
esp_err_t sky_stepper_calibrate_mark_open(sky_stepper_handle_t handle);

// Marquer la position fermée
esp_err_t sky_stepper_calibrate_mark_closed(sky_stepper_handle_t handle);

// Annuler la calibration
esp_err_t sky_stepper_calibrate_cancel(sky_stepper_handle_t handle);

// Vérifier si calibré
bool sky_stepper_is_calibrated(sky_stepper_handle_t handle);

// Obtenir les données de calibration
typedef struct {
    int32_t open_position;    // Position steps ouverte (= 0 après normalisation)
    int32_t closed_position;  // Position steps fermée
    int32_t total_steps;      // Course totale
    uint32_t timestamp;       // Date de la dernière calibration
} sky_stepper_calibration_t;

esp_err_t sky_stepper_get_calibration(sky_stepper_handle_t handle,
                                            sky_stepper_calibration_t *cal);

// Effacer la calibration
esp_err_t sky_stepper_clear_calibration(sky_stepper_handle_t handle);
```

### Persistance NVS

Les données de calibration sont stockées dans le namespace `stp_cal_<nom>` :

| Clé | Type | Contenu |
|-----|------|---------|
| `open_pos` | i32 | Position ouverte |
| `closed_pos` | i32 | Position fermée |
| `total` | i32 | Course totale |
| `ts` | u32 | Timestamp calibration |

## Patterns

### Concept

Un pattern est une **séquence nommée de mouvements** qui peut être enregistrée, sauvegardée et rejouée. Deux types :

1. **Position preset** : un simple pourcentage nommé (ex: "mi-ombre" = 50%)
2. **Sequence** : une suite d'actions avec délais (ex: ouvrir 30%, pause 5s, ouvrir 70%)

### Structure de données

```c
typedef enum {
    SKY_PATTERN_PRESET,     // Position simple (pourcentage)
    SKY_PATTERN_SEQUENCE,   // Suite de mouvements
} sky_pattern_type_t;

typedef struct {
    uint8_t target_percent;      // Position cible (0-100)
    uint32_t delay_after_ms;     // Délai après avoir atteint la position
} sky_pattern_step_t;

#define SKY_PATTERN_MAX_STEPS 16
#define SKY_PATTERN_NAME_MAX  16

typedef struct {
    char name[SKY_PATTERN_NAME_MAX]; // Identifiant du pattern
    sky_pattern_type_t type;
    union {
        uint8_t preset_percent;                                   // Pour PRESET
        struct {
            sky_pattern_step_t steps[SKY_PATTERN_MAX_STEPS];
            uint8_t step_count;
        } sequence;                                                // Pour SEQUENCE
    };
} sky_pattern_t;
```

### API Patterns

```c
// === Presets ===

// Sauvegarder un preset (ex: "demi" = 50%)
esp_err_t sky_stepper_preset_save(sky_stepper_handle_t handle,
                                        const char *name,
                                        uint8_t percent);

// Appliquer un preset
esp_err_t sky_stepper_preset_apply(sky_stepper_handle_t handle,
                                         const char *name);

// Supprimer un preset
esp_err_t sky_stepper_preset_delete(sky_stepper_handle_t handle,
                                          const char *name);

// === Sequences ===

// Commencer l'enregistrement d'une séquence
esp_err_t sky_stepper_sequence_record_start(sky_stepper_handle_t handle,
                                                   const char *name);

// Ajouter un step à la séquence en cours d'enregistrement
esp_err_t sky_stepper_sequence_record_step(sky_stepper_handle_t handle,
                                                  uint8_t target_percent,
                                                  uint32_t delay_after_ms);

// Terminer et sauvegarder l'enregistrement
esp_err_t sky_stepper_sequence_record_stop(sky_stepper_handle_t handle);

// Jouer une séquence
esp_err_t sky_stepper_sequence_play(sky_stepper_handle_t handle,
                                          const char *name);

// Arrêter la lecture d'une séquence
esp_err_t sky_stepper_sequence_stop(sky_stepper_handle_t handle);

// === Gestion ===

// Lister les patterns d'un moteur
esp_err_t sky_stepper_pattern_list(sky_stepper_handle_t handle,
                                         sky_pattern_t *patterns,
                                         uint8_t *count,
                                         uint8_t max);

// Exporter un pattern (pour envoyer au client)
esp_err_t sky_stepper_pattern_export(sky_stepper_handle_t handle,
                                           const char *name,
                                           sky_pattern_t *pattern);

// Importer un pattern (reçu du client)
esp_err_t sky_stepper_pattern_import(sky_stepper_handle_t handle,
                                           const sky_pattern_t *pattern);
```

### Persistance NVS

Les patterns sont stockés dans le namespace `stp_pat_<nom>` :

| Clé | Type | Contenu |
|-----|------|---------|
| `_count` | u8 | Nombre de patterns |
| `_names` | blob | Liste des noms (séparés par `\0`) |
| `p_<name>` | blob | Données du pattern (struct sérialisée) |

### Flux d'enregistrement live

```
Client                          ESP32                          Moteur
  │                               │                              │
  │ record_start("matin")         │                              │
  │──────────────────────────────►│                              │
  │                               │                              │
  │ set_height(30)                │                              │
  │──────────────────────────────►│ record_step(30, 0)           │
  │                               │─────────────────────────────►│ Bouge à 30%
  │                               │                              │
  │ (5 secondes passent)          │                              │
  │                               │                              │
  │ set_height(70)                │                              │
  │──────────────────────────────►│ record_step(70, 5000)        │
  │                               │─────────────────────────────►│ Bouge à 70%
  │                               │                              │
  │ record_stop()                 │                              │
  │──────────────────────────────►│ Sauvegarde en NVS            │
  │                               │                              │
  │ (plus tard...)                │                              │
  │                               │                              │
  │ sequence_play("matin")        │                              │
  │──────────────────────────────►│ Step 1: set_height(30)       │
  │                               │─────────────────────────────►│ 30%
  │                               │ Pause 5s                     │
  │                               │ Step 2: set_height(70)       │
  │                               │─────────────────────────────►│ 70%
```

## Tâches

- [ ] Implémenter le flux de calibration (start, mark_open, mark_closed)
- [ ] Persister les données de calibration en NVS
- [ ] Implémenter le calcul height% ↔ steps basé sur la calibration
- [ ] Implémenter les presets (save, apply, delete)
- [ ] Implémenter les séquences (record, play, stop)
- [ ] Persister les patterns en NVS
- [ ] Implémenter l'export/import de patterns (pour échange avec le client)
- [ ] Publier les événements calibration/pattern via l'event bus

## Critères de validation

- [ ] Calibration complète : mark_open → mark_closed → set_height(50) = milieu exact du store
- [ ] La calibration persiste après reboot
- [ ] Un preset "demi" à 50% amène le moteur au milieu du store
- [ ] Une séquence enregistrée se rejoue fidèlement (positions + timing)
- [ ] On peut exporter un pattern, le modifier côté client, et le réimporter
- [ ] `set_height()` retourne une erreur si le moteur n'est pas calibré

## Notes techniques

- **Précision du pourcentage** : `set_height(50)` calcule `total_steps / 2` et appelle `move_to()`. La précision est de ~0.088° (1 half-step), ce qui est bien plus fin que nécessaire pour un store.
- **NVS et patterns** : les blobs NVS ont une limite de taille (~1984 bytes pour un blob). Un pattern avec 16 steps tient largement. Si on veut des séquences plus complexes à l'avenir, il faudra soit fragmenter, soit utiliser une partition dédiée.
- **Enregistrement live** : pendant l'enregistrement, le moteur exécute réellement les mouvements (pour que l'utilisateur voie le résultat). Les délais entre les commandes client sont capturés automatiquement.
- **Sécurité calibration** : pendant la calibration, le moteur tourne en continu à vitesse réduite. Le client **doit** envoyer `calibrate_mark_closed()` ou `calibrate_cancel()` pour arrêter. Un timeout de sécurité (configurable via Kconfig, ex: 120s) arrête automatiquement si le client ne répond pas.
