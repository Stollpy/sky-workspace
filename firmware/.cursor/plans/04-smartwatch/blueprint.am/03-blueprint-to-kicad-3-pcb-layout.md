# Phase 3 — PCB Layout dans KiCad

## 1. Board Outline

```
Edge.Cuts : rectangle arrondi 38 × 34 mm, coins rayon 4 mm
```

Dessiner avec l'outil **Arc** ou **Rounded Rectangle** sur la couche Edge.Cuts.

---

## 2. Zones fonctionnelles — Placement des composants

Le PCB de 38 × 34 mm est divisé en **zones fonctionnelles** pour minimiser les interférences et optimiser le routage :

```
┌──────────────────────────────────┐
│    ┌──────DISPLAY ZONE──────┐    │
│    │  (FPC connector LCD)   │    │  TOP (face écran)
│    │  FT6336U (touch)       │    │
│    └────────────────────────┘    │
│                                  │
│  ┌─MCU─┐  ┌─POWER─┐  ┌─AUDIO─┐ │
│  │ESP32 │  │AXP2101│  │MAX983 │ │
│  │PICO  │  │       │  │SPM14  │ │
│  └──────┘  └───────┘  │Speaker│ │
│                        └───────┘ │
│  ┌─SENSORS──┐  ┌─CONNECTIVITY──┐ │
│  │PCF8563   │  │SIM7080G       │ │
│  │BMI270    │  │eSIM MFF2      │ │
│  │DRV2605L  │  │TPS62743       │ │
│  └──────────┘  │Cbulk tantale  │ │
│                └───────────────┘ │
│                                  │
│  [BOOT]  [USB-C]  [POWER]       │  ← Bord latéral
│                                  │
│  ┌──RF ZONE (keep-out)─────┐    │
│  │ ESP32 antenna area      │    │  ← Bord opposé, pas de GND
│  │ (15mm free zone)        │    │
│  └─────────────────────────┘    │
│                                  │
│  [U.FL LTE]        [U.FL GPS]   │  ← Connecteurs FPC bord
└──────────────────────────────────┘

                BOTTOM (face peau)
┌──────────────────────────────────┐
│                                  │
│         ┌──HR ZONE──┐           │
│         │ MAX30102   │           │  Centré, aligné ouverture
│         │ (LEDs face │           │  back cover
│         │  bottom)   │           │
│         └────────────┘           │
│                                  │
│    LRA motor (fils soudés)       │
│                                  │
└──────────────────────────────────┘
```

---

## 3. Règles de placement critiques

### 3.1. ESP32-PICO-V3-02

- Placer **en bord de PCB** pour que l'antenne intégrée dépasse ou soit au bord
- **Zone d'exclusion RF** : 15 mm autour de l'antenne du module — **aucun cuivre** (GND, signal, power) sur toutes les couches dans cette zone
- Condensateurs de découplage **sous le module** (face bottom) ou **directement adjacent** (<2 mm)
- Via thermique GND sous le pad exposed (si présent)

### 3.2. AXP2101

- Placer **près de l'ESP32** (alimentation principale)
- Trace courte vers USB-C (VBUS) et vers batterie (VBAT)
- Condensateurs de découplage **sur chaque sortie rail** : placer le cap le plus proche possible de la pin ALDO/BLDO correspondante

### 3.3. TPS62743 (Buck LTE)

- Placer **près du SIM7080G** (trace courte pour le courant 2A)
- **Boucle de courant minimale** : VIN → L → VOUT → charge → GND → Cin → VIN
- L'inductance 2.2 µH doit être **adjacente** au TPS62743 (< 3 mm)
- Le condensateur 470 µF tantale doit être **entre le TPS62743 et le SIM7080G**
- Plan de GND **continu** sous toute cette zone (pas de coupure)

### 3.4. SIM7080G

- **Plus gros composant** (17.6 × 15.7 mm) → il occupe ~25% de la surface du PCB
- Placer dans un coin, avec les connecteurs U.FL RF en bord de PCB
- **GND pad** sous le module : utiliser plusieurs vias thermiques vers le plan GND (In1.Cu)
- Garder les traces SIM (SIM_DATA, SIM_CLK, SIM_RST) **courtes** et **blindées** vers l'eSIM MFF2

