# Plan 03 — Network : Phase 3 — OTA Updates (sky_ota)

## Objectif

Créer le package `sky_ota` : gestion des mises à jour firmware Over-The-Air avec rollback automatique, self-test post-update et support du protocole Matter OTA.

## Prérequis

- Plan 03, Phase 2 (Matter Integration) complétée
- Plan 01, Phase 3 (Crash Safety) complétée
- Table de partitions avec ota_0 + ota_1 (Plan 01, Phase 1)

## Architecture

```
┌───────────────────────────────────────────────┐
│              sky_ota                       │
├───────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌────────────────────┐  │
│  │  OTA Manager    │  │  Self-Test Engine   │  │
│  │  (download,     │  │  (validate après    │  │
│  │   flash, reboot)│  │   update)           │  │
│  └────────┬────────┘  └────────┬───────────┘  │
│           │                    │               │
│  ┌────────▼────────────────────▼───────────┐  │
│  │           Rollback Manager              │  │
│  │  (auto-rollback si self-test échoue)    │  │
│  └─────────────────────────────────────────┘  │
│                                                │
│  ┌─────────────────┐  ┌────────────────────┐  │
│  │  HTTPS Client   │  │  Matter OTA        │  │
│  │  (pull depuis   │  │  Requestor         │  │
│  │   serveur)      │  │  (standard Matter) │  │
│  └─────────────────┘  └────────────────────┘  │
└───────────────────────────────────────────────┘
```

## Configuration

```c
typedef void (*sky_ota_self_test_t)(bool *passed, void *ctx);

typedef struct {
    const char *update_url;              // URL du serveur OTA (HTTPS)
    const char *server_cert_pem;         // Certificat TLS du serveur
    uint32_t check_interval_ms;          // Intervalle de vérification (0 = manuel)
    bool auto_update;                    // Appliquer automatiquement les updates
    bool matter_ota_enabled;             // Activer le Matter OTA Requestor
    sky_ota_self_test_t self_test;  // Callback de self-test post-update
    void *self_test_ctx;
    uint32_t self_test_timeout_ms;       // Timeout du self-test (défaut: 30000)
} sky_ota_config_t;

#define SKY_OTA_CONFIG_DEFAULT() { \
    .update_url = NULL, \
    .server_cert_pem = NULL, \
    .check_interval_ms = 0, \
    .auto_update = false, \
    .matter_ota_enabled = true, \
    .self_test = NULL, \
    .self_test_ctx = NULL, \
    .self_test_timeout_ms = 30000, \
}
```

## API

```c
esp_err_t sky_ota_init(const sky_ota_config_t *config);
esp_err_t sky_ota_start(void);

// Vérifier manuellement s'il y a une mise à jour
esp_err_t sky_ota_check(bool *update_available);

// Lancer la mise à jour manuellement
esp_err_t sky_ota_update(void);

typedef struct {
    char current_version[32];
    char available_version[32];
    bool update_available;
    bool update_in_progress;
    uint8_t download_progress;    // 0-100%
} sky_ota_status_t;

esp_err_t sky_ota_get_status(sky_ota_status_t *status);

void sky_ota_deinit(void);
```

## Self-Test Post-Update

Callback configuré par l'application pour valider que le nouveau firmware est fonctionnel :

```c
// Exemple de self-test pour Sky Motor
void sky_motor_self_test(bool *passed, void *ctx) {
    *passed = true;

    // 1. Vérifier que le Wi-Fi se connecte
    if (!sky_wifi_is_connected()) {
        ESP_LOGE(TAG, "Self-test FAILED: Wi-Fi not connected");
        *passed = false;
        return;
    }

    // 2. Vérifier que Matter démarre
    if (!sky_matter_is_commissioned()) {
        ESP_LOGW(TAG, "Self-test: Matter not commissioned (expected on first boot)");
        // Pas un échec critique
    }

    // 3. Vérifier que le moteur s'initialise
    sky_stepper_handle_t motor = sky_stepper_get("salon");
    if (motor == NULL) {
        ESP_LOGE(TAG, "Self-test FAILED: motor not found");
        *passed = false;
        return;
    }

    ESP_LOGI(TAG, "Self-test PASSED");
}
```

## Flux OTA complet

