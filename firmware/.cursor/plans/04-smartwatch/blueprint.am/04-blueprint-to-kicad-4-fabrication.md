# Phase 4 — Fabrication : Gerbers, BOM Finale, Commande

## 1. Export Gerbers depuis KiCad

```
File → Fabrication Outputs → Gerbers (.gbr)
```

### 1.1. Couches à exporter

| Couche KiCad | Fichier Gerber | Description |
|-------------|----------------|-------------|
| F.Cu | `stollper-watch-F_Cu.gbr` | Cuivre face top |
| In1.Cu | `stollper-watch-In1_Cu.gbr` | Plan GND |
| In2.Cu | `stollper-watch-In2_Cu.gbr` | Plan Power |
| B.Cu | `stollper-watch-B_Cu.gbr` | Cuivre face bottom |
| F.Paste | `stollper-watch-F_Paste.gbr` | Stencil pâte top |
| B.Paste | `stollper-watch-B_Paste.gbr` | Stencil pâte bottom |
| F.Silkscreen | `stollper-watch-F_Silkscreen.gbr` | Sérigraphie top |
| B.Silkscreen | `stollper-watch-B_Silkscreen.gbr` | Sérigraphie bottom |
| F.Mask | `stollper-watch-F_Mask.gbr` | Solder mask top |
| B.Mask | `stollper-watch-B_Mask.gbr` | Solder mask bottom |
| Edge.Cuts | `stollper-watch-Edge_Cuts.gbr` | Contour PCB |

### 1.2. Export Drill

```
File → Fabrication Outputs → Drill Files (.drl)
```

- Format : Excellon
- Unités : mm
- Zéros : Decimal format
- Merge PTH et NPTH en un seul fichier (JLCPCB le préfère) ou séparés selon le fabricant

### 1.3. Export Pick & Place (si assemblage JLCPCB SMT)

```
File → Fabrication Outputs → Component Placement (.csv)
```

- Format CSV
- Colonnes : Ref, Val, Package, PosX, PosY, Rot, Side

---

## 2. BOM Finale Corrigée

BOM complète intégrant les **corrections** par rapport à Blueprint.am (passifs ajoutés, composants corrigés).

### 2.1. Composants actifs (ICs)

| # | Ref | Composant | Package | Qté | Fournisseur | Note |
|---|-----|-----------|---------|-----|-------------|------|
| 1 | U1 | ESP32-PICO-V3-02 | LGA-48 (7×7mm) | 1 | DigiKey / Mouser | MCU principal |
| 2 | U2 | AXP2101 | QFN-48 (4×4mm) | 1 | AliExpress / LCSC | PMU |
| 3 | U3 | TPS62743 | SOT-23-5 | 1 | DigiKey / Mouser / LCSC | DC-DC buck LTE |
| 4 | U4 | PCF8563 | SO-8 | 1 | DigiKey / LCSC | RTC |
| 5 | U5 | MAX30102 | OLGA-14 (3×3mm) | 1 | DigiKey / Mouser | HR + SpO2 |
| 6 | U6 | BMI270 | LGA-14 (2.5×2.5mm) | 1 | DigiKey / Mouser | IMU 6 axes |
| 7 | U7 | DRV2605L | VSSOP-10 (3×3mm) | 1 | DigiKey / LCSC | Haptic driver |
| 8 | U8 | FT6336U | QFN-16 (3×3mm) | 1 | LCSC | Touch controller |
| 9 | U9 | MAX98357A | WLP-9 (1.4×1.4mm) | 1 | DigiKey / Mouser | Ampli I2S |
| 10 | U10 | SPM1423HM4H-B | LGA-4 (3.3×2.6mm) | 1 | DigiKey | Micro PDM |
| 11 | U11 | SIM7080G | LGA-68 (17.6×15.7mm) | 1 | DigiKey / LCSC | LTE-M modem |
| 12 | U12 | MIA-M10Q | LCC-22 (4.5×4.5mm) | 1 | DigiKey / Mouser | GPS GNSS |
| 13 | U13 | MFF2 eSIM | MFF2-8 (6×5mm) | 1 | DigiKey | eSIM soudée |

### 2.2. Passifs (condensateurs)

