# Plan 04 — Power : Phase 1 — Battery Management (sky_power)

## Objectif

Créer le package `sky_power` : gestion de la batterie, monitoring du niveau de charge, seuils d'alerte configurables et reporting via Matter (Power Source cluster).

## Prérequis

- Plan 01, Phase 2 (Core Package) complétée

## Architecture

```
┌──────────────────────────────────────────┐
│           sky_power                  │
├──────────────────────────────────────────┤
│  ┌──────────────┐  ┌─────────────────┐  │
│  │  ADC Monitor │  │  Level Calc     │  │
│  │  (lecture    │  │  (voltage →     │  │
│  │   tension)   │  │   pourcentage)  │  │
│  └──────┬───────┘  └────────┬────────┘  │
│         │                   │            │
│  ┌──────▼───────────────────▼────────┐  │
│  │        Threshold Manager          │  │
│  │  (low, critical, shutdown)        │  │
│  └──────────────┬────────────────────┘  │
│                 │                        │
│  ┌──────────────▼────────────────────┐  │
│  │        Action Handler             │  │
│  │  (events, motor block, sleep)     │  │
│  └───────────────────────────────────┘  │
└──────────────────────────────────────────┘
```

## Configuration

```c
typedef struct {
    gpio_num_t adc_pin;               // Pin ADC pour lire la tension batterie
    adc_atten_t adc_atten;            // Atténuation ADC (défaut: ADC_ATTEN_DB_11)
    float voltage_divider_ratio;      // Ratio du diviseur de tension (ex: 2.0 pour 1:1)
    float voltage_max;                // Tension batterie pleine (ex: 4.2V Li-Ion)
    float voltage_min;                // Tension batterie vide (ex: 3.0V Li-Ion)
    uint8_t threshold_low;            // Seuil bas en % (défaut: 20)
    uint8_t threshold_critical;       // Seuil critique en % (défaut: 5)
    uint32_t sample_interval_ms;      // Intervalle de mesure (défaut: 60000 = 1min)
    uint8_t sample_average_count;     // Nombre d'échantillons pour moyenner (défaut: 16)
    bool block_motor_on_critical;     // Bloquer les moteurs si critique (défaut: true)
    bool auto_sleep_on_critical;      // Entrer en deep sleep si critique (défaut: false)
} sky_power_config_t;

#define SKY_POWER_CONFIG_LIPO_DEFAULT() { \
    .adc_pin = GPIO_NUM_34, \
    .adc_atten = ADC_ATTEN_DB_11, \
    .voltage_divider_ratio = 2.0f, \
    .voltage_max = 4.2f, \
    .voltage_min = 3.0f, \
    .threshold_low = 20, \
    .threshold_critical = 5, \
    .sample_interval_ms = 60000, \
    .sample_average_count = 16, \
    .block_motor_on_critical = true, \
    .auto_sleep_on_critical = false, \
}
```

## API

```c
esp_err_t sky_power_init(const sky_power_config_t *config);
esp_err_t sky_power_start(void);

typedef struct {
    float voltage;            // Tension actuelle (V)
    uint8_t percent;          // Niveau de charge (0-100%)
    bool charging;            // En charge (si détectable)
    bool low;                 // Sous le seuil bas
    bool critical;            // Sous le seuil critique
} sky_power_status_t;

esp_err_t sky_power_get_status(sky_power_status_t *status);
uint8_t sky_power_get_percent(void);
float sky_power_get_voltage(void);
bool sky_power_is_low(void);
bool sky_power_is_critical(void);

esp_err_t sky_power_stop(void);
void sky_power_deinit(void);
```

## Conversion tension → pourcentage

La courbe de décharge d'une batterie Li-Ion/LiPo n'est pas linéaire. On utilise une table de lookup :

