# CLAUDE.md — firmware/ (Luxia legacy, ESP-IDF C)

> Fichier chargé en contexte quand on travaille sous `firmware/`.
> Consolide les règles précédemment dans `firmware/.cursor/rules/*.mdc`.
> Skills détaillés : `firmware/.claude/skills/<nom>/SKILL.md`.
> Plans d'implémentation et notes d'apprentissage : `firmware/.cursor/plans/` et `firmware/.cursor/learning/` (laissés en place).

## Périmètre

Firmware PCB legacy pour **Luxia** (store connecté à moteur pas-à-pas 28BYJ-48, target ESP32-WROOM-32 classique, Matter over Wi-Fi). Construit avec **PlatformIO** (`platformio.ini` env `luxia`).

État actuel d'après les notes : **RAM ~14.8 % | Flash ~43.9 %** (bien sous le slot OTA 1.9 MB).

## Conventions Sky framework (toujours appliquer)

### Langage & style
- **C11**, **ESP-IDF v5.x**, build **PlatformIO**.
- Préfixes : `sky_` (framework) ou `luxia_` (produit spécifique).
- Retour d'erreur : **toujours `esp_err_t`**. `ESP_ERROR_CHECK()` pour les inits fatales uniquement.
- Logging : `ESP_LOG{I,W,E,D}(TAG, ...)` avec `static const char *TAG = "sky_<component>"` (ou `"luxia_<module>"`).

### Structure d'un composant
```
components/sky_<name>/
├── CMakeLists.txt          # idf_component_register(SRC_DIRS "src" INCLUDE_DIRS "include" REQUIRES sky_core)
├── Kconfig                  # menu "Sky <Name>" ... endmenu
├── include/sky_<name>.h
└── src/sky_<name>.c
```

### Lifecycle pattern (tout composant)
`init(config) → start() → stop() → deinit()`
- `init` est **idempotent** (`ESP_ERR_INVALID_STATE` si déjà init).
- Enregistrer chez `sky_registry` pour la gestion globale.
- Config par défaut via macro `SKY_<NAME>_CONFIG_DEFAULT()`.
- `sky_core_start()` appelle init+start dans l'ordre d'enregistrement, `sky_core_stop()` fait l'inverse.

### Event bus
- `sky_event_bus_config_t` : `max_subscribers=32`, `queue_size=16`, `async=true`.
- Handlers **hors mutex** (prévention deadlocks).
- Async = payload copié par `malloc` → garder les events **< 64 bytes**.
- **Jamais bloquer** dans un handler. **Jamais `sky_event_publish` en tenant un mutex du composant**.
- ID ranges : core `0x00xx`, stepper `0x01xx`, network `0x02xx`, OTA `0x028x`, power `0x03xx`.

### NVS (config persistante)
- API : `sky_config_set_*/get_*`, `sky_config_erase[_ns]`.
- **Namespace ≤ 15 chars, clé ≤ 15 chars** (contrainte NVS).
- Jamais d'écriture NVS en hot loop — persister uniquement sur changement d'état.
- `sky_config_init()` gère `nvs_flash_init` + erase-on-corrupt.

### Thread safety
- Mutex FreeRTOS pour l'état partagé.
- Queues pour commandes inter-tâches (stepper, OTA).
- Tout appel de fonction de mouvement **via queue** (jamais depuis une ISR).

### Crash safety
- `sky_crash_register(service_id, &crash_config)` par composant (si contrôle hardware).
- `safe_state_handler` **en contexte ISR** : pas de `malloc`, pas de logging, pas d'appels FreeRTOS, seulement GPIO brut.
- `sky_error_report` avec `SKY_SEVERITY_FATAL` déclenche automatiquement le handler.
- Actions : `NONE`, `LOG`, `STOP`, `SAFE_STATE`, `RESTART`, `REBOOT`.

## sky_core (components/sky_core/**)

Bootstrap + event bus + registry + NVS config + error + crash.

```c
#include "sky_core.h"  // inclut event, registry, config, crash, error

sky_core_init();
// ... registrations ...
sky_core_start();
```

Voir `firmware/.claude/skills/sky-core-usage/SKILL.md` pour l'API détaillée.

## sky_stepper (components/sky_stepper/**)

### Couches
1. **GPIO HAL** (`sky_stepper_gpio.c`) — 4 pins en sortie, inversion optionnelle.
2. **Step Engine** (`sky_stepper_engine.c`) — tables de phases (wave / full / half), logique de direction.
3. **Motor Registry** (`sky_stepper.c`) — gestion multi-moteurs, tâches FreeRTOS, commandes.

