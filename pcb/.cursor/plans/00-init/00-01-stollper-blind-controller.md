# Stollper Blind Controller — Plan PCB Final

## 1. Objectif

Concevoir un PCB compact pour un contrôleur de moteur pas à pas de store enrouleur,
alimenté par batterie Li-Ion, avec communication sans fil (BLE/Wi-Fi) et port de charge
magnétique. Le système mécanique utilise un ressort de contrepoids pour minimiser
l'effort moteur et la consommation énergétique.

---

## 2. Stratégie mécanique — Ressort de contrepoids

Le moteur 28BYJ-48 alimenté en 3.3V via batterie Li-Ion (après drop Darlington du
ULN2003A) ne délivre que ~20-25 mN.m de couple. Sans assistance, c'est insuffisant
pour un store standard.

**Solution retenue : ressort spiral à force constante dans le tube d'enroulement (ø 38mm).**

- Store descend → le ressort accumule de l'énergie
- Store monte → le ressort restitue l'énergie
- Le moteur ne fournit que l'effort de friction (~15-20 mN.m) → compatible avec le 28BYJ-48 à basse tension
- Maintien en position sans courant (deep sleep ESP32 possible)
- Réduction de consommation : 60-80%
- Pas d'impact sur le PCB (composant mécanique uniquement)

**Conséquence sur le PCB :** pas besoin de boost converter 5V, le circuit reste simple.

---

## 3. Schématique complète

### 3.1 Microcontrôleur — ESP32-WROOM-32

| Paramètre         | Valeur                                    |
|--------------------|-------------------------------------------|
| Module             | ESP32-WROOM-32 (18 x 25.5 mm)            |
| Alimentation       | 3.3V depuis LDO                           |
| Condensateurs      | 10µF + 100nF (découplage VCC)             |

**Pins GPIO utilisées :**

| GPIO  | Fonction                  |
|-------|---------------------------|
| GPIO13 | ULN2003A IN1             |
| GPIO12 | ULN2003A IN2             |
| GPIO14 | ULN2003A IN3             |
| GPIO27 | ULN2003A IN4             |
| GPIO34 | ADC batterie (input only) |
| GPIO1  | UART TX (programmation)  |
| GPIO3  | UART RX (programmation)  |
| EN     | Enable (programmation)   |
| GPIO0  | Boot mode (programmation)|

**Programmation — Test Points obligatoires :**
- TX, RX, GND, 3V3, EN, GPIO0 (6 pads minimum)
- EN nécessite pull-up 10k vers 3.3V + condensateur 100nF vers GND
- GPIO0 nécessite pull-up 10k vers 3.3V (boot normal = HIGH, flash = LOW)

### 3.2 Driver Moteur — ULN2003A (SOP-16)

| Paramètre         | Valeur                                    |
|--------------------|-------------------------------------------|
| Boîtier            | SOP-16                                    |
| Entrées            | IN1-IN4 ← GPIO ESP32                     |
| Sorties            | OUT1-OUT4 → Connecteur moteur JST-XH 5p  |
| VCC (COM)          | VBAT direct (3.7-4.2V)                   |
| Drop Darlington    | ~0.9V par sortie                          |

Le moteur est alimenté directement depuis VBAT (pas depuis le 3.3V régulé)
pour maximiser la tension disponible aux bobines.

### 3.3 Alimentation

#### 3.3.1 Batterie Li-Ion

| Paramètre         | Valeur                                    |
|--------------------|-------------------------------------------|
| Connecteur         | JST-PH 2 broches                         |
| Tension            | 3.0V (déchargée) — 4.2V (pleine)         |
| Capacité recommandée | ≥ 2000 mAh                             |

#### 3.3.2 Charge — TP4056 (SOP-8) + Protection DW01A + 8205A

| Composant  | Boîtier | Fonction                                  |
|------------|---------|-------------------------------------------|
| TP4056     | SOP-8   | Contrôleur de charge Li-Ion               |
| DW01A      | SOT-23-6| Protection surcharge / surdécharge / court-circuit |
| 8205A      | SOT-23-6| Dual MOSFET de coupure                    |

**TP4056 — Composants associés :**

| Réf  | Composant     | Valeur     | Fonction                          |
|------|---------------|------------|-----------------------------------|
| RPROG | Résistance   | 1.2kΩ     | Courant de charge = 1A            |
| C_IN  | Condensateur | 10µF       | Découplage entrée (pin VCC)       |
| C_OUT | Condensateur | 10µF       | Découplage sortie (pin BAT)       |
| LED1  | LED rouge    | 0603       | En charge (pin CHRG)              |
| LED2  | LED verte    | 0603       | Chargé (pin STDBY)               |
| R_LED1 | Résistance  | 1kΩ       | Limitation courant LED rouge      |
| R_LED2 | Résistance  | 1kΩ       | Limitation courant LED verte      |

**DW01A + 8205A — Protection batterie :**

- Coupure si tension < 2.5V (surdécharge) → protège la batterie Li-Ion
- Coupure si tension > 4.3V (surcharge)
- Coupure en cas de court-circuit
- Se place entre le TP4056 et le connecteur batterie JST-PH

