# Plan 05 — Sky Motor : Phase 2 — Client API & Commands

## Objectif

Définir et implémenter l'**API complète** exposée par le firmware Sky Motor aux clients (apps, Home Assistant, Google Home, Apple Home). Cette API est servie via Matter (commandes standard) et potentiellement via un endpoint REST/BLE pour les fonctionnalités avancées (patterns, calibration).

## Prérequis

- Plan 05, Phase 1 (Firmware Assembly) complétée

## Canaux de communication

```
┌──────────────────────────────────────────────────┐
│                   Client Apps                     │
│  (Google Home, Apple Home, Home Assistant, etc.)  │
└────────────┬─────────────────────────┬────────────┘
             │                         │
     ┌───────▼───────┐       ┌────────▼────────┐
     │    Matter      │       │   Custom API    │
     │  (standard)    │       │  (avancé)       │
     │               │       │                 │
     │  • Open/Close │       │  • Calibration  │
     │  • GoTo %     │       │  • Patterns     │
     │  • Stop       │       │  • Diagnostics  │
     │  • Position   │       │  • Config       │
     │  • Battery    │       │                 │
     └───────┬───────┘       └────────┬────────┘
             │                         │
     ┌───────▼─────────────────────────▼────────┐
     │            Sky Motor Firmware              │
     └───────────────────────────────────────────┘
```

## API Matter (standard)

Ces commandes sont nativement supportées par tous les écosystèmes Matter.

### Window Covering Cluster (0x0102)

| Commande Matter | Action firmware | Paramètres |
|----------------|----------------|------------|
| `UpOrOpen` | `sky_stepper_set_height(motor, 0)` | — |
| `DownOrClose` | `sky_stepper_set_height(motor, 100)` | — |
| `StopMotion` | `sky_stepper_stop(motor)` | — |
| `GoToLiftPercentage` | `sky_stepper_set_height(motor, %)` | `percent: 0-100` |

### Attributs reportés

| Attribut | Source | Mise à jour |
|----------|--------|-------------|
| `CurrentPositionLiftPercent100ths` | Position moteur | Temps réel (subscription) |
| `TargetPositionLiftPercent100ths` | Commande en cours | À chaque commande |
| `OperationalStatus` | State machine moteur | Temps réel |
| `InstalledOpenLimitLift` | Calibration (0) | Après calibration |
| `InstalledClosedLimitLift` | Calibration (max) | Après calibration |
| `Mode` | Config (ex: calibration mode) | Manuel |

### Power Source Cluster (0x002F)

| Attribut | Source | Mise à jour |
|----------|--------|-------------|
| `BatPercentRemaining` | `sky_power_get_percent() * 2` | Toutes les minutes |
| `BatVoltage` | `sky_power_get_voltage() * 1000` | Toutes les minutes |
| `BatChargeLevel` | Seuils (OK/Warning/Critical) | Sur changement |
| `BatChargeState` | Détection de charge | Sur changement |

## API Custom (fonctionnalités avancées)

Pour les fonctionnalités non couvertes par Matter (calibration, patterns, diagnostics), on utilise un **cluster custom Matter** ou un **endpoint BLE GATT** dédié.

### Option recommandée : cluster Matter custom

Matter permet de définir des clusters custom (ID >= 0xFFF10000). Cela permet de garder un seul canal de communication.

### Commandes de calibration

```
Commande                    Payload                      Réponse
─────────────────────────────────────────────────────────────────────
calibrate_start             {direction: "cw"|"ccw"}      {status: "ok"}
calibrate_mark_open         {}                           {position: 1234}
calibrate_mark_closed       {}                           {position: 4567, total: 3333}
calibrate_cancel            {}                           {status: "cancelled"}
calibrate_status            {}                           {calibrated: true, total: 3333}
```

### Commandes de patterns

