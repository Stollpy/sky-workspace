# Plan 03 — Network : Phase 2 — Matter Integration (sky_matter)

## Objectif

Créer le package `sky_matter` : abstraction du protocole Matter via le SDK `esp-matter`. Le package expose une API simple pour déclarer des endpoints et mapper des clusters à des callbacks applicatifs, sans que l'application ait besoin de connaître les détails de connectedhomeip.

## Prérequis

- Plan 03, Phase 1 (Wi-Fi Manager) complétée
- Plan 01, Phase 2 (Core Package) complétée

## Architecture

```
┌──────────────────────────────────────────────┐
│             Application (Sky Motor)           │
│   sky_matter_add_window_covering(...)    │
├──────────────────────────────────────────────┤
│             sky_matter                    │
│  ┌─────────────────────────────────────────┐ │
│  │  Device Type Helpers                     │ │
│  │  (window_covering, temperature, etc.)    │ │
│  ├─────────────────────────────────────────┤ │
│  │  Attribute Bridge                        │ │
│  │  (map Matter attributes ↔ app callbacks) │ │
│  ├─────────────────────────────────────────┤ │
│  │  Commissioning Manager                   │ │
│  │  (QR code, BLE, PASE, CASE)             │ │
│  ├─────────────────────────────────────────┤ │
│  │  esp-matter SDK (connectedhomeip)        │ │
│  └─────────────────────────────────────────┘ │
└──────────────────────────────────────────────┘
```

## Configuration

```c
typedef struct {
    const char *device_name;          // Nom affiché (ex: "Sky Store Salon")
    uint16_t vendor_id;               // Vendor ID CSA (0xFFF1 = test)
    uint16_t product_id;              // Product ID (ex: 0x0001 = Sky Motor)
    const char *serial_number;        // Numéro de série unique
    const char *firmware_version;     // Version firmware (ex: "1.0.0")
    uint32_t setup_passcode;          // Code d'appairage (ex: 20202021)
    uint16_t discriminator;           // Discriminator BLE (12-bit)
} sky_matter_config_t;

#define SKY_MATTER_CONFIG_DEV_DEFAULT() { \
    .device_name = "Sky Device", \
    .vendor_id = 0xFFF1, \
    .product_id = 0x0001, \
    .serial_number = "STP-000001", \
    .firmware_version = "0.1.0", \
    .setup_passcode = 20202021, \
    .discriminator = 0xF00, \
}
```

## API — Core

```c
esp_err_t sky_matter_init(const sky_matter_config_t *config);
esp_err_t sky_matter_start(void);
esp_err_t sky_matter_stop(void);
bool sky_matter_is_commissioned(void);

// Obtenir le QR code pour le commissioning
esp_err_t sky_matter_get_qr_code(char *buf, size_t buf_size);

// Obtenir le manual pairing code
esp_err_t sky_matter_get_manual_code(char *buf, size_t buf_size);

// Factory reset (effacer tous les fabrics et recommencer)
esp_err_t sky_matter_factory_reset(void);

void sky_matter_deinit(void);
```

## API — Device Type Helpers

Fonctions de haut niveau pour ajouter des device types standard sans manipuler les clusters directement.

### Window Covering (stores)

```c
typedef struct {
    // Callbacks appelés quand Matter demande une action
    esp_err_t (*on_move_to_percent)(uint8_t percent, void *ctx);
    esp_err_t (*on_open)(void *ctx);
    esp_err_t (*on_close)(void *ctx);
    esp_err_t (*on_stop)(void *ctx);
    void *ctx;
} sky_matter_wc_callbacks_t;

typedef struct sky_matter_wc *sky_matter_wc_handle_t;

// Ajouter un endpoint Window Covering
esp_err_t sky_matter_add_window_covering(
    const sky_matter_wc_callbacks_t *callbacks,
    sky_matter_wc_handle_t *handle);

// Mettre à jour la position actuelle (notifie les contrôleurs Matter)
esp_err_t sky_matter_wc_update_position(
    sky_matter_wc_handle_t handle,
    uint8_t current_percent);

// Mettre à jour le statut opérationnel
typedef enum {
    SKY_MATTER_WC_STATUS_STOPPED,
    SKY_MATTER_WC_STATUS_OPENING,
    SKY_MATTER_WC_STATUS_CLOSING,
} sky_matter_wc_status_t;

esp_err_t sky_matter_wc_update_status(
    sky_matter_wc_handle_t handle,
    sky_matter_wc_status_t status);
```

### Template pour futurs device types

```c
// Capteur de température (futur)
esp_err_t sky_matter_add_temperature_sensor(
    sky_matter_temp_callbacks_t *callbacks,
    sky_matter_temp_handle_t *handle);

// On/Off switch (futur)
esp_err_t sky_matter_add_on_off(
    sky_matter_onoff_callbacks_t *callbacks,
    sky_matter_onoff_handle_t *handle);
```

## Mapping Matter ↔ Stepper

Le lien entre Matter et le moteur se fait dans l'application (pas dans les packages) :

