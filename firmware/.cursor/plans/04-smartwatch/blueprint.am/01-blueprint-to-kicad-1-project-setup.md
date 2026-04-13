# Phase 1 — Création du Projet KiCad

## 1. Installer KiCad 8+ (si pas déjà fait)

```bash
brew install --cask kicad     # macOS
```

## 2. Créer le projet

```
File → New Project → "stollper-watch"
```

Structure résultante :

```
stollper-watch/
├── stollper-watch.kicad_pro        ← Projet
├── stollper-watch.kicad_sch        ← Schéma
├── stollper-watch.kicad_pcb        ← PCB layout
├── libs/                           ← Librairies custom
│   ├── stollper-watch.kicad_sym    ← Symboles
│   └── stollper-watch.pretty/      ← Footprints
└── 3d/                             ← Modèles 3D (.step/.wrl)
```

## 3. Librairies de symboles nécessaires

### 3.1. Symboles disponibles dans les libs KiCad standard

| Composant | Librairie KiCad | Symbole |
|-----------|----------------|---------|
| ESP32-PICO-V3-02 | `RF_Module` | `ESP32-PICO-V3-02` |
| USB-C Receptacle | `Connector` | `USB_C_Receptacle_USB2.0` |
| Résistances 0402 | `Device` | `R_Small` |
| Condensateurs 0402 | `Device` | `C_Small` |
| Inductance | `Device` | `L_Small` |
| Boutons tactiles | `Switch` | `SW_Push` |
| Batterie CR/ML | `Device` | `Battery_Cell` |
| Connecteur FPC RF | `Connector_Coaxial` | `U.FL` ou `Hirose_U.FL` |

### 3.2. Symboles à créer ou importer (libs custom)

| Composant | Source recommandée | Action |
|-----------|-------------------|--------|
| **AXP2101** | SnapEDA / Ultra Librarian | Importer symbole + footprint QFN-48 |
| **PCF8563** | KiCad `Timer_RTC` (vérifier) | Souvent dispo, sinon SnapEDA |
| **MAX30102** | SnapEDA | Importer (optique 14-pin OLGA) |
| **BMI270** | SnapEDA / Bosch | Importer (LGA-14) |
| **DRV2605L** | SnapEDA / TI | Importer (DGS-10 VSSOP) |
| **FT6336U** | SnapEDA | Importer (QFN-16) |
| **ST7789V3** | En général pas de symbole — c'est un **module avec FPC**. Créer un connecteur FPC 18–24 pins |
| **SPM1423HM4H-B** | SnapEDA / Knowles | Importer (LGA-4) |
| **MAX98357A** | SnapEDA / Analog Devices | Importer (WLP-9 ou TQFN) |
| **SIM7080G** | SIMCom / SnapEDA | Importer (LGA-68, gros footprint) |
| **MIA-M10Q** | u-blox / SnapEDA | Importer (LCC-22) |
| **TPS62743** | TI / SnapEDA | Importer (SOT-23-5 ou DSBGA) |
| **MFF2 eSIM** | SnapEDA (chercher "MFF2") | Importer (8-pad) |

### 3.3. Comment importer depuis SnapEDA

1. Aller sur https://www.snapeda.com
2. Chercher le composant (ex. "AXP2101")
3. Télécharger au format **KiCad**
4. Extraire le `.kicad_sym` → copier dans `libs/stollper-watch.kicad_sym`
5. Extraire le `.kicad_mod` → copier dans `libs/stollper-watch.pretty/`
6. Extraire le `.step` → copier dans `3d/`
7. Dans KiCad : Preferences → Manage Symbol Libraries → Add `libs/stollper-watch.kicad_sym`
8. Même chose pour footprints : Preferences → Manage Footprint Libraries → Add `libs/stollper-watch.pretty`

## 4. Paramètres du projet

### 4.1. Design Rules (PCB)

```
Preferences → Board Setup → Design Rules
```

| Paramètre | Valeur | Raison |
|-----------|--------|--------|
| Track width (default) | 0.15 mm | 0402 passifs, fine pitch |
| Track width (power) | 0.3 mm | Rails 3.3V, VBAT |
| Track width (LTE power) | 0.5 mm | 2A peak SIM7080G |
| Clearance | 0.12 mm | Standard 4-couches |
| Via drill | 0.3 mm | JLCPCB standard |
| Via diameter | 0.6 mm | JLCPCB standard |
| Via drill (micro) | 0.2 mm | Si HDI disponible |
| Board outline | 38 × 34 mm | Apple Watch form factor |
| Copper layers | 4 (F.Cu, In1.Cu, In2.Cu, B.Cu) | Signal-GND-Power-Signal |

### 4.2. Stackup 4 couches

```
Board Setup → Board Stackup
```

| Couche | Épaisseur | Rôle |
|--------|-----------|------|
| F.Cu | 35 µm | Signal + composants top |
| Prepreg | 0.2 mm | Isolation |
| In1.Cu | 35 µm | **Plan de masse (GND)** |
| Core | 0.8 mm | Substrat FR4 |
| In2.Cu | 35 µm | **Plan d'alimentation (3V3, VBAT)** |
| Prepreg | 0.2 mm | Isolation |
| B.Cu | 35 µm | Signal + composants bottom |

> **Total ≈ 1.6 mm** — standard JLCPCB 4 couches.

### 4.3. Net Classes

```
Board Setup → Net Classes
```

| Net Class | Track Width | Clearance | Via Size | Nets |
|-----------|-------------|-----------|----------|------|
| Default | 0.15 mm | 0.12 mm | 0.6/0.3 mm | Tous les signaux |
| Power | 0.3 mm | 0.15 mm | 0.6/0.3 mm | 3V3, VBAT, 5V |
| LTE_Power | 0.5 mm | 0.2 mm | 0.8/0.4 mm | VBAT_LTE, 3V3_LTE |
| RF | 0.35 mm | 0.3 mm | — | ANT_WIFI, ANT_LTE, ANT_GPS |

## 5. Checklist Phase 1

- [ ] Projet KiCad créé
- [ ] Structure de dossiers en place (libs/, 3d/)
- [ ] Symboles importés depuis SnapEDA pour les composants non-standard (~12 symboles)
- [ ] Footprints importés et associés
- [ ] Design rules configurées (track widths, clearances, vias)
- [ ] Stackup 4 couches défini
- [ ] Net Classes créées (Default, Power, LTE_Power, RF)
- [ ] Board outline 38 × 34 mm tracée sur Edge.Cuts
