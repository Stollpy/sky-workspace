# Plan 01 — Architecture : Phase 1 — Project Scaffolding

## Objectif

Restructurer le projet pour passer d'un fichier `main.c` monolithique à une **architecture modulaire basée sur les composants ESP-IDF**. Poser les fondations réutilisables pour tous les futurs produits Sky.

## Prérequis

- Aucun (c'est la première phase)

## Contexte

Le projet actuel contient un `src/main.c` de test. On doit :
1. Adopter la structure composant ESP-IDF (`components/`)
2. Définir la convention de nommage `sky_*` pour tous les packages
3. Configurer la table de partitions pour Matter + OTA
4. Préparer le `sdkconfig` avec les options critiques

## Structure cible

```
firmware/
├── CMakeLists.txt                    # Projet racine ESP-IDF
├── platformio.ini                    # Config PlatformIO
├── partitions.csv                    # Table de partitions custom (OTA + Matter)
├── sdkconfig.defaults                # Options sdkconfig par défaut
│
├── components/                       # Packages réutilisables Sky
│   ├── sky_core/                # Framework de base (event bus, registry, errors)
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig                   # Options menuconfig
│   │   ├── include/
│   │   │   ├── sky_core.h       # API publique unifiée
│   │   │   ├── sky_event.h      # Système d'événements
│   │   │   ├── sky_registry.h   # Registre de services
│   │   │   ├── sky_error.h      # Gestion d'erreurs
│   │   │   └── sky_config.h     # Système de configuration
│   │   └── src/
│   │       ├── sky_event.c
│   │       ├── sky_registry.c
│   │       ├── sky_error.c
│   │       └── sky_config.c
│   │
│   ├── sky_stepper/             # Package moteur pas-à-pas
│   ├── sky_matter/              # Package protocole Matter
│   ├── sky_wifi/                # Package gestion Wi-Fi
│   ├── sky_ota/                 # Package mises à jour OTA
│   └── sky_power/               # Package gestion énergie
│
├── src/                              # Application spécifique (Sky Motor)
│   ├── CMakeLists.txt
│   └── main.c                        # Point d'entrée app_main()
│
└── test/                             # Tests unitaires
```

## Tâches

### 1. Table de partitions (`partitions.csv`)

Créer une table de partitions adaptée à Matter + OTA sur 4MB de flash :

```csv
# Name,    Type, SubType,  Offset,   Size,    Flags
nvs,       data, nvs,      0x9000,   0x6000,
otadata,   data, ota,      0xf000,   0x2000,
phy_init,  data, phy,      0x11000,  0x1000,
fctry,     data, nvs,      0x12000,  0x6000,
ota_0,     app,  ota_0,    0x20000,  0x1E0000,
ota_1,     app,  ota_1,    0x200000, 0x1E0000,
```

- **nvs** (24KB) : stockage clé-valeur (configs, calibration, patterns)
- **otadata** (8KB) : métadonnées OTA (quel slot est actif)
- **phy_init** (4KB) : calibration radio Wi-Fi/BLE
- **fctry** (24KB) : certificats Matter (DAC, PAI) — partition factory séparée
- **ota_0** (~1.9MB) : slot firmware principal
- **ota_1** (~1.9MB) : slot firmware secondaire

### 2. Configuration PlatformIO

Mettre à jour `platformio.ini` :

```ini
[env:sky-motor]
platform = espressif32
board = esp32dev
framework = espidf

monitor_speed = 115200
upload_speed = 460800

board_build.flash_size = 4MB
board_upload.flash_size = 4MB
board_build.partitions = partitions.csv

upload_port = /dev/cu.usbserial-110
monitor_port = /dev/cu.usbserial-110
```

### 3. Scaffold des composants

Pour chaque composant `sky_*`, créer la structure minimale :

**CMakeLists.txt** (pattern commun) :
```cmake
idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "include"
    REQUIRES <dépendances>
)
```

**Kconfig** (pattern commun) :
```kconfig
menu "Sky <Component>"
    config SKY_<COMPONENT>_ENABLED
        bool "Enable Sky <Component>"
        default y
        help
            Enable the Sky <Component> package.
endmenu
```

### 4. Conventions de nommage

| Élément | Convention | Exemple |
|---------|-----------|---------|
| Composant | `sky_<nom>` | `sky_stepper` |
| Header publique | `sky_<nom>.h` | `sky_stepper.h` |
| Préfixe fonctions | `sky_<nom>_` | `sky_stepper_init()` |
| Préfixe types | `sky_<nom>_` | `sky_stepper_config_t` |
| Préfixe Kconfig | `SKY_<NOM>_` | `SKY_STEPPER_MAX_MOTORS` |
| Tag log | `sky_<nom>` | `ESP_LOGI("sky_stepper", ...)` |

### 5. sdkconfig.defaults

Options critiques à définir :

```
# Partition table
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"

# FreeRTOS
CONFIG_FREERTOS_HZ=1000

# Watchdog
CONFIG_ESP_TASK_WDT=y
CONFIG_ESP_TASK_WDT_TIMEOUT_S=10

# Wi-Fi
CONFIG_ESP_WIFI_SOFTAP_SUPPORT=y

# BLE (pour commissioning Matter)
CONFIG_BT_ENABLED=y
CONFIG_BT_NIMBLE_ENABLED=y

# Log level
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
```

### 6. Mettre à jour `src/CMakeLists.txt`

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES sky_core
)
```

### 7. Créer un `main.c` minimal

Remplacer le test stepper par un squelette propre qui initialise le framework :

```c
#include "sky_core.h"
#include "esp_log.h"

static const char *TAG = "sky-motor";

void app_main(void)
{
    ESP_LOGI(TAG, "Sky Motor firmware starting...");

    // Initialiser le framework Sky
    sky_core_init();

    ESP_LOGI(TAG, "Sky Motor firmware ready.");
}
```

## Critères de validation

- [ ] Le projet compile avec `pio run`
- [ ] La table de partitions est correctement flashée (`pio run -t uploadfs` ou vérification via monitor)
- [ ] Chaque composant `sky_*` a sa structure de base (CMakeLists.txt, Kconfig, include/, src/)
- [ ] `main.c` appelle `sky_core_init()` et démarre sans crash
- [ ] Les logs affichent le tag approprié

## Notes techniques

- **PlatformIO + ESP-IDF** : les composants dans `components/` sont automatiquement détectés par ESP-IDF via PlatformIO. Pas besoin de configuration supplémentaire.
- **Dépendances entre composants** : utiliser `REQUIRES` et `PRIV_REQUIRES` dans le CMakeLists.txt de chaque composant pour déclarer les dépendances.
- **Taille firmware** : avec Matter + Wi-Fi + BLE, on approche ~1.9MB. Surveiller avec `pio run -t size`.