```
Commande                    Payload                      Réponse
─────────────────────────────────────────────────────────────────────
preset_save                 {name: "demi", percent: 50}  {status: "ok"}
preset_apply                {name: "demi"}               {status: "ok"}
preset_delete               {name: "demi"}               {status: "ok"}
preset_list                 {}                           {presets: [{name, percent}]}

sequence_record_start       {name: "matin"}              {status: "recording"}
sequence_record_step        {percent: 30, delay_ms: 0}   {step: 1}
sequence_record_step        {percent: 70, delay_ms: 5000} {step: 2}
sequence_record_stop        {}                           {status: "saved", steps: 2}
sequence_play               {name: "matin"}              {status: "playing"}
sequence_stop               {}                           {status: "stopped"}
sequence_list               {}                           {sequences: [{name, steps}]}
sequence_delete             {name: "matin"}              {status: "ok"}
sequence_export             {name: "matin"}              {pattern: {...}}
sequence_import             {pattern: {...}}             {status: "ok"}
```

### Commandes de diagnostic

```
Commande                    Payload                      Réponse
─────────────────────────────────────────────────────────────────────
status                      {}                           {motor: {...}, battery: {...},
                                                          wifi: {...}, matter: {...}}
reboot                      {}                           {status: "rebooting"}
factory_reset               {}                           {status: "resetting"}
firmware_version            {}                           {version: "1.2.3"}
ota_check                   {}                           {available: true, version: "1.3.0"}
ota_update                  {}                           {status: "downloading"}
```

## Sérialisation

Les payloads custom utilisent **CBOR** (Concise Binary Object Representation) plutôt que JSON pour économiser la bande passante et la RAM. CBOR est le format natif de Matter pour les TLV (Type-Length-Value).

```c
// Taille typique d'un payload
// JSON: {"name":"matin","percent":50}  = 31 bytes
// CBOR: équivalent                     = 18 bytes (42% plus petit)
```

## Intégration Home Assistant

Home Assistant supporte Matter nativement. Les commandes standard (open, close, position) fonctionnent out-of-the-box. Pour les commandes custom (calibration, patterns), deux options :

1. **Matter custom cluster** : Home Assistant peut exposer les clusters custom via son API
2. **Service dédié** : créer un add-on Home Assistant qui communique directement avec le firmware

## Tâches

- [ ] Implémenter le mapping complet Matter Window Covering ↔ stepper
- [ ] Implémenter le mapping Matter Power Source ↔ battery monitor
- [ ] Définir et implémenter le cluster Matter custom pour calibration + patterns
- [ ] Implémenter la sérialisation CBOR des payloads custom
- [ ] Créer les handlers pour chaque commande custom
- [ ] Tester avec Google Home (open, close, go to %)
- [ ] Tester avec Apple Home (open, close, go to %)
- [ ] Tester avec Home Assistant (standard + custom commands)
- [ ] Documenter l'API complète pour les développeurs d'apps

## Critères de validation

- [ ] Google Home : le store apparaît comme "Window Covering", toutes les commandes standard fonctionnent
- [ ] Apple Home : même chose
- [ ] Home Assistant : commandes standard + accès aux commandes custom (calibration, patterns)
- [ ] La calibration complète fonctionne de bout en bout via l'API
- [ ] Un pattern enregistré via l'API se rejoue correctement
- [ ] Le niveau de batterie est visible dans toutes les apps
- [ ] La latence commande → début de mouvement est < 500ms (hors deep sleep)

## Notes techniques

- **Cluster custom Matter** : les clusters custom doivent avoir un ID dans la plage fabricant (0xFFF10000 - 0xFFF1FFFE pour le vendor ID 0xFFF1). En production avec un vrai Vendor ID, la plage sera différente.
- **CBOR** : ESP-IDF n'inclut pas de librairie CBOR par défaut. Utiliser `tinycbor` (petit, performant, adapté à l'embarqué) ou la librairie CBOR intégrée à connectedhomeip.
- **Rate limiting** : limiter les commandes de mouvement à 1 par seconde pour éviter les abus (ex: slider qui envoie une commande à chaque pixel de mouvement).
- **Idempotence** : `set_height(50)` quand le store est déjà à 50% ne doit pas déclencher de mouvement. Vérifier la position actuelle avant de déplacer.
