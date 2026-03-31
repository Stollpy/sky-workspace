# Plan 01 — Architecture : Phase 2 — Core Package (sky_core)

## Objectif

Implémenter le package `sky_core` : le **framework de base** que tous les composants Sky utilisent. Il fournit l'event bus, le registre de services, la gestion d'erreurs et le système de configuration.

## Prérequis

- Phase 1 (Project Scaffolding) complétée

## Composants du package

### 1. Event Bus (`sky_event`)

Système publish/subscribe pour la communication **découplée** entre composants. Inspiré du pattern Observer.

```c
// --- Types ---

typedef uint32_t sky_event_id_t;

typedef struct {
    sky_event_id_t event_id;
    void *data;
    size_t data_size;
    uint32_t timestamp_ms;
} sky_event_t;

typedef void (*sky_event_handler_t)(const sky_event_t *event, void *ctx);

typedef struct {
    uint16_t max_subscribers;      // Défaut: 32
    uint16_t queue_size;           // Défaut: 16
    bool async;                    // true = dispatch via FreeRTOS queue
} sky_event_bus_config_t;

// --- API ---

esp_err_t sky_event_bus_init(const sky_event_bus_config_t *config);
esp_err_t sky_event_subscribe(sky_event_id_t event_id,
                                    sky_event_handler_t handler,
                                    void *ctx);
esp_err_t sky_event_unsubscribe(sky_event_id_t event_id,
                                      sky_event_handler_t handler);
esp_err_t sky_event_publish(sky_event_id_t event_id,
                                  const void *data,
                                  size_t data_size);
void sky_event_bus_deinit(void);
```

**Événements prédéfinis (sky_events.h)** :

```c
// Core events (0x0000 - 0x00FF)
#define SKY_EVENT_SYSTEM_READY       0x0001
#define SKY_EVENT_SYSTEM_ERROR       0x0002
#define SKY_EVENT_SYSTEM_SHUTDOWN    0x0003

// Stepper events (0x0100 - 0x01FF)
#define SKY_EVENT_STEPPER_STARTED    0x0100
#define SKY_EVENT_STEPPER_STOPPED    0x0101
#define SKY_EVENT_STEPPER_POSITION   0x0102
#define SKY_EVENT_STEPPER_ERROR      0x0103
#define SKY_EVENT_STEPPER_CALIBRATED 0x0104

// Network events (0x0200 - 0x02FF)
#define SKY_EVENT_WIFI_CONNECTED     0x0200
#define SKY_EVENT_WIFI_DISCONNECTED  0x0201
#define SKY_EVENT_MATTER_READY       0x0210
#define SKY_EVENT_MATTER_COMMAND     0x0211

// Power events (0x0300 - 0x03FF)
#define SKY_EVENT_BATTERY_LOW        0x0300
#define SKY_EVENT_BATTERY_CRITICAL   0x0301
#define SKY_EVENT_SLEEP_ENTER        0x0310
#define SKY_EVENT_SLEEP_EXIT         0x0311
```

### 2. Service Registry (`sky_registry`)

Registre centralisé des services/composants actifs. Permet à n'importe quel composant de retrouver un autre composant par son identifiant. Inspiré du Service Locator pattern.

```c
// --- Types ---

typedef const char* sky_service_id_t;

typedef struct {
    esp_err_t (*init)(void *config);
    esp_err_t (*start)(void);
    esp_err_t (*stop)(void);
    esp_err_t (*deinit)(void);
} sky_service_lifecycle_t;

typedef struct {
    sky_service_id_t id;
    sky_service_lifecycle_t lifecycle;
    void *instance;
    void *config;
    bool initialized;
    bool running;
} sky_service_entry_t;

typedef struct {
    uint8_t max_services;   // Défaut: 16
} sky_registry_config_t;

// --- API ---

esp_err_t sky_registry_init(const sky_registry_config_t *config);
esp_err_t sky_registry_register(sky_service_id_t id,
                                      const sky_service_lifecycle_t *lifecycle,
                                      void *config);
void *sky_registry_get(sky_service_id_t id);
esp_err_t sky_registry_start_all(void);
esp_err_t sky_registry_stop_all(void);
void sky_registry_deinit(void);
```

**Usage typique (pattern Fortify-like)** :

```c
// Dans le package sky_stepper :
sky_service_lifecycle_t stepper_lifecycle = {
    .init = stepper_service_init,
    .start = stepper_service_start,
    .stop = stepper_service_stop,
    .deinit = stepper_service_deinit,
};

// Dans main.c (l'application) :
sky_stepper_config_t motor_config = { /* ... */ };
sky_registry_register("stepper", &stepper_lifecycle, &motor_config);
sky_registry_start_all();  // Initialise et démarre tout dans l'ordre
```