| # | Ref | Valeur | Package | Qté | Net/Usage |
|---|-----|--------|---------|-----|-----------|
| 14 | C1–C3 | 100 nF X7R | 0402 | 3 | ESP32 VDD3P3 |
| 15 | C4 | 10 µF X5R | 0402 | 1 | ESP32 VDD3P3 |
| 16 | C5 | 100 nF X7R | 0402 | 1 | ESP32 VDD_RTC |
| 17 | C6 | 10 µF X5R | 0805 | 1 | AXP2101 VBUS |
| 18 | C7 | 10 µF X5R | 0805 | 1 | AXP2101 VBAT |
| 19 | C8 | 22 µF X5R | 0805 | 1 | AXP2101 DC1 |
| 20 | C9 | 100 nF X7R | 0402 | 1 | AXP2101 DC1 |
| 21 | C10–C15 | 1 µF X5R | 0402 | 6 | AXP2101 ALDO2/3/4, BLDO1/2, VBACKUP |
| 22 | C16 | 10 µF X5R | 0805 | 1 | TPS62743 Cin |
| 23 | C17 | 22 µF X5R | 0805 | 1 | TPS62743 Cout |
| 24 | C18 | 470 µF 6.3V | Tantale polymère (7343) | 1 | TPS62743 → SIM7080G bulk |
| 25 | C19 | 100 nF X7R | 0402 | 1 | MAX30102 VDD |
| 26 | C20 | 1 µF X5R | 0402 | 1 | MAX30102 VLED |
| 27 | C21–C22 | 100 nF X7R | 0402 | 2 | BMI270 VDD + VDDIO |
| 28 | C23 | 100 nF X7R | 0402 | 1 | DRV2605L |
| 29 | C24 | 10 µF X5R | 0402 | 1 | DRV2605L |
| 30 | C25 | 100 nF X7R | 0402 | 1 | PCF8563 |
| 31 | C26 | 100 nF X7R | 0402 | 1 | FT6336U |
| 32 | C27 | 100 nF X7R | 0402 | 1 | SPM1423HM4H |
| 33 | C28 | 100 nF X7R | 0402 | 1 | MAX98357A |
| 34 | C29 | 10 µF X5R | 0402 | 1 | MAX98357A |
| 35 | C30 | 100 nF X7R | 0402 | 1 | SIM7080G |
| 36 | C31 | 10 µF X5R | 0805 | 1 | SIM7080G |
| 37 | C32 | 100 nF X7R | 0402 | 1 | MIA-M10Q |
| 38 | C33 | 10 µF X5R | 0805 | 1 | MIA-M10Q |
| 39 | C34 | 100 nF X7R | 0402 | 1 | Display VCC |
| 40 | C35 | 10 µF X5R | 0402 | 1 | Display VCC |
| 41 | C36 | 1 µF X5R | 0402 | 1 | ESP32 EN (reset RC) |

### 2.3. Passifs (résistances)

| # | Ref | Valeur | Package | Qté | Usage |
|---|-----|--------|---------|-----|-------|
| 42 | R1–R2 | 4.7 kΩ | 0402 | 2 | I2C Bus 0 pull-ups (SDA/SCL) |
| 43 | R3–R4 | 4.7 kΩ | 0402 | 2 | I2C Bus 1 pull-ups (SDA1/SCL1) |
| 44 | R5 | 10 kΩ | 0402 | 1 | ESP32 EN pull-up |
| 45 | R6 | 10 kΩ | 0402 | 1 | GPIO0 BOOT pull-up |
| 46 | R7–R8 | 5.1 kΩ | 0402 | 2 | USB-C CC1/CC2 pull-downs |
| 47 | R9 | 100 kΩ | 0402 | 1 | MAX98357A SD_MODE pull-up |
| 48 | R10–R11 | Selon datasheet TPS62743 | 0402 | 2 | Diviseur feedback (ex. 1MΩ + 309kΩ pour 3.3V) |
| 49 | R12 | 1 MΩ | 0402 | 1 | USB-C shield → GND |

### 2.4. Inductance

| # | Ref | Valeur | Package | Qté | Usage |
|---|-----|--------|---------|-----|-------|
| 50 | L1 | 2.2 µH, Isat ≥ 2A | 1210 ou 1008 | 1 | TPS62743 buck inductance |

### 2.5. Connecteurs et mécaniques