```c
// Courbe de décharge typique Li-Ion (3.7V nominal)
static const struct { float voltage; uint8_t percent; } lipo_curve[] = {
    {4.20f, 100},
    {4.15f,  95},
    {4.11f,  90},
    {4.08f,  85},
    {4.02f,  80},
    {3.98f,  75},
    {3.95f,  70},
    {3.91f,  65},
    {3.87f,  60},
    {3.85f,  55},
    {3.84f,  50},
    {3.82f,  45},
    {3.80f,  40},
    {3.79f,  35},
    {3.77f,  30},
    {3.75f,  25},
    {3.73f,  20},
    {3.71f,  15},
    {3.69f,  10},
    {3.61f,   5},
    {3.27f,   0},
};
```

Interpolation linéaire entre les points de la table pour une lecture lisse.

## Intéraction avec le stepper

Quand la batterie passe en mode critique et que `block_motor_on_critical = true`, le package power installe automatiquement un hook BEFORE_MOVE sur tous les moteurs enregistrés :

```c
// Hook installé automatiquement par sky_power
void power_motor_guard(sky_stepper_hook_event_t *event, void *ctx) {
    if (sky_power_is_critical()) {
        event->cancel = true;
        sky_error_report(ESP_ERR_NOT_ALLOWED,
                              SKY_SEVERITY_WARNING,
                              "power",
                              "Motor blocked: battery critical");
    }
}
```

## Intégration Matter

Le cluster **Power Source** (0x002F) permet de reporter le niveau de batterie aux contrôleurs Matter :

| Attribut Matter | Source |
|----------------|--------|
| BatPercentRemaining | `sky_power_get_percent() * 2` (Matter utilise 0-200) |
| BatChargeLevel | OK / Warning / Critical selon les seuils |
| BatVoltage | `sky_power_get_voltage() * 1000` (mV) |
| BatChargeState | Charging / NotCharging |

## Événements publiés

| Événement | Données | Quand |
|-----------|---------|-------|
| `SKY_EVENT_BATTERY_LOW` | `{percent, voltage}` | Passage sous le seuil bas |
| `SKY_EVENT_BATTERY_CRITICAL` | `{percent, voltage}` | Passage sous le seuil critique |
| `SKY_EVENT_BATTERY_OK` | `{percent, voltage}` | Retour au-dessus du seuil bas |
| `SKY_EVENT_BATTERY_CHARGING` | `{voltage}` | Détection de charge |

## Tâches

- [ ] Implémenter la lecture ADC avec moyennage
- [ ] Implémenter la conversion voltage → pourcentage (table LiPo)
- [ ] Implémenter le threshold manager (low, critical, OK)
- [ ] Implémenter le hook moteur automatique (block on critical)
- [ ] Publier les événements batterie via l'event bus
- [ ] Intégrer avec `sky_registry` (service lifecycle)
- [ ] Préparer le mapping Matter Power Source cluster
- [ ] Créer le Kconfig (seuils, pin ADC, intervalle de mesure)

## Critères de validation

- [ ] La lecture ADC donne une tension cohérente (vérifiée au multimètre)
- [ ] Le pourcentage suit la courbe LiPo (pas linéaire)
- [ ] L'événement BATTERY_LOW se déclenche au bon seuil
- [ ] Le moteur est bloqué quand la batterie est critique
- [ ] Le niveau de batterie est visible dans l'app Google Home / Apple Home
- [ ] Le moyennage ADC filtre les pics de bruit

## Notes techniques

- **Pin ADC** : utiliser un pin ADC1 (GPIO 32-39) car ADC2 est occupé par le Wi-Fi sur l'ESP32. GPIO 34-39 sont input-only, idéal pour la lecture batterie.
- **Diviseur de tension** : la batterie LiPo (4.2V max) dépasse la range ADC de l'ESP32 (3.3V). Un diviseur R1=R2 (ratio 2:1) ramène 4.2V à 2.1V, bien dans la range avec atténuation 11dB.
- **Bruit ADC ESP32** : l'ADC de l'ESP32 est notoirement bruité (~50mV). Le moyennage sur 16 échantillons + un filtre EMA (exponential moving average) sont recommandés.
- **Détection de charge** : sans pin dédié pour la détection de charge, on peut inférer la charge par une tension qui augmente. Plus fiable avec un signal du circuit de charge (ex: pin STAT du TP4056).
