# Matter — Le protocole domotique universel

## Qu'est-ce que Matter ?

Matter est un **protocole applicatif open-source** pour la domotique, créé par la **CSA** (Connectivity Standards Alliance, ex-Zigbee Alliance). Il est soutenu par Apple, Google, Amazon et Samsung.

L'objectif : un appareil domotique Matter fonctionne avec **tous les écosystèmes** (Apple Home, Google Home, Amazon Alexa, Samsung SmartThings, Home Assistant) sans configuration spécifique.

## Architecture du protocole

```
┌─────────────────────────────────────────────────────┐
│                   Application                        │
│            (Window Covering, Thermostat, etc.)       │
├─────────────────────────────────────────────────────┤
│                  Data Model                          │
│         (Clusters, Attributs, Commandes)             │
├─────────────────────────────────────────────────────┤
│              Interaction Model                       │
│       (Read, Write, Subscribe, Invoke)               │
├─────────────────────────────────────────────────────┤
│                   Security                           │
│     (CASE sessions, certificats, chiffrement)        │
├─────────────────────────────────────────────────────┤
│              Message Layer                           │
│         (MRP — reliable messaging)                   │
├─────────────────────────────────────────────────────┤
│              Transport                               │
│        ┌──────────┬──────────┬──────────┐           │
│        │  Wi-Fi   │  Thread  │ Ethernet │           │
│        └──────────┴──────────┴──────────┘           │
│                      + BLE                           │
│              (commissioning uniquement)               │
└─────────────────────────────────────────────────────┘
```

## Concepts clés

### Fabric (réseau Matter)

Un **fabric** est un réseau logique de confiance. Quand tu ajoutes un appareil via Google Home, il rejoint le fabric Google. Un même appareil peut être dans **plusieurs fabrics** simultanément (multi-admin).

```
         ┌─── Fabric Apple Home ───┐
         │                         │
ESP32 ───┼─── Fabric Google Home ──┼─── Contrôleurs
Store    │                         │
         └─── Fabric Home Asst. ───┘
```

### Node, Endpoint, Cluster

```
Node (= un appareil physique, ex: ESP32)
 │
 ├── Endpoint 0 : Root Node (obligatoire)
 │    ├── Cluster: Basic Information (nom, fabricant, etc.)
 │    ├── Cluster: Network Commissioning
 │    ├── Cluster: OTA Software Update Requestor
 │    └── Cluster: General Diagnostics
 │
 └── Endpoint 1 : Window Covering (notre store)
      ├── Cluster: Window Covering (0x0102)
      │    ├── Attributs:
      │    │    ├── CurrentPositionLiftPercent100ths (position actuelle)
      │    │    ├── TargetPositionLiftPercent100ths (position cible)
      │    │    ├── OperationalStatus (en mouvement/arrêté)
      │    │    ├── InstalledOpenLimitLift
      │    │    └── InstalledClosedLimitLift
      │    └── Commandes:
      │         ├── UpOrOpen
      │         ├── DownOrClose
      │         ├── StopMotion
      │         └── GoToLiftPercentage(percent)
      ├── Cluster: Identify
      └── Cluster: Groups
```

### Device Type : Window Covering (0x0202)