| # | Ref | Composant | Qté | Usage |
|---|-----|-----------|-----|-------|
| 51 | J1 | USB-C Receptacle (USB 2.0, 16 pin) | 1 | Charge + programmation |
| 52 | J2 | Connecteur U.FL / IPEX MHF | 1 | Antenne LTE FPC |
| 53 | J3 | Connecteur U.FL / IPEX MHF | 1 | Antenne GPS FPC |
| 54 | J4 | Connecteur FPC 18–24 pins, 0.5 mm pitch | 1 | Display LCD |
| 55 | SW1 | Bouton tactile 3×3×2 mm | 1 | BOOT |
| 56 | SW2 | Bouton tactile 3×3×2 mm | 1 | POWER (via AXP2101) |
| 57 | BT1 | LiPo 3.7V 300–400 mAh | 1 | Batterie (fil soudé) |
| 58 | M1 | LRA Vibration Motor 8×3.5 mm | 1 | Haptic (fil soudé) |
| 59 | SPK1 | Mini Speaker 8×8 mm | 1 | Audio out (fil soudé) |

### 2.6. Résumé

| Catégorie | Quantité |
|-----------|----------|
| ICs actifs | 13 |
| Condensateurs | ~36 |
| Résistances | ~12 |
| Inductances | 1 |
| Connecteurs | 4 |
| Boutons | 2 |
| Batterie + moteur + speaker | 3 |
| **Total composants** | **~71** |

> Blueprint.am en listait **41** → le vrai total est **~71** en comptant tous les passifs.

---

## 3. Où commander le PCB

### 3.1. JLCPCB (recommandé pour proto)

- URL : https://jlcpcb.com
- Upload : ZIP des Gerbers + drill files
- Options à sélectionner :

| Paramètre | Valeur |
|-----------|--------|
| Layers | 4 |
| PCB Thickness | 1.6 mm |
| Surface Finish | **ENIG** (pour les pads fin pitch QFN/BGA) |
| Solder Mask | Black (ou au choix) |
| Silkscreen | White |
| Min Track/Space | 0.1/0.1 mm (si disponible) |
| Via Hole | 0.3 mm |
| Board Size | 38 × 34 mm |
| Quantity | 5 (minimum) |
| Impedance Control | Oui (50 Ω pour traces RF) — coûte un peu plus |

- **Prix estimé** : ~15–30 € pour 5 PCBs (4 couches, impedance control)
- **Délai** : 5–8 jours fabrication + shipping

### 3.2. PCBWay (alternative)

- URL : https://www.pcbway.com
- Mêmes paramètres, interface similaire
- Souvent légèrement plus cher mais support réactif

### 3.3. Stencil (pour la pâte à souder)

Commander un **stencil en acier** avec les PCBs :
- JLCPCB : cocher "SMT Stencil" lors de la commande (~8 €)
- Face top obligatoire, face bottom si MAX30102 est soudé en bottom

---

## 4. Assemblage SMT (optionnel JLCPCB Assembly)

JLCPCB propose un service d'assemblage SMT. Si tu l'utilises :

1. Exporter le **BOM** depuis KiCad (format CSV)
2. Exporter le **Pick & Place** (Component Placement .csv)
3. Mapper chaque composant à un **LCSC part number** (certains composants comme l'ESP32-PICO ou le SIM7080G ne sont pas toujours en stock LCSC)

> Pour un **premier proto**, je recommande la **soudure manuelle** (hot air + pâte) :
> tu vérifies chaque soudure, tu comprends le PCB, et tu peux corriger des erreurs.

---

## 5. Validation pré-fabrication

### 5.1. DRC KiCad

```
Inspect → Design Rules Checker → Run DRC
```

**0 erreurs, 0 warnings** avant d'exporter.

### 5.2. Gerber Viewer

Vérifier les Gerbers avec un viewer **indépendant** :
- https://www.gerber-viewer.com (en ligne)
- **gerbv** (macOS : `brew install gerbv`)
- KiCad built-in : File → Open Gerber Viewer

Vérifier visuellement :
- [ ] Contour PCB correct
- [ ] Cuivre sur les 4 couches
- [ ] Pads visibles pour tous les composants
- [ ] Solder mask openings correctes
- [ ] Silk lisible
- [ ] Drill holes aux bons emplacements

### 5.3. Checklist finale

- [ ] Gerbers exportés (12 fichiers)
- [ ] Drill file exporté
- [ ] Gerbers vérifiés dans un viewer externe
- [ ] BOM finale vérifiée (71 composants)
- [ ] Commande PCB passée (JLCPCB/PCBWay)
- [ ] Stencil commandé
- [ ] Composants commandés (DigiKey/Mouser/LCSC/AliExpress)
- [ ] Outils de soudure prêts (station hot air, pâte, microscope)