### 3. Error Handling (`sky_error`)

Gestion d'erreurs structurée avec niveaux de sévérité et hooks configurables.

```c
// --- Types ---

typedef enum {
    SKY_SEVERITY_INFO,
    SKY_SEVERITY_WARNING,
    SKY_SEVERITY_ERROR,
    SKY_SEVERITY_CRITICAL,
    SKY_SEVERITY_FATAL,
} sky_error_severity_t;

typedef struct {
    esp_err_t code;
    sky_error_severity_t severity;
    const char *component;
    const char *message;
    uint32_t timestamp_ms;
} sky_error_t;

typedef void (*sky_error_handler_t)(const sky_error_t *error, void *ctx);

// --- API ---

esp_err_t sky_error_init(void);
esp_err_t sky_error_set_handler(sky_error_severity_t min_severity,
                                      sky_error_handler_t handler,
                                      void *ctx);
void sky_error_report(esp_err_t code,
                           sky_error_severity_t severity,
                           const char *component,
                           const char *message);
```

### 4. Configuration System (`sky_config`)

Abstraction au-dessus de NVS pour stocker/lire des configurations de manière typée.

```c
// --- Types ---

typedef const char* sky_config_ns_t;  // Namespace NVS

// --- API ---

esp_err_t sky_config_init(void);
esp_err_t sky_config_set_u32(sky_config_ns_t ns, const char *key, uint32_t value);
esp_err_t sky_config_get_u32(sky_config_ns_t ns, const char *key, uint32_t *value);
esp_err_t sky_config_set_i32(sky_config_ns_t ns, const char *key, int32_t value);
esp_err_t sky_config_get_i32(sky_config_ns_t ns, const char *key, int32_t *value);
esp_err_t sky_config_set_str(sky_config_ns_t ns, const char *key, const char *value);
esp_err_t sky_config_get_str(sky_config_ns_t ns, const char *key, char *buf, size_t len);
esp_err_t sky_config_set_blob(sky_config_ns_t ns, const char *key,
                                    const void *data, size_t len);
esp_err_t sky_config_get_blob(sky_config_ns_t ns, const char *key,
                                    void *data, size_t *len);
esp_err_t sky_config_erase(sky_config_ns_t ns, const char *key);
esp_err_t sky_config_erase_ns(sky_config_ns_t ns);
void sky_config_deinit(void);
```

### 5. API unifiée (`sky_core.h`)

Point d'entrée unique qui initialise tous les sous-systèmes :

```c
typedef struct {
    sky_event_bus_config_t event_bus;
    sky_registry_config_t registry;
} sky_core_config_t;

#define SKY_CORE_DEFAULT_CONFIG() { \
    .event_bus = { .max_subscribers = 32, .queue_size = 16, .async = true }, \
    .registry = { .max_services = 16 }, \
}

esp_err_t sky_core_init(void);
esp_err_t sky_core_init_with_config(const sky_core_config_t *config);
esp_err_t sky_core_start(void);
esp_err_t sky_core_stop(void);
void sky_core_deinit(void);
```

## Tâches

- [ ] Implémenter `sky_event` (event bus pub/sub avec FreeRTOS queue)
- [ ] Implémenter `sky_registry` (service locator avec lifecycle hooks)
- [ ] Implémenter `sky_error` (error reporting avec severity levels)
- [ ] Implémenter `sky_config` (wrapper NVS typé)
- [ ] Implémenter `sky_core` (init unifiée)
- [ ] Créer le Kconfig pour les options configurables via menuconfig
- [ ] Écrire les headers publiques avec documentation Doxygen

## Critères de validation

- [ ] `sky_core_init()` initialise event bus + registry + config + error handling sans crash
- [ ] On peut publier un événement et le recevoir dans un subscriber
- [ ] On peut enregistrer un service et le retrouver via `sky_registry_get()`
- [ ] Les configs NVS persistent entre les reboots
- [ ] Les erreurs CRITICAL déclenchent le handler configuré

## Notes techniques

- L'event bus **async** utilise une FreeRTOS queue + une task dédiée. L'event bus **sync** appelle les handlers directement dans le contexte de l'appelant (attention aux deadlocks).
- Le registry a un ordre d'initialisation implicite : les services sont `init()` dans l'ordre d'enregistrement, puis `start()` dans le même ordre. Le `stop()` se fait en ordre **inverse**.
- La config NVS utilise des namespaces pour isoler les données de chaque composant (ex: `stepper`, `matter`, `wifi`).