C'est **exactement** ce qu'il nous faut. Le standard Matter définit déjà :
- Ouvrir / fermer complètement
- Aller à un pourcentage précis (0-100%)
- Arrêter le mouvement
- Reporter la position actuelle en temps réel
- Signaler le statut opérationnel (en train d'ouvrir, fermer, arrêté)

Aucun besoin d'inventer un protocole custom. Les apps (Apple Home, Google Home) ont déjà l'UI pour contrôler un Window Covering.

## Commissioning (appairage)

Le commissioning est le processus d'ajout d'un appareil au réseau Matter.

```
1. Utilisateur scanne le QR code sur l'appareil
         │
         ▼
2. L'app (Google Home, etc.) se connecte en BLE
         │
         ▼
3. Échange de clés PASE (Passcode-Authenticated Session)
   └── Utilise le setup code (ex: 20202021)
         │
         ▼
4. L'app fournit les credentials Wi-Fi
         │
         ▼
5. L'ESP32 se connecte au Wi-Fi
         │
         ▼
6. Session CASE (Certificate-Authenticated Session)
   └── Certificats échangés, fabric rejoint
         │
         ▼
7. Appareil opérationnel dans le fabric
```

### Éléments nécessaires pour le commissioning

| Élément | Description |
|---------|------------|
| **Discriminator** | Identifiant 12-bit pour différencier les appareils en BLE |
| **Setup Passcode** | Code à 8 chiffres (ex: 20202021) pour l'appairage PASE |
| **QR Code / Manual Code** | Encode discriminator + passcode + vendor/product ID |
| **DAC** (Device Attestation Certificate) | Certificat prouvant que l'appareil est un vrai produit Matter |
| **PAI** (Product Attestation Intermediate) | Certificat intermédiaire signé par la CSA |

### DAC — Device Attestation Certificate

Pour un **produit commercial**, il faut obtenir un **Vendor ID** et un **Product ID** auprès de la CSA. Pendant le développement, on utilise des certificats de test fournis par le SDK.

## ESP-Matter SDK

Espressif fournit **esp-matter**, un composant ESP-IDF basé sur le projet open-source **connectedhomeip** (CHIP).

### Installation

```
# Le SDK esp-matter est un composant ESP-IDF
# Il se gère via le composant manager ou en submodule

# Dépendances système
idf.py add-dependency "espressif/esp_matter"
```

### Structure d'une app Matter minimale

```c
#include <esp_matter.h>
#include <esp_matter_endpoint.h>

using namespace esp_matter;
using namespace chip::app::Clusters;

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg) {
    // Réagir aux événements Matter (fabric join, commission, etc.)
}

static esp_err_t app_attribute_update_cb(
    attribute::callback_type_t type,
    uint16_t endpoint_id,
    uint32_t cluster_id,
    uint32_t attribute_id,
    esp_matter_attr_val_t *val,
    void *priv_data)
{
    // Callback quand un attribut Matter est modifié
    // Ex: GoToLiftPercentage → mettre à jour la position moteur
}

void app_main(void) {
    // 1. Créer le node Matter
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, NULL);

    // 2. Ajouter un endpoint Window Covering
    window_covering::config_t wc_config;
    endpoint_t *endpoint = window_covering::create(node, &wc_config,
                                                    ENDPOINT_FLAG_NONE, NULL);

    // 3. Démarrer Matter
    esp_matter::start(app_event_cb);
}
```

### Mapping Matter ↔ Moteur

```
Matter Command              →  Action Moteur
────────────────────────────────────────────
UpOrOpen                    →  stepper_move(motor, 0, CW)
DownOrClose                 →  stepper_move(motor, max_steps, CCW)
GoToLiftPercentage(50%)     →  stepper_move_to(motor, max_steps * 0.5)
StopMotion                  →  stepper_stop(motor)

Motor Event                 →  Matter Attribute Update
────────────────────────────────────────────
Position changed            →  CurrentPositionLiftPercent100ths
Movement started            →  OperationalStatus = Moving
Movement stopped            →  OperationalStatus = Stopped
```

## Transport : Wi-Fi vs Thread

| | Wi-Fi | Thread |
|---|---|---|
| **Support ESP32-WROOM-32** | Oui | Non (pas de 802.15.4) |
| **Portée** | ~50m intérieur | ~10m (mesh) |
| **Consommation** | Élevée (100-300mA) | Très faible (~10mA) |
| **Bande passante** | Haute | Faible |
| **Infrastructure** | Routeur Wi-Fi existant | Border Router requis |
| **Idéal pour** | Appareils sur secteur/grosse batterie | Capteurs sur pile |

**Notre choix : Wi-Fi** — L'ESP32-WROOM-32 ne supporte pas Thread (pas de radio 802.15.4). Wi-Fi est adapté car le moteur consomme déjà beaucoup.

Pour les futurs capteurs sur pile (température, luminosité), un ESP32-C6 ou ESP32-H2 avec Thread serait préférable.

## Ressources mémoire

| Composant | Flash approximative | RAM approximative |
|-----------|-------------------|-------------------|
| Matter stack | ~1.2 MB | ~80 KB |
| Wi-Fi stack | ~300 KB | ~40 KB |
| BLE (commissioning) | ~200 KB | ~30 KB |
| NVS (Matter fabrics) | ~24 KB | — |
| Application | ~100-200 KB | ~20 KB |
| **Total** | **~1.9 MB** | **~170 KB** |

L'ESP32-WROOM-32 a 520 KB de SRAM et 4 MB de flash. C'est **juste suffisant** mais ça passe avec une partition OTA de ~1.9 MB par slot.

## Considérations pour notre projet

1. **Utiliser esp-matter** d'Espressif — c'est le SDK officiel, bien maintenu, avec des exemples pour Window Covering
2. **Wi-Fi transport** — seul choix possible avec l'ESP32-WROOM-32
3. **BLE pour commissioning** — standard Matter, tous les contrôleurs s'y attendent
4. **Certificats de test** pendant le développement, vrais DAC pour la production
5. **Multi-fabric** — permettre que l'appareil soit contrôlé par Google Home ET Apple Home simultanément
6. **Subscriptions** — les contrôleurs Matter s'abonnent aux changements de position (pas de polling)
7. **Taille firmware** — surveiller de près, on est proche de la limite avec 1.9 MB/slot

## Erreurs courantes

1. **Oublier le BLE** : sans BLE, pas de commissioning → l'appareil ne peut pas être ajouté
2. **Certificats expirés** : les DAC de test ont une durée de vie limitée
3. **Pas de NVS** : Matter stocke les fabrics, sessions, credentials en NVS → sans NVS, l'appareil oublie tout au reboot
4. **Ignorer les subscriptions** : si on ne notifie pas les changements de position, l'UI des apps ne se met pas à jour
5. **RAM insuffisante** : Matter + Wi-Fi + BLE utilisent beaucoup de RAM → éviter les allocations dynamiques inutiles
