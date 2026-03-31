# Plan 03 — Network : Phase 1 — Wi-Fi Manager (sky_wifi)

## Objectif

Créer le package `sky_wifi` : un gestionnaire Wi-Fi réutilisable qui gère la connexion, la reconnexion automatique, le provisioning (premier appairage) et publie des événements réseau pour les autres composants.

## Prérequis

- Plan 01, Phase 2 (Core Package) complétée

## Architecture

```
┌────────────────────────────────────┐
│         sky_wifi              │
├────────────────────────────────────┤
│  ┌──────────┐  ┌────────────────┐ │
│  │ Station  │  │  Provisioning  │ │
│  │ Manager  │  │  (BLE / SoftAP)│ │
│  └────┬─────┘  └───────┬────────┘ │
│       │                │          │
│  ┌────▼────────────────▼────────┐ │
│  │       Connection FSM         │ │
│  │  (disconnected → connecting  │ │
│  │   → connected → reconnecting)│ │
│  └──────────────┬───────────────┘ │
│                 │                  │
│  ┌──────────────▼───────────────┐ │
│  │    Event Publisher            │ │
│  │  (→ sky_event bus)       │ │
│  └──────────────────────────────┘ │
└────────────────────────────────────┘
```

## Configuration

```c
typedef enum {
    SKY_WIFI_PROV_NONE,       // Pas de provisioning (SSID/pass en config)
    SKY_WIFI_PROV_BLE,        // Provisioning via BLE (recommandé pour Matter)
    SKY_WIFI_PROV_SOFTAP,     // Provisioning via SoftAP + portail captif
} sky_wifi_prov_mode_t;

typedef struct {
    const char *ssid;                    // SSID (NULL si provisioning)
    const char *password;                // Mot de passe (NULL si provisioning)
    sky_wifi_prov_mode_t prov_mode; // Mode de provisioning
    const char *hostname;                // Hostname mDNS (ex: "sky-salon")
    uint8_t max_retry;                   // Tentatives de reconnexion (0 = infini)
    uint32_t retry_interval_ms;          // Intervalle entre tentatives (défaut: 5000)
    bool auto_reconnect;                 // Reconnexion automatique (défaut: true)
} sky_wifi_config_t;

#define SKY_WIFI_CONFIG_DEFAULT() { \
    .ssid = NULL, \
    .password = NULL, \
    .prov_mode = SKY_WIFI_PROV_BLE, \
    .hostname = "sky-device", \
    .max_retry = 0, \
    .retry_interval_ms = 5000, \
    .auto_reconnect = true, \
}
```

## API

```c
esp_err_t sky_wifi_init(const sky_wifi_config_t *config);
esp_err_t sky_wifi_start(void);
esp_err_t sky_wifi_stop(void);
bool sky_wifi_is_connected(void);

typedef struct {
    bool connected;
    uint8_t rssi;             // Force du signal
    char ssid[33];            // SSID connecté
    char ip[16];              // Adresse IP
    uint32_t uptime_ms;       // Temps depuis la connexion
} sky_wifi_status_t;

esp_err_t sky_wifi_get_status(sky_wifi_status_t *status);

// Forcer une reconnexion
esp_err_t sky_wifi_reconnect(void);

// Effacer les credentials Wi-Fi stockés (pour re-provisioning)
esp_err_t sky_wifi_reset_credentials(void);

void sky_wifi_deinit(void);
```

## Machine à états

```
                 ┌──────────────┐
       init()    │              │
    ────────────►│ DISCONNECTED │◄─────────────────┐
                 │              │                   │
                 └──────┬───────┘                   │
                        │                           │
               start()  │                           │ max_retry
                        │                           │ atteint
                 ┌──────▼───────┐            ┌──────┴───────┐
                 │              │  échec     │              │
                 │ CONNECTING   │───────────►│ RECONNECTING │
                 │              │            │              │
                 └──────┬───────┘            └──────┬───────┘
                        │                           │
                success │                    success│
                        │                           │
                 ┌──────▼───────────────────────────▼──┐
                 │                                      │
                 │            CONNECTED                 │
                 │                                      │
                 └──────────────┬───────────────────────┘
                                │
                       perte de │ connexion
                                │
                         ┌──────▼───────┐
                         │              │
                         │ RECONNECTING │──── retry ──►
                         │              │
                         └──────────────┘
```

## Provisioning BLE

Le provisioning BLE est le flux standard Matter : l'utilisateur scanne un QR code, l'app se connecte en BLE et fournit les credentials Wi-Fi. Le package `sky_wifi` utilise `wifi_provisioning` d'ESP-IDF.

```c
// Démarré automatiquement si aucun SSID/password en NVS
// et prov_mode = SKY_WIFI_PROV_BLE

// Les credentials reçus sont stockés en NVS automatiquement
// Au prochain boot, la connexion se fait directement
```

## Événements publiés

| Événement | Données | Quand |
|-----------|---------|-------|
| `SKY_EVENT_WIFI_CONNECTED` | `{ssid, ip, rssi}` | Connexion établie |
| `SKY_EVENT_WIFI_DISCONNECTED` | `{reason}` | Connexion perdue |
| `SKY_EVENT_WIFI_RECONNECTING` | `{attempt, max}` | Tentative de reconnexion |
| `SKY_EVENT_WIFI_PROV_START` | `{mode}` | Provisioning démarré |
| `SKY_EVENT_WIFI_PROV_DONE` | `{ssid}` | Credentials reçus |

## Persistance NVS

| Namespace | Clé | Type | Contenu |
|-----------|-----|------|---------|
| `stpwifi` | `ssid` | str | SSID Wi-Fi |
| `stpwifi` | `pass` | str | Mot de passe (chiffré si possible) |
| `stpwifi` | `prov` | u8 | Provisioning effectué (0/1) |

## Tâches

- [ ] Implémenter la connexion Wi-Fi station mode
- [ ] Implémenter la reconnexion automatique avec backoff
- [ ] Implémenter le provisioning BLE (via `wifi_provisioning` ESP-IDF)
- [ ] Persister les credentials en NVS
- [ ] Publier les événements Wi-Fi via l'event bus
- [ ] Intégrer avec `sky_registry` (service lifecycle)
- [ ] Créer le Kconfig (retry count, interval, prov mode default)
- [ ] Implémenter le hostname mDNS

## Critères de validation

- [ ] Première connexion : provisioning BLE → credentials reçus → connexion Wi-Fi
- [ ] Reboot : connexion automatique avec les credentials en NVS
- [ ] Coupure Wi-Fi : reconnexion automatique quand le réseau revient
- [ ] `sky_wifi_reset_credentials()` → re-provisioning au prochain boot
- [ ] Les événements Wi-Fi sont reçus par les subscribers (ex: Matter attend CONNECTED)

## Notes techniques

- **Ordre de démarrage** : Wi-Fi doit être initialisé avant Matter. Matter s'abonne à `WIFI_CONNECTED` pour démarrer la stack réseau.
- **Power save** : ESP-IDF supporte le Wi-Fi power save mode (modem sleep). Configurable via `esp_wifi_set_ps()`. Réduit la consommation de ~150mA à ~20mA en idle. Augmente la latence de ~1ms à ~300ms. Recommandé pour le mode batterie.
- **BLE + Wi-Fi** : sur l'ESP32-WROOM-32, BLE et Wi-Fi partagent la radio 2.4GHz. La coexistence est gérée par ESP-IDF mais introduit un overhead. Après le provisioning + commissioning Matter, le BLE peut être désactivé pour libérer des ressources.