### Task pattern
- **Une tâche FreeRTOS par moteur** (`stepper_motor_task`).
- Commandes envoyées via `xQueueSend` — **jamais de mouvement depuis une ISR**.
- Types : `CMD_MOVE`, `CMD_MOVE_TO`, `CMD_SET_HEIGHT`, `CMD_STOP`, `CMD_EMERGENCY_STOP`, `CMD_CALIBRATE_*`.

### Hooks
- `run_hooks(type, event)` — si un hook fixe `event->cancel = true`, l'opération est annulée.
- Invoqués actuellement : **seulement `BEFORE_MOVE` et `AFTER_MOVE`**.
- S'exécutent **dans le contexte de la tâche moteur**.

### Calibration
- State machine dans `exec_calibrate()`, boucle de commandes interne.
- `MARK_OPEN` / `MARK_CLOSED` consommés **dans la boucle calibrate** (pas dans le main switch).
- Timeout : `CONFIG_SKY_STEPPER_CALIBRATION_TIMEOUT_S` (défaut **120 s**).
- Persistance NVS : clés `cop` (open), `ccl` (closed), `cts` (total), `pos` (position actuelle).

### NVS namespaces stepper
- Par moteur : `"stp0"`, `"stp1"`, … (index, ≤ 15 chars).
- Patterns : `"pdat"` (blob), `"pcnt"` (count).

### Moteur par défaut
**28BYJ-48** : half-step, **4096 steps/rev**, step delay 2000 µs. Ramp désactivée par défaut (activer via `sky_stepper_set_ramp()`).

Skill : `firmware/.claude/skills/sky-stepper-usage/SKILL.md`.

## sky_wifi (components/sky_wifi/**)

- **STA uniquement**. Provisioning (BLE / SoftAP) déclaré mais pas implémenté.
- Credentials NVS : namespace `"sky_wifi"`, clés `"ssid"` / `"pass"`.
- Ordre de résolution : config code → Kconfig défauts → NVS.
- Auto-reconnect : **backoff exponentiel** `base × 2^min(retry, 5)` (max 32× base, soit ~160 s avec base 5 s).
- `esp_netif_init()` + `esp_event_loop_create_default()` avec gestion gracieuse des erreurs.
- **mDNS différé** (composant externe en ESP-IDF v5.x).

Skill : `firmware/.claude/skills/sky-wifi-usage/SKILL.md`.

## sky_matter (components/sky_matter/**)

**Statut : SKELETON.** `#error` si `CONFIG_SKY_MATTER_ENABLED` est activé sans SDK. Toutes les API retournent `ESP_OK` ou `ESP_ERR_NOT_SUPPORTED`.

- Window Covering : callbacks `on_move_to_percent`, `on_open`, `on_close`, `on_stop`.
- Dev defaults : vendor `0xFFF1`, product `0x0001`, passcode `20202021`, discriminator `0xF00`.
- Intégration future : `esp-matter` SDK → endpoints réels, Power Source cluster pour batterie.

Skill : `firmware/.claude/skills/sky-matter-usage/SKILL.md`.

## sky_ota (components/sky_ota/**)

- Download HTTPS via `esp_https_ota` ; tâche priorité **5**, stack **8192**.
- Self-test **synchrone** dans `svc_start`, timer pour timeout rollback.
- `esp_ota_mark_app_valid_cancel_rollback()` on pass ; `esp_ota_mark_app_invalid_rollback_and_reboot()` on fail.
- Progress : `SKY_EVENT_OTA_PROGRESS` à chaque % changé, log tous les 10 %.
- Post-succès : `sky_core_stop()` → delay 2 s → `esp_restart()`.
- `sky_ota_check()` est un **stub** (toujours `update_available = false`).
- `sdkconfig` : `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` **obligatoire** pour que le rollback fonctionne.

Skill : `firmware/.claude/skills/sky-ota-usage/SKILL.md`.

## sky_power (components/sky_power/**)

### Battery (sky_power.c)
- **ADC1 oneshot** avec calibration (curve fitting ou line fitting, fallback raw).
- LiPo discharge curve : **table 21 points + interpolation linéaire**.
- Événements seuils **sur front montant uniquement** (low→critical, ok→low, low→ok).
- Charging detection : `voltage > prev_voltage + 0.05 V`.
- `block_motor_on_critical` et `auto_sleep_on_critical` sont des champs de config **non implémentés dans sky_power.c** — à gérer en externe (hook dans `main.c`).

### Sleep (sky_sleep.c)
- Modes : `ACTIVE` (WIFI_PS_NONE), `MODEM_SLEEP` (PS_MIN_MODEM), `LIGHT_SLEEP` (PS_MAX_MODEM), `DEEP_SLEEP`.
- **Wakelocks** : 8 slots max, ID index (`sky_wakelock_t = uint32_t`).
- Acquérir un wakelock → stoppe le timer idle, force ACTIVE.
- Relâcher le dernier wakelock → démarre le timer idle.
- Deep sleep refusé si wakelocks > 0.
- Avant deep sleep : `sky_core_stop()` + delay 500 ms (si `save_state_before_sleep`).
- `RTC_DATA_ATTR s_wake_count` survit au deep sleep, incrémenté à chaque `sky_sleep_init`.

