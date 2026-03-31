# OTA (Over-The-Air) — Mise à jour firmware sans fil

## Principe

L'OTA permet de mettre à jour le firmware d'un ESP32 **sans connexion physique** (USB/UART). Le nouveau firmware est téléchargé via Wi-Fi et flashé directement en mémoire. C'est indispensable pour un produit commercial déployé chez des utilisateurs.

## Architecture mémoire — Partition Table

L'ESP32 divise sa flash en **partitions**. Pour l'OTA, on utilise un schéma à double partition applicative :

```
┌──────────────────────────────────────────────┐
│                  Flash 4MB                    │
├──────────┬───────────────────────────────────┤
│ Bootloader│  0x0000 — 0x7FFF   (32 KB)       │
├──────────┼───────────────────────────────────┤
│ Part.Table│  0x8000 — 0x8FFF   (4 KB)        │
├──────────┼───────────────────────────────────┤
│   NVS    │  0x9000 — 0xEFFF   (24 KB)        │
├──────────┼───────────────────────────────────┤
│ otadata  │  0xF000 — 0x10FFF  (8 KB)         │
├──────────┼───────────────────────────────────┤
│ phy_init │  0x11000 — 0x11FFF (4 KB)         │
├──────────┼───────────────────────────────────┤
│  fctry   │  0x12000 — 0x17FFF (24 KB)        │
│ (Matter) │  Certificats d'attestation         │
├──────────┼───────────────────────────────────┤
│  ota_0   │  0x20000 — 0x1FFFFF (~1.9 MB)     │
│          │  Slot applicatif principal          │
├──────────┼───────────────────────────────────┤
│  ota_1   │  0x200000 — 0x3FFFFF (~1.9 MB)    │
│          │  Slot applicatif secondaire         │
└──────────┴───────────────────────────────────┘
```

## Fonctionnement du boot OTA

```
                    Bootloader
                        │
                        ▼
               Lire otadata partition
                        │
              ┌─────────┴─────────┐
              ▼                   ▼
        ota_0 actif ?       ota_1 actif ?
              │                   │
              ▼                   ▼
        Démarrer ota_0     Démarrer ota_1
```

1. Le **bootloader** lit la partition `otadata` pour savoir quel slot est actif
2. Il démarre le firmware du slot actif (ota_0 ou ota_1)
3. Lors d'une mise à jour, le nouveau firmware est écrit dans le slot **inactif**
4. `otadata` est mis à jour pour pointer vers le nouveau slot
5. Au prochain reboot, le bootloader démarre le nouveau firmware

## Processus de mise à jour

```
Client (serveur/app)          ESP32
       │                        │
       │   1. Vérifier version  │
       │◄───────────────────────│
       │                        │
       │   2. Envoyer firmware  │
       │───────────────────────►│
       │                        │  3. Écrire dans slot inactif
       │                        │  4. Vérifier intégrité (SHA-256)
       │                        │  5. Mettre à jour otadata
       │                        │  6. Reboot
       │                        │
       │   7. Confirmer OK      │
       │◄───────────────────────│  8. Marquer comme valide
```

## Rollback — Protection contre les mises à jour défectueuses

Le mécanisme de rollback est **critique** pour un produit commercial. Si le nouveau firmware crashe :

```
1er boot après OTA:
  ┌──────────────────────────┐
  │ Nouveau firmware démarre │
  │ État: ESP_OTA_IMG_PENDING│
  └──────────┬───────────────┘
             │
    ┌────────┴────────┐
    ▼                 ▼
  Succès            Crash / Watchdog
    │                 │
    ▼                 ▼
  esp_ota_mark_     Bootloader détecte
  app_valid()       l'échec au reboot
    │                 │
    ▼                 ▼
  ESP_OTA_IMG_      Rollback automatique
  VALID             vers l'ancien slot
```

### Conditions de rollback automatique

- Le firmware ne call pas `esp_ota_mark_app_valid()` dans un délai raisonnable
- Le watchdog se déclenche (crash loop)
- Le firmware appelle explicitement `esp_ota_mark_app_invalid()`

### Stratégie recommandée

```c
void app_main(void) {
    // Vérifier si c'est un premier boot après OTA
    esp_ota_img_states_t state;
    esp_ota_get_state_partition(esp_ota_get_running_partition(), &state);

    if (state == ESP_OTA_IMG_PENDING_VERIFY) {
        // Exécuter des self-tests (Wi-Fi, Matter, moteur, etc.)
        if (self_test_passed()) {
            esp_ota_mark_app_valid();  // Confirmer la mise à jour
        } else {
            esp_ota_mark_app_invalid(); // Forcer le rollback
            esp_restart();
        }
    }

    // ... reste de l'application
}
```

## OTA sécurisé — Signature du firmware

Pour un produit commercial, **toujours signer les firmware** pour empêcher l'injection de firmware malveillant.

### Secure Boot v2 (recommandé)

1. Générer une paire de clés RSA-3072 ou ECDSA-256
2. Signer le firmware avec la clé privée (gardée secrète)
3. La clé publique est flashée dans l'eFuse de l'ESP32 (irréversible)
4. Le bootloader vérifie la signature avant chaque boot

### App Signing (plus simple, sans eFuse)

1. Signer le binaire OTA avec une clé privée
2. Le firmware vérifie la signature avant d'accepter l'update
3. Moins sécurisé que Secure Boot mais plus flexible

## Sources OTA

### HTTP/HTTPS OTA (classique)

```c
esp_http_client_config_t config = {
    .url = "https://updates.sky.com/sky-motor/v1.2.0.bin",
    .cert_pem = server_cert,
};
esp_https_ota(&config);
```

### Matter OTA (standard domotique)

Matter définit un **OTA Provider** et un **OTA Requestor** :
- Le produit (OTA Requestor) demande périodiquement s'il y a une mise à jour
- Le hub/serveur (OTA Provider) fournit le firmware si disponible
- Le flux est standardisé et interopérable entre fabricants

### Push via BLE (provisioning initial)

Pour le premier flash en usine ou en cas de Wi-Fi indisponible, on peut utiliser le BLE pour pousser un firmware. Plus lent mais utile en fallback.

## Considérations pour notre projet

| Aspect | Choix recommandé |
|--------|-----------------|
| Partition table | Double OTA (ota_0 + ota_1) |
| Rollback | Automatique via watchdog + self-test |
| Signature | App Signing minimum, Secure Boot v2 pour production |
| Transport | Matter OTA + HTTPS fallback |
| Taille max firmware | ~1.9 MB par slot (flash 4MB) |
| Vérification | SHA-256 intégrité + signature RSA/ECDSA |
| Self-test post-OTA | Wi-Fi connect + Matter init + motor init |

## Erreurs courantes

1. **Oublier `esp_ota_mark_app_valid()`** : le firmware rollback au prochain reboot
2. **Pas de rollback** : un firmware défectueux brique l'appareil → inacceptable en production
3. **OTA non sécurisé** : n'importe qui peut flasher un firmware malveillant
4. **Pas assez de flash** : le firmware dépasse la taille du slot OTA → l'update échoue silencieusement
5. **Update pendant une opération moteur** : le store peut rester bloqué à mi-course → toujours finir l'opération en cours avant de reboot
