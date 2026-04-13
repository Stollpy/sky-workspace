# Phase 4 — Firmware : Connectivité

## Objectif

Implémenter les couches de communication : **BLE** (notifications, data sync),
**Wi-Fi** (OTA, sync cloud), **LTE** (cellulaire via AT commands), et
**Bluetooth audio** (streaming vers écouteurs/speaker).

---

## 1. `sky_ble` — BLE GATT Server / Client

| Détail | Valeur |
|--------|--------|
| Stack | ESP-IDF NimBLE (léger, adapté wearable) |
| Rôle | GATT Server (expose données capteurs) + GATT Client (optionnel) |
| Dépendance | `sky_core` |

**Fonctionnalités :**
- Service GATT « Heart Rate » (standard BLE HR Profile, UUID 0x180D)
- Service GATT custom pour SpO2, IMU, GPS
- Notifications push vers app mobile
- Pairing / bonding sécurisé (LE Secure Connections)
- Publication événements : `SKY_EVENT_BLE_CONNECTED`, `SKY_EVENT_BLE_DISCONNECTED`

**Dev estimé : 2–3 semaines**

---

## 2. `sky_wifi` (existant) — Adaptation montre

Le composant `sky_wifi` existe déjà dans le framework Sky.

**Adaptations spécifiques montre :**
- Politique agressive de coupure Wi-Fi (Wi-Fi OFF par défaut, ON uniquement pour OTA / sync)
- Portail captif pour config initiale (provisioning via BLE préféré)
- Intégration `sky_power` : Wi-Fi éteint en mode basse conso

**Dev estimé : 1 semaine (adaptation)**

---

## 3. `sky_lte` — Modem cellulaire LTE-M (SIM7080G)

| Détail | Valeur |
|--------|--------|
| Module | SIM7080G (ou Quectel BG95-M1) |
| Interface | UART, commandes AT |
| Dépendance | `sky_core` |

**Fonctionnalités :**
- Machine à états : OFF → INIT → REGISTERED → CONNECTED → DATA
- Couche AT parser (envoi commande AT, parse réponse, timeout)
- Enregistrement réseau LTE-M / NB-IoT
- Stack TCP/IP via le modem (AT+CIPSTART, etc.)
- HTTP(S) client via modem pour push data cloud
- SMS envoi/réception (optionnel)
- Publication événements : `SKY_EVENT_LTE_REGISTERED`, `SKY_EVENT_LTE_DATA_READY`
- Gestion power : `AT+CFUN=0` pour mode avion, power pin control

**Dev estimé : 3–4 semaines**

### Structure AT parser

```
components/sky_lte/
├── CMakeLists.txt
├── Kconfig
├── include/
│   └── sky_lte.h
└── src/
    ├── sky_lte.c         ← lifecycle, state machine
    ├── sky_lte_at.c      ← AT command send/parse engine
    ├── sky_lte_net.c     ← TCP/IP, HTTP over modem
    └── sky_lte_sms.c     ← SMS (optionnel)
```

---

## 4. Audio Bluetooth (A2DP / LE Audio)

### Problème

L'ESP32-**S3** ne supporte **pas** Bluetooth Classic.
Streaming audio vers écouteurs/speaker BT classiques (A2DP) n'est donc **pas natif**.

### Solutions

| Option | Implémentation | Complexité |
|--------|---------------|------------|
| **A — Module BT externe** | KCX_BT_EMITTER branché en I2S, piloté via GPIO/UART. L'ESP32-S3 envoie l'audio en I2S au module qui le stream en A2DP | Faible |
| **B — BLE Audio (LE Audio / LC3)** | Standard plus récent, supporté par ESP32-S3 (en cours de support ESP-IDF). Nécessite écouteurs compatibles LE Audio | Moyen, dépend du support ESP-IDF |
| **C — ESP32 classique en co-processeur** | Second chip ESP32 (pas S3) dédié au BT Classic, communique avec le S3 en SPI/UART | Élevé |

**Recommandation proto** : Option A (module externe) pour valider le concept.
**Recommandation PCB custom** : Option C (ESP32 classique co-processeur) ou attendre support mature LE Audio.

**Dev estimé : 1–2 semaines (option A)**

---

## 5. Event ID Ranges

| Range | Composant |
|-------|-----------|
| 0x0200–0x020F | `sky_ble` |
| 0x0210–0x021F | `sky_wifi` (existant) |
| 0x0250–0x026F | `sky_lte` |
| 0x0270–0x027F | BT Audio |

---

## 6. Ordre de développement

```
sky_wifi (adapt) ──► sky_ble ──► BT Audio (module ext) ──► sky_lte
   (1 sem)           (2-3 sem)      (1-2 sem)              (3-4 sem)
```

Commencer par Wi-Fi (déjà fonctionnel, juste adapter la politique power),
puis BLE (cœur de la communication montre ↔ téléphone),
puis audio BT (module externe),
puis LTE en dernier (le plus complexe, pas indispensable pour le MVP).