```c
// === Dans main.c (Sky Motor) ===

static sky_stepper_handle_t motor;
static sky_matter_wc_handle_t wc;

static esp_err_t on_move_to_percent(uint8_t percent, void *ctx) {
    return sky_stepper_set_height(motor, percent);
}

static esp_err_t on_open(void *ctx) {
    return sky_stepper_set_height(motor, 0);
}

static esp_err_t on_close(void *ctx) {
    return sky_stepper_set_height(motor, 100);
}

static esp_err_t on_stop(void *ctx) {
    return sky_stepper_stop(motor);
}

// Écouter les changements de position du moteur pour notifier Matter
void on_stepper_position(const sky_event_t *event, void *ctx) {
    sky_stepper_position_event_t *pos = event->data;
    sky_matter_wc_update_position(wc, pos->percent);
}
```

## Commissioning Flow

```
1. Premier boot (pas de fabric stocké)
   │
   ├── sky_matter_start() détecte : pas commissionné
   │
   ├── Afficher QR code en log (+ via sky_matter_get_qr_code)
   │   Ex: "MT:Y.K90AFN00KA0648G00"
   │
   ├── BLE advertising démarre (discriminator dans le nom)
   │
   ├── Utilisateur scanne le QR code avec Google Home / Apple Home
   │
   ├── App se connecte en BLE → PASE session → Wi-Fi credentials
   │
   ├── ESP32 se connecte au Wi-Fi (via sky_wifi)
   │
   ├── CASE session → fabric rejoint → NOC installé
   │
   ├── Événement SKY_EVENT_MATTER_READY publié
   │
   └── Appareil opérationnel, contrôlable via l'app

2. Boots suivants (fabric en NVS)
   │
   ├── sky_wifi_start() → connexion auto
   │
   ├── sky_matter_start() → charge les fabrics depuis NVS
   │
   └── Opérationnel immédiatement
```

## Événements publiés

| Événement | Données | Quand |
|-----------|---------|-------|
| `SKY_EVENT_MATTER_READY` | `{commissioned, fabric_count}` | Stack Matter prête |
| `SKY_EVENT_MATTER_COMMAND` | `{endpoint, cluster, command}` | Commande reçue |
| `SKY_EVENT_MATTER_COMMISSIONED` | `{fabric_id}` | Nouveau fabric ajouté |
| `SKY_EVENT_MATTER_DECOMMISSIONED` | `{fabric_id}` | Fabric retiré |

## Structure des fichiers

```
components/sky_matter/
├── CMakeLists.txt
├── Kconfig
├── include/
│   ├── sky_matter.h              # API publique
│   ├── sky_matter_wc.h           # Helper Window Covering
│   └── sky_matter_types.h        # Types partagés
└── src/
    ├── sky_matter.c              # Init, start, commissioning
    ├── sky_matter_wc.c           # Window Covering endpoint
    └── sky_matter_attribute.c    # Bridge attributs ↔ callbacks
```

## Tâches

- [ ] Intégrer le SDK `esp-matter` comme dépendance du composant
- [ ] Implémenter `sky_matter_init/start` (node creation, commissioning)
- [ ] Implémenter le helper Window Covering (endpoint + cluster + attributs)
- [ ] Implémenter le bridge attributs ↔ callbacks (attribute_update_cb)
- [ ] Générer et logger le QR code de commissioning
- [ ] Publier les événements Matter via l'event bus
- [ ] Intégrer avec `sky_wifi` (attendre CONNECTED avant de démarrer)
- [ ] Intégrer avec `sky_registry` (service lifecycle)
- [ ] Créer le Kconfig (vendor_id, product_id, passcode defaults)
- [ ] Tester le commissioning complet avec Google Home ou Apple Home

## Critères de validation

- [ ] QR code scannable depuis Google Home / Apple Home
- [ ] Commissioning BLE → Wi-Fi → operational réussi
- [ ] La commande "Ouvrir" dans l'app déclenche `on_open()` callback
- [ ] La commande "50%" dans l'app déclenche `on_move_to_percent(50)`
- [ ] La mise à jour de position via `wc_update_position()` se reflète dans l'app en temps réel
- [ ] Après reboot, l'appareil se reconnecte automatiquement au fabric
- [ ] Multi-fabric : ajout à Google Home ET Apple Home simultanément

## Notes techniques

- **esp-matter SDK** : le SDK est en C++ (basé sur connectedhomeip). Notre wrapper `sky_matter` expose une API C pour rester cohérent avec le reste du framework. Intern, on utilisera `extern "C"` pour les fichiers d'implémentation.
- **RAM** : Matter + Wi-Fi + BLE = ~170KB RAM. L'ESP32-WROOM-32 a 520KB, donc ~350KB restent pour l'application et les tasks moteur. C'est suffisant mais il faut éviter les allocations dynamiques excessives.
- **Taille firmware** : avec esp-matter, le firmware fera ~1.5-1.8MB. Ça passe dans un slot OTA de 1.9MB, mais de justesse. Activer les optimisations de taille (`CONFIG_COMPILER_OPTIMIZATION_SIZE=y`).
- **Certificats de test** : en développement, utiliser `CONFIG_ESP_MATTER_DAC_PROVIDER` avec les certificats de test fournis par le SDK. Pour la production, il faudra obtenir un Vendor ID/Product ID officiel auprès de la CSA et flasher les DAC dans la partition `fctry`.