#### 3.3.3 Régulateur LDO — AP2112K-3.3 (SOT-23-5)

| Paramètre         | Valeur                                    |
|--------------------|-------------------------------------------|
| Boîtier            | SOT-23-5                                  |
| Dropout             | ~250mV (à 600mA)                        |
| Tension sortie     | 3.3V                                      |
| Courant max        | 600mA                                     |
| C_IN               | 1µF céramique                             |
| C_OUT              | 1µF céramique                             |

**IMPORTANT :** Ne PAS utiliser l'AMS1117-3.3 (dropout ~1V, incompatible avec batterie 3.7V).

#### 3.3.4 Mesure tension batterie — Pont diviseur

| Réf  | Composant   | Valeur | Notes                              |
|------|-------------|--------|------------------------------------|
| R_DIV1 | Résistance | 100kΩ | Haute impédance (minimise courant de fuite) |
| R_DIV2 | Résistance | 220kΩ | Ratio : VBAT × 220/(100+220) = 0.688 × VBAT |
| C_FILT | Condensateur | 100nF | Filtrage bruit ADC, sur GPIO34     |

- VBAT 4.2V → ADC voit 2.89V (dans le range 0-3.3V)
- VBAT 3.0V → ADC voit 2.06V
- Courant de fuite : ~13µA (acceptable en deep sleep)

### 3.4 Connecteur de charge magnétique — Pogo Pins

| Paramètre         | Valeur                                    |
|--------------------|-------------------------------------------|
| Type               | Connecteur magnétique Pogo Pins 2 broches |
| Pas                | 2.54mm (à confirmer selon modèle)        |
| Empreinte          | Custom (pads circulaires ø 1.5mm)         |

**Protection de polarité inversée :**

| Réf  | Composant       | Valeur    | Fonction                        |
|------|-----------------|-----------|----------------------------------|
| D1   | Diode Schottky  | SS14 (SMA)| Protection inversion polarité    |

- VCC pogo → anode D1 → cathode D1 → VIN du TP4056
- GND pogo → masse commune
- Drop diode : ~0.3V (5V chargeur → 4.7V au TP4056 → acceptable)

---

## 4. BOM résumé (Bill of Materials)

| Réf    | Composant          | Boîtier     | Qté |
|--------|---------------------|-------------|-----|
| U1     | ESP32-WROOM-32     | Module      | 1   |
| U2     | ULN2003A           | SOP-16      | 1   |
| U3     | TP4056             | SOP-8       | 1   |
| U4     | DW01A              | SOT-23-6    | 1   |
| U5     | 8205A              | SOT-23-6    | 1   |
| U6     | AP2112K-3.3        | SOT-23-5    | 1   |
| D1     | SS14               | SMA         | 1   |
| LED1   | LED rouge          | 0603        | 1   |
| LED2   | LED verte          | 0603        | 1   |
| R1     | 10kΩ (pull-up EN)  | 0402        | 1   |
| R2     | 10kΩ (pull-up GPIO0)| 0402       | 1   |
| R3     | 1.2kΩ (RPROG)      | 0402        | 1   |
| R4     | 1kΩ (LED1)         | 0402        | 1   |
| R5     | 1kΩ (LED2)         | 0402        | 1   |
| R6     | 100kΩ (div haut)   | 0402        | 1   |
| R7     | 220kΩ (div bas)    | 0402        | 1   |
| C1     | 10µF (déc. ESP32)  | 0805        | 1   |
| C2     | 100nF (déc. ESP32) | 0402        | 1   |
| C3     | 10µF (TP4056 IN)   | 0805        | 1   |
| C4     | 10µF (TP4056 OUT)  | 0805        | 1   |
| C5     | 1µF (LDO IN)       | 0402        | 1   |
| C6     | 1µF (LDO OUT)      | 0402        | 1   |
| C7     | 100nF (EN filter)  | 0402        | 1   |
| C8     | 100nF (ADC filter) | 0402        | 1   |
| J1     | JST-PH 2 broches   | THT         | 1   |
| J2     | JST-XH 5 broches   | THT         | 1   |
| J3     | Pogo Pins mag. 2p  | Custom      | 1   |
| TP1-6  | Test Points (prog) | Pad SMD     | 6   |
| MH1-4  | Trous de montage   | ø 3mm      | 4   |

**Total : 30 composants** (hors trous de montage et test points)

---

## 5. Contraintes de placement (Layout)

### 5.1 Dimensions

- **Cible :** 45mm × 65mm (marge ajoutée vs 40×60 pour accueillir tous les composants)
- **Couches :** 2 (Top + Bottom)
- **Épaisseur :** 1.6mm standard

### 5.2 Zones du PCB