### 3.5. MAX30102

- **Face bottom obligatoire** (LEDs + photodiode vers la peau)
- Centré sur le PCB (aligné avec l'ouverture du back cover)
- **Pas de cuivre** entre le capteur et le bord du PCB dans la zone optique
- Via I2C vers la face top

### 3.6. Antennes — Connecteurs U.FL

- Placer les **2 connecteurs U.FL** (LTE + GPS) sur le **bord du PCB** côté bracelet
- Traces RF en **microstrip 50 Ω** :
  - Largeur de trace ≈ **0.35 mm** (sur FR4 1.6mm, 4 couches, diélectrique ~4.4)
  - Calculer avec un outil de calcul d'impédance (KiCad built-in ou https://www.eeweb.com/tools/microstrip-impedance)
  - GND plan **continu et ininterrompu** sous la trace RF (In1.Cu)
- **Pas de via** sous les traces RF
- Distance entre les 2 antennes : **maximum possible** pour éviter le couplage

---

## 4. Routage — Ordre recommandé

| Priorité | Nets | Technique |
|----------|------|-----------|
| 1 | **GND** | Copper zone (fill) sur In1.Cu — plan continu, éviter les splits |
| 2 | **3V3, VBAT** | Copper zone sur In2.Cu + traces larges sur F.Cu/B.Cu |
| 3 | **Traces RF** (ANT_LTE, ANT_GPS, ANT_WIFI) | Microstrip 50 Ω, face top, GND continu dessous |
| 4 | **SPI display** (SCK, MOSI, CS, DC) | Traces groupées, longueur matchée pas nécessaire (SPI <40 MHz) |
| 5 | **I2C Bus 0 et Bus 1** | Traces directes, pas de longueur critique |
| 6 | **UART** (LTE, GPS) | Traces directes |
| 7 | **I2S + PDM** (audio) | Traces groupées, éloigner du SIM7080G (interférences RF) |
| 8 | **SIM interface** | Traces courtes et blindées (GND guard) |

---

## 5. Copper Zones (Plans de cuivre)

| Couche | Zone | Net |
|--------|------|-----|
| F.Cu | Zone partielle si nécessaire | GND (stitching) |
| **In1.Cu** | **Zone complète** | **GND** (plan principal) |
| **In2.Cu** | Zone complète | **3V3** (avec split pour VBAT si nécessaire) |
| B.Cu | Zone partielle | GND (stitching) |

### Via stitching

- Ajouter des **vias GND de stitching** tous les ~3 mm autour du board pour connecter les plans GND top/bottom vers In1.Cu
- **Autour des zones RF** : vias plus denses (tous les 1.5–2 mm) pour former un mur de blindage

---

## 6. DRC (Design Rule Check)

Avant de passer à la fabrication :

```
Inspect → Design Rules Checker
```

| Check | Critère |
|-------|---------|
| Clearance violations | 0 |
| Track width violations | 0 |
| Unconnected nets | 0 |
| Via drill violations | 0 |
| Copper zone issues | 0 |
| Board outline violations | 0 |

---

## 7. Checklist Phase 3

- [ ] Board outline 38 × 34 mm avec coins arrondis sur Edge.Cuts
- [ ] Composants placés selon les zones fonctionnelles
- [ ] ESP32 en bord de PCB avec keep-out RF 15 mm
- [ ] MAX30102 face bottom, centré
- [ ] SIM7080G + TPS62743 + eSIM groupés
- [ ] Connecteurs U.FL en bord, traces 50 Ω
- [ ] GND plan continu sur In1.Cu (pas de split)
- [ ] Power plan sur In2.Cu
- [ ] Via stitching GND placés
- [ ] Tous les nets routés
- [ ] DRC = 0 erreurs
- [ ] 3D Viewer vérifié (View → 3D Viewer) pour le clearance mécanique