```
1. Nouvelle version disponible (serveur HTTPS ou Matter OTA Provider)
         │
2. Téléchargement dans le slot OTA inactif
         │ ← Événement SKY_EVENT_OTA_PROGRESS (0-100%)
         │
3. Vérification SHA-256 du firmware téléchargé
         │
4. Mise à jour de otadata → nouveau slot actif
         │
5. Reboot
         │
6. Bootloader démarre le nouveau firmware
         │
7. sky_ota détecte : ESP_OTA_IMG_PENDING_VERIFY
         │
8. Exécuter self_test callback
         │
    ┌────┴────┐
    ▼         ▼
  PASS      FAIL
    │         │
    ▼         ▼
  esp_ota_   esp_ota_mark_
  mark_app_  app_invalid()
  valid()    + esp_restart()
    │         │
    ▼         ▼
  OK !      Bootloader rollback
            vers l'ancien slot
```

## Sécurité OTA

```c
// Vérification d'intégrité pendant le téléchargement
// esp-matter/connectedhomeip gère la vérification pour Matter OTA

// Pour HTTPS OTA, vérifier :
// 1. Certificat TLS du serveur (pinning)
// 2. Taille du firmware (ne doit pas dépasser le slot)
// 3. SHA-256 post-download
// 4. Signature du firmware (optionnel mais recommandé en production)
```

## Événements publiés

| Événement | Données | Quand |
|-----------|---------|-------|
| `SKY_EVENT_OTA_AVAILABLE` | `{version, size}` | Nouvelle version détectée |
| `SKY_EVENT_OTA_PROGRESS` | `{percent}` | Progression du téléchargement |
| `SKY_EVENT_OTA_COMPLETE` | `{version, success}` | Mise à jour terminée |
| `SKY_EVENT_OTA_ROLLBACK` | `{from_version, to_version}` | Rollback effectué |

## Interaction avec les autres composants

Le moteur **ne doit pas être en mouvement** pendant un reboot OTA :

```c
// Avant de rebooter pour appliquer l'update :
void pre_ota_reboot(void *ctx) {
    // Arrêter tous les moteurs proprement
    sky_stepper_handle_t motors[4];
    uint8_t count;
    sky_stepper_list(motors, &count, 4);
    for (int i = 0; i < count; i++) {
        sky_stepper_stop(motors[i]);
        // Attendre que le moteur s'arrête
        while (sky_stepper_get_state(motors[i]) != SKY_STEPPER_STATE_IDLE) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
```

## Tâches

- [ ] Implémenter le OTA manager (téléchargement HTTPS vers slot inactif)
- [ ] Implémenter le self-test engine (callback + timeout + auto-rollback)
- [ ] Intégrer le Matter OTA Requestor (via esp-matter)
- [ ] Implémenter la vérification SHA-256 post-download
- [ ] Gérer le pre-reboot (arrêter les moteurs, sauvegarder l'état)
- [ ] Publier les événements OTA via l'event bus
- [ ] Intégrer avec `sky_registry` (service lifecycle)
- [ ] Créer le Kconfig (check interval, auto-update, self-test timeout)

## Critères de validation

- [ ] OTA HTTPS : téléchargement + flash + reboot + self-test PASS → firmware mis à jour
- [ ] OTA HTTPS : téléchargement + flash + reboot + self-test FAIL → rollback automatique
- [ ] Matter OTA : le hub détecte une update dispo → la pousse à l'appareil → mise à jour
- [ ] Le moteur s'arrête proprement avant le reboot OTA
- [ ] La position du moteur est correcte après le reboot (persistance NVS)
- [ ] Un firmware corrompu (SHA-256 invalide) est rejeté

## Notes techniques

- **Taille du slot OTA** : ~1.9MB. Le firmware Matter fait ~1.5-1.8MB. La marge est faible. Surveiller la taille à chaque release.
- **Durée du téléchargement** : un firmware de 1.8MB sur Wi-Fi 802.11n prend ~5-15 secondes. Le timeout de téléchargement doit être assez long (60-120s) pour les connexions lentes.
- **Matter OTA** : le flux Matter OTA utilise le BDX (Bulk Data Transfer) protocol. L'ESP32 agit en OTA Requestor, le hub (Google Home, etc.) ou un serveur dédié agit en OTA Provider.
- **Versioning** : utiliser le semantic versioning (major.minor.patch). Stocker la version dans `PROJECT_VER` du CMakeLists.txt. Le self-test peut vérifier que la version a bien changé.