```
           BORD SUPÉRIEUR (côté antenne)
    ┌──────────────────────────────────────────┐
    │ MH1              [ANTENNE]           MH2 │
    │          ┌──────────────────┐             │
    │          │  ESP32-WROOM-32  │             │
    │          │      (U1)        │             │
    │          └──────────────────┘             │
    │                                          │
    │  TP1-6    R1 R2 C1 C2      R6 R7 C8     │
    │  (prog)   (pull-ups)       (ADC div)     │
    │                                          │
    │  U6(LDO)  C5 C6                         │
    │                                          │
    │  U3(TP4056) R3 C3 C4    U2(ULN2003A)   │
    │  U4(DW01A) U5(8205A)                    │
    │                                          │
    │  LED1 LED2  D1  [J3 POGO]     [J2 MOT] │
    │ MH3         BORD INFÉRIEUR          MH4 │
    └──────────────────────────────────────────┘
      ← charge magnétique    connecteur moteur →
```

### 5.3 Règles de placement

| Règle | Détail |
|-------|--------|
| Antenne ESP32 | Dépasse du bord supérieur du PCB. AUCUN plan de masse ni trace de cuivre sous l'antenne (zone d'exclusion ~10mm depuis le bord du module) |
| Connecteur moteur (J2) | Bord droit, au centre vertical, pour câblage vers l'axe du tube |
| Connecteur pogo (J3) | Bord inférieur gauche, accessible depuis l'extérieur du boîtier |
| LEDs (LED1, LED2) | À côté du connecteur pogo, visibles depuis l'extérieur |
| Test points (TP1-6) | Regroupés, accessibles quand le PCB est hors du boîtier |
| Trous de montage | 4 coins, ø 3mm, keepout cuivre ø 6mm |
| Condensateurs de découplage | Au plus près des pins VCC de chaque IC |
| Plan de masse | Top et Bottom, via stitching tous les ~5mm, sauf zone antenne |

---

## 6. Netlist — Connexions complètes

### 6.1 Rail VBAT (3.7-4.2V)

```
J1.+ → U4/U5 (protection) → TP4056.BAT → VBAT
VBAT → U2.COM (ULN2003A, alim moteur)
VBAT → U6.VIN (LDO)
VBAT → R6 (pont diviseur ADC)
J3.VCC → D1.anode ; D1.cathode → U3.VCC (TP4056 IN)
```

### 6.2 Rail 3V3

```
U6.VOUT → 3V3
3V3 → U1.3V3 (ESP32)
3V3 → R1 (pull-up EN)
3V3 → R2 (pull-up GPIO0)
3V3 → C1+ ; C2+ (découplage ESP32)
3V3 → C6+ (découplage LDO OUT)
3V3 → TP (test point 3V3)
```

### 6.3 Signaux moteur

```
U1.GPIO13 → U2.IN1
U1.GPIO12 → U2.IN2
U1.GPIO14 → U2.IN3
U1.GPIO27 → U2.IN4
U2.OUT1 → J2.pin1 (bobine A+)
U2.OUT2 → J2.pin2 (bobine A-)
U2.OUT3 → J2.pin3 (bobine B+)
U2.OUT4 → J2.pin4 (bobine B-)
J2.pin5 → VBAT (alimentation centre moteur)
```

### 6.4 Signaux ADC et LEDs

```
VBAT → R6 → (noeud) → R7 → GND
(noeud) → C8 → GND
(noeud) → U1.GPIO34

U3.CHRG → R4 → LED1.anode ; LED1.cathode → GND
U3.STDBY → R5 → LED2.anode ; LED2.cathode → GND
```

### 6.5 GND (masse commune)

Toutes les masses reliées au plan de masse : J1.−, J3.GND, U1.GND, U2.GND (×4),
U3.GND, U4.GND, U5.GND, U6.GND, C1−, C2−, ..., C8−, LED cathodes.

---

## 7. Estimation de consommation

| Mode               | Courant       | Durée type      |
|---------------------|---------------|-----------------|
| Deep sleep ESP32    | ~10µA         | 99% du temps    |
| BLE advertising     | ~130mA        | ~2s par commande|
| Moteur actif        | ~150mA*       | ~30s par cycle  |
| Charge batterie     | ~1A (entrée)  | ~2h pour 2000mAh|

*Avec ressort contrepoids. Sans ressort : ~350mA.

**Autonomie estimée (batterie 2000mAh, 2 cycles/jour) : 3-6 mois.**

---

## 8. Plan d'implémentation tscircuit

Le code sera structuré en sous-composants modulaires :

```
src/
├── index.tsx                  # Export principal
├── StollperBlindController.tsx # Board principal (45×65mm)
├── components/
│   ├── Esp32Module.tsx        # ESP32 + découplage + pull-ups
│   ├── MotorDriver.tsx        # ULN2003A + connecteur JST-XH
│   ├── PowerManagement.tsx    # TP4056 + DW01A/8205A + LDO
│   ├── BatteryMonitor.tsx     # Pont diviseur + filtrage ADC
│   ├── MagneticCharger.tsx    # Pogo pins + diode protection
│   └── ProgrammingHeader.tsx  # Test points (TX/RX/EN/GPIO0/3V3/GND)
```

Chaque module encapsule ses composants passifs associés pour un code lisible et maintenable.