### GPIO battery
- ADC battery : **GPIO 32–39 uniquement** (ADC1). **GPIO 34–39 = input-only** (recommandés).
- Wake button : GPIO supportant RTC (ext0 wakeup).
- **ADC2 indisponible si Wi-Fi actif.**

Skill : `firmware/.claude/skills/sky-power-usage/SKILL.md`.

## Luxia firmware — src/**

### Startup order (CRITIQUE — ne pas réordonner)
```c
void app_main(void) {
    sky_core_init();                         // 1. Framework
    sky_power_init(&power_cfg);              // 2. Battery (tôt)
    sky_wifi_init(&wifi_cfg);                // 3. Network
    sky_stepper_register(&motor_cfg, &h);    // 4. Motor + hooks
    sky_matter_init(&matter_cfg);            // 5. Protocol
    sky_matter_add_window_covering(&wc_cb);  // 5b. Endpoint
    sky_ota_init(&ota_cfg);                  // 6. Updates
    sky_sleep_init(&sleep_cfg);              // 7. Power save (dernier)
    luxia_cmd_init(h);                       // 8. Command API
    sky_event_subscribe(...);                // 9. Event bridges
    sky_core_start();                        // 10. Démarre tous les services
}
```

### Bridges (main.c)
- **Stepper → Matter** : `on_position_event` abonné à `SKY_EVENT_STEPPER_POSITION` → `sky_matter_wc_update_position` + `update_status`.
- **Matter → Stepper** : `wc_callbacks` (open/close/move_to/stop) → `sky_stepper_set_height` / `sky_stepper_stop`.
- **Power → Stepper** : `battery_motor_guard` hook sur `BEFORE_MOVE` → `event->cancel = true` si `sky_power_is_critical()`.

### GPIO pinout Luxia PCB
| Fonction | GPIO | Notes |
|----------|------|-------|
| Stepper IN1 | 17 | |
| Stepper IN2 | 16 | |
| Stepper IN3 | 4  | |
| Stepper IN4 | 0  | **Boot mode pin — attention** |
| Battery ADC | 34 | ADC1, input-only |
| Wake button | 35 | input-only, pull-up externe |

### luxia_cmd (commandes produit-spécifiques)
Bridge API custom ↔ composants `sky_*`. `luxia_cmd_init(motor_handle)` mémorise la ref moteur pour tous les handlers.

| Catégorie | IDs | Commandes |
|-----------|-----|-----------|
| Calibration | `0x01`–`0x05` | start, mark_open, mark_closed, cancel, status |
| Presets | `0x10`–`0x13` | save, apply, delete, list |
| Sequences | `0x20`–`0x28` | record, play, stop, list, export, import |
| Diagnostics | `0x40`–`0x46` | status, reboot, factory_reset, version, ota, set_name |

## Build targets

```bash
pio run -e luxia                 # Build
pio run -e luxia -t upload       # Flash
pio run -e luxia -t monitor      # Serial monitor
pio run -e luxia -t menuconfig   # Kconfig UI
pio run -e luxia -t erase        # Erase flash
```

## Ajouter un nouveau composant Sky

Voir **`firmware/.claude/skills/sky-new-component/SKILL.md`** — guide complet en 7 étapes + checklist (headers, impl, Kconfig, CMakeLists, events, crash handler, intégration `src/CMakeLists.txt`).

## Plans d'implémentation & apprentissage

- `firmware/.cursor/plans/00-init/` — init projet + architecture, stepper, network, power, sky-motor
- `firmware/.cursor/plans/01-http-server/` — HTTP server component
- `firmware/.cursor/plans/04-smartwatch/` — BOM, proto, drivers, connectivity, UI, KiCad blueprint, cost production
- `firmware/.cursor/plans/05-sky-container/` — BOM Sky container + matrice LLM OPi5+
- `firmware/.cursor/plans/07-sky-hub/` — init ask
- `firmware/.cursor/learning/` — notes stepper motor, OTA, Matter

## Debug UART côté hub (quand un firmware tire sur un bus série vers la SBC)

Pour inspecter l'autre bout du bus (device-service sur le hub qui lit `/dev/ttyAMA0`) :
voir le skill `.claude/skills/sky-hub-ssh/SKILL.md` et le guide
`docs/logiciel/developpement/sky-hub-ssh.md`. État live du hardware hub (RPi4B /
OPi5+ / autre) dans `.claude/context/sky-hub-state.md`.
