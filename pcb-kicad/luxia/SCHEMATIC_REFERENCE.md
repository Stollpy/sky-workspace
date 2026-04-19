# Luxia PCB — Référence Schéma Électrique

## Vue d'ensemble — Architecture 2 PCBs

Le système Luxia est composé de **2 PCBs indépendants** reliés par un connecteur magnétique :

### PCB Principal (Luxia Main)
- **ESP32-C6-WROOM-1** — MCU Wi-Fi 6 / BLE 5 / Thread (Matter-ready)
- **ULN2003A** — Driver Darlington pour moteur pas à pas
- **28BYJ-48** — Moteur pas à pas 5V unipolaire (4096 pas/tour half-step)
- **MT3608** — Boost converter VBAT → 5V (alimentation moteur)
- **AMS1117-3.3** — LDO 3.3V (alimentation ESP32-C6)
- **Connecteur magnétique** (femelle) — reçoit VBAT + GND du satellite
- **Pas d'USB-C** — toute l'alimentation vient du satellite

### PCB Satellite (Batterie)
- **Batterie LiPo 1S** (3.7V, connecteur JST)
- **TP4056-42-ESOP8** — Chargeur LiPo CC/CV
- **DW01A + FS8205A** — Protection batterie (sur-décharge, sur-charge, court-circuit)
- **USB-C** — recharge de la batterie uniquement (pas de data)
- **Connecteur magnétique** (mâle) — fournit VBAT + GND au principal
- **LEDs** charge/standby

Le satellite se débranche physiquement. Sans satellite → pas de courant → principal éteint.

---

# PARTIE A — PCB PRINCIPAL (Luxia Main)

## A.1 Nomenclature (BOM) — Principal

| Ref | Composant | Valeur / Part Number | Footprint | KiCad Symbol | Qté |
|-----|-----------|---------------------|-----------|-------------|-----|
| U1 | ESP32-C6-WROOM-1 | 4MB Flash | Module SMD | `RF_Module:ESP32-C6-WROOM-1` | 1 |
| U2 | ULN2003A | Darlington Array | DIP-16 ou SOIC-16 | `Transistor_Array:ULN2003A` | 1 |
| U4 | MT3608 | Boost VBAT→5V | SOT-23-6 | `Regulator_Switching:MT3608` | 1 |
| U5 | AMS1117-3.3 | LDO 3.3V | SOT-223 | `Regulator_Linear:AMS1117-3.3` | 1 |
| J1 | Magnetic Pogo Pin | 2P (VBAT+, GND) | Custom pads | `Connector:Conn_01x02` | 1 |
| J2 | JST-XH 5P | Connecteur 28BYJ-48 | JST-XH 2.50mm vertical | `Connector_Generic:Conn_01x05` | 1 |
| J3 | Header 1x4 | Prog UART (3V3/TX/RX/GND) | Pin Header 2.54mm | `Connector:Conn_01x04` | 1 |
| SW1 | Tactile Switch | Reset | 6x6mm SMD | `Switch:SW_Push` | 1 |
| SW2 | Tactile Switch | Boot (GPIO9) | 6x6mm SMD | `Switch:SW_Push` | 1 |
| R1 | Résistance | 100kΩ | 0603 | `Device:R` | 1 |
| R2 | Résistance | 100kΩ | 0603 | `Device:R` | 1 |
| R4 | Résistance (EN pull-up) | 10kΩ | 0603 | `Device:R` | 1 |
| R5 | Résistance (BOOT pull-up) | 10kΩ | 0603 | `Device:R` | 1 |
| R8 | Résistance (LED status) | 470Ω | 0603 | `Device:R` | 1 |
| R_fb1 | Résistance (MT3608 FB) | 100kΩ | 0603 | `Device:R` | 1 |
| R_fb2 | Résistance (MT3608 FB) | 33kΩ | 0603 | `Device:R` | 1 |
| C1 | Condensateur (EN) | 100nF | 0603 | `Device:C` | 1 |
| C2 | Condensateur (LDO in) | 10µF | 0805 | `Device:C_Polarized` | 1 |
| C3 | Condensateur (LDO out) | 22µF | 0805 | `Device:C_Polarized` | 1 |
| C4 | Condensateur (VBAT in) | 10µF | 0805 | `Device:C_Polarized` | 1 |
| C5 | Condensateur (5V out) | 22µF | 0805 | `Device:C_Polarized` | 1 |
| C6 | Condensateur (ADC filter) | 100nF | 0603 | `Device:C` | 1 |
| C7 | Condensateur (ESP32 bypass) | 100nF | 0603 | `Device:C` | 1 |
| L1 | Inductance (MT3608) | 22µH / 1A min | 1210 | `Device:L` | 1 |
| D1 | Diode Schottky (MT3608) | SS34 (3A 40V) | SMA | `Diode:SS34` | 1 |
| D4 | LED bleue | Status ESP | 0603 | `Device:LED` | 1 |

## A.2 Nomenclature (BOM) — Satellite

| Ref | Composant | Valeur / Part Number | Footprint | KiCad Symbol | Qté |
|-----|-----------|---------------------|-----------|-------------|-----|
| U3 | TP4056-42-ESOP8 | Chargeur LiPo | ESOP-8 | `Battery_Management:TP4056` | 1 |
| U6 | DW01A | Protection batterie | SOT-23-6 | `Battery_Management:DW01A` | 1 |
| Q1 | FS8205A | Dual MOSFET | TSSOP-8 | `Transistor_FET:FS8205A` | 1 |
| J_USB | USB-C Receptacle | Power only (pas de data) | USB-C SMD | `Connector:USB_C_Receptacle_USB2.0` | 1 |
| J_MAG | Magnetic Pogo Pin | 2P (VBAT+, GND) | Custom pads | `Connector:Conn_01x02` | 1 |
| J_BAT | JST-PH 2P | Connecteur batterie LiPo | JST-PH 2mm | `Connector:JST_PH_B2B-PH-K` | 1 |
| R3 | Résistance (PROG) | 2kΩ (500mA) | 0603 | `Device:R` | 1 |
| R6 | Résistance (LED CHG) | 1kΩ | 0603 | `Device:R` | 1 |
| R7 | Résistance (LED STBY) | 1kΩ | 0603 | `Device:R` | 1 |
| R_CC1 | Résistance (USB CC1) | 5.1kΩ | 0603 | `Device:R` | 1 |
| R_CC2 | Résistance (USB CC2) | 5.1kΩ | 0603 | `Device:R` | 1 |
| C8 | Condensateur (USB VBUS) | 100nF | 0603 | `Device:C` | 1 |
| C9 | Condensateur (BAT) | 10µF | 0805 | `Device:C_Polarized` | 1 |
| D2 | LED rouge | Charge | 0603 | `Device:LED` | 1 |
| D3 | LED verte | Standby / Full | 0603 | `Device:LED` | 1 |

---

## A.3 Mapping GPIO ESP32-C6 (PCB Principal)

> **IMPORTANT** : Le firmware actuel cible ESP32 (GPIOs 17, 16, 4, 0, 34, 35).
> L'ESP32-C6 a un pinout différent. Voici le remappage nécessaire :

| Fonction | GPIO ESP32 (actuel) | GPIO ESP32-C6 (nouveau) | Type | Notes |
|----------|--------------------|-----------------------|------|-------|
| Stepper IN1 | 17 | **GPIO 2** | Output | Digital |
| Stepper IN2 | 16 | **GPIO 3** | Output | Digital |
| Stepper IN3 | 4 | **GPIO 10** | Output | Digital |
| Stepper IN4 | 0 | **GPIO 11** | Output | Digital |
| Battery ADC | 34 | **GPIO 0** | Analog In | ADC1_CH0, diviseur tension |
| Wake Button | 35 | **GPIO 7** | Input | Pull-up externe, RTC capable |
| Status LED | — | **GPIO 8** | Output | Strapping mais OK en output |
| UART TX (prog) | — | **GPIO 16** | UART | Header de programmation J3 |
| UART RX (prog) | — | **GPIO 17** | UART | Header de programmation J3 |
| EN (Reset) | EN | **EN** | Reset | RC + bouton |
| Boot | — | **GPIO 9** | Input | Strapping, bouton boot |

> **Note** : Pas d'USB-C sur le principal. La programmation initiale se fait via
> le header UART (J3) ou par OTA après le premier flash.

### GPIOs libres restants sur ESP32-C6 :
GPIO 1, 4, 5, 6, 12, 13, 14, 15, 18, 19, 20, 21, 22, 23

---

## A.4 Netlist — PCB Principal

### A.4.1 Entrée alimentation — Connecteur Magnétique (J1)

```
J1.Pin1 (VBAT+) ──── Net VBAT (bus principal)
J1.Pin2 (GND)   ──── GND commun

Net VBAT ────┬───── C4 (10µF) ──── GND (filtrage entrée)
             ├───── U4.VIN (MT3608 boost)
             ├───── U5.VIN (AMS1117 LDO)
             └───── R1 (diviseur tension → ADC)
```

### A.4.2 Boost Converter MT3608 (U4) — VBAT → 5V

Topologie type MT3608 (boost) : **L1** entre **U4.VIN** et **U4.SW** ; **D1** (Schottky) **anode** sur **SW**, **cathode** sur le net **5V**.

```
Net VBAT ────┬───── C4 (10µF) ──── GND
             │
             ├───── U4.VIN ──── L1 (22µH) ──── U4.SW ──── D1 (SS34, anode) ─── D1 (cathode) ─── Net 5V
             │
U4.EN ───────────── U4.VIN (toujours actif)

U4.FB ───────────── Diviseur tension depuis 5V
                     R_fb1 (100kΩ): 5V → FB
                     R_fb2 (33kΩ): FB → GND
                     → Vout = 0.6V × (1 + 100k/33k) ≈ 5.0V
U4.GND ──────────── GND

Net 5V ──────┬───── C5 (22µF) ──── GND
             ├───── U2.COM (ULN2003A pin 9)
             └───── J2.Pin1 (28BYJ-48 red wire, centre tap)
```

### A.4.3 LDO AMS1117-3.3 (U5) — VBAT → 3.3V

```
U5.VIN ──────┬───── Net VBAT
             └───── C2 (10µF) ──── GND
U5.VOUT ─────┬───── Net 3V3
             └───── C3 (22µF) ──── GND
U5.GND ──────────── GND

Net 3V3 ─────┬───── U1.3V3 (ESP32-C6 power)
             ├───── C7 (100nF) ──── GND (bypass ESP32)
             ├───── R4 (10kΩ) ──── U1.EN (pull-up reset)
             └───── R5 (10kΩ) ──── U1.GPIO9 (pull-up boot)
```

### A.4.4 ESP32-C6-WROOM-1 (U1)

```
U1.3V3 ──────────── Net 3V3
U1.GND ──────────── GND (tous les pads GND)
U1.EN ───────┬───── R4 (10kΩ) ──── 3V3
             ├───── C1 (100nF) ──── GND
             └───── SW1 ──── GND (bouton reset)

U1.GPIO9 ────┬───── R5 (10kΩ) ──── 3V3
             └───── SW2 ──── GND (bouton boot)

U1.GPIO2 ────────── U2.IN1 (ULN2003A pin 1) → Stepper IN1
U1.GPIO3 ────────── U2.IN2 (ULN2003A pin 2) → Stepper IN2
U1.GPIO10 ───────── U2.IN3 (ULN2003A pin 3) → Stepper IN3
U1.GPIO11 ───────── U2.IN4 (ULN2003A pin 4) → Stepper IN4

U1.GPIO0 ────────── Midpoint diviseur R1/R2 (VBAT_SENSE) → Battery ADC

U1.GPIO7 ────────── Bouton Wake (pull-up 10kΩ → 3V3, bouton → GND)

U1.GPIO8 ────────── R8 (470Ω) → D4 (LED bleue) → GND

U1.GPIO16 ───────── J3.Pin2 (TX → programmation UART)
U1.GPIO17 ───────── J3.Pin3 (RX → programmation UART)
```

### A.4.5 Header de programmation UART (J3)

```
J3.Pin1 ──── 3V3
J3.Pin2 ──── U1.GPIO16 (TX)
J3.Pin3 ──── U1.GPIO17 (RX)
J3.Pin4 ──── GND
```

> Après le premier flash via UART, les mises à jour se font par OTA (sky_ota).

### A.4.6 ULN2003A (U2) + 28BYJ-48 (J2)

```
U2.IN1 (pin 1) ──── U1.GPIO2
U2.IN2 (pin 2) ──── U1.GPIO3
U2.IN3 (pin 3) ──── U1.GPIO10
U2.IN4 (pin 4) ──── U1.GPIO11
U2.IN5-7 ────────── GND (non utilisés, pull-down)

U2.OUT1 (pin 16) ── J2.Pin2 (Orange — Coil A)
U2.OUT2 (pin 15) ── J2.Pin4 (Pink   — Coil B)
U2.OUT3 (pin 14) ── J2.Pin3 (Yellow — Coil C)
U2.OUT4 (pin 13) ── J2.Pin5 (Blue   — Coil D)

U2.COM (pin 9) ──── Net 5V
U2.GND (pin 8) ──── GND

J2.Pin1 (Red) ───── Net 5V (centre tap moteur)
```

### A.4.7 Diviseur de tension batterie (R1/R2)

```
Net VBAT ──── R1 (100kΩ) ──┬── R2 (100kΩ) ──── GND
                            │
                            ├── C6 (100nF) ──── GND (filtre)
                            │
                            └── U1.GPIO0 (ADC)

VBAT_SENSE = VBAT × R2/(R1+R2) = VBAT × 0.5
Pour VBAT = 4.2V → VBAT_SENSE = 2.1V (dans la plage ADC 0-3.3V de l'ESP32-C6)
```

---

# PARTIE B — PCB SATELLITE (Batterie)

## B.1 Netlist — Satellite

### B.1.1 USB-C (J_USB) — Charge uniquement

```
J_USB.VBUS ──┬───── U3.VCC (TP4056 IN+)
             └───── C8 (100nF) ──── GND
J_USB.GND ───────── GND
J_USB.D- ────────── NC (non connecté — pas de data)
J_USB.D+ ────────── NC (non connecté — pas de data)
J_USB.CC1 ───────── R_CC1 (5.1kΩ) ──── GND
J_USB.CC2 ───────── R_CC2 (5.1kΩ) ──── GND
```

> Les résistances CC1/CC2 = 5.1kΩ identifient le device comme "sink"
> auprès de la source USB-C (chargeur). Obligatoire pour recevoir du 5V.

### B.1.2 Chargeur TP4056-42-ESOP8 (U3)

```
U3.VCC ──────────── J_USB.VBUS (5V USB)
U3.BAT ──────────── Net BAT_PROT (vers protection DW01A)
U3.PROG ─────────── R3 (2kΩ) ──── GND     → I_charge = 1000/2 = 500mA
U3.CHRG ─────────── R6 (1kΩ) → D2 (LED rouge, anode) → VBUS
U3.STDBY ────────── R7 (1kΩ) → D3 (LED verte, anode) → VBUS
U3.TEMP ─────────── GND (pas de NTC, ou 10kΩ NTC si batterie équipée)
U3.CE ───────────── VCC (toujours actif)
U3.GND ──────────── GND
```

### B.1.3 Protection batterie DW01A (U6) + FS8205A (Q1)

```
           ┌────── BAT_PROT (depuis U3.BAT) ────────┐
           │                                          │
     [DW01A U6]                               [FS8205A Q1]
     VCC ← BAT_PROT                          S1 ← BAT-
     VSS ← GND (via Q1)                      S2 ← GND
     OD  → Q1.Gate1 (sur-décharge)            D1,D2 ← BAT- (drains communs)
     OC  → Q1.Gate2 (sur-charge)              G1 ← U6.OD
     CS  ← entre Q1.S1 et GND                G2 ← U6.OC
           │
     J_BAT.B+ ← BAT_PROT (via protection)
     J_BAT.B- ← GND (via Q1 MOSFETs)
```

### B.1.4 Sortie vers connecteur magnétique (J_MAG)

```
J_MAG.Pin1 (VBAT+) ──── Net BAT_PROT (tension batterie protégée, 3.0V–4.2V)
J_MAG.Pin2 (GND)   ──── GND
```

### B.1.5 Batterie LiPo (J_BAT)

```
J_BAT.Pin1 (B+) ──── Net BAT_PROT (via protection)
J_BAT.Pin2 (B-) ──── GND (via protection MOSFETs)
```

---

## 5. Schéma bloc — Système complet

```
╔══════════════════════════════════════════╗
║           PCB SATELLITE                  ║
║                                          ║
║  ┌─────────┐    ┌──────────┐   ┌──────┐ ║
║  │ USB-C   │───→│ TP4056   │──→│ LiPo │ ║
║  │ (charge)│ 5V │ -42-ESOP8│BAT│      │ ║
║  └─────────┘    └──────────┘   └──┬───┘ ║
║                  LED CHG/STBY     │      ║
║                          ┌────────┘      ║
║                          │ DW01A         ║
║                          │ FS8205A       ║
║                          │ (protection)  ║
║                          ▼               ║
║                  ┌──────────────┐        ║
║                  │   MAGNETIC   │        ║
║                  │   CONNECTOR  │        ║
║                  │  (mâle) ●●   │        ║
╚══════════════════╪══════════════╪════════╝
                   │  VBAT   GND  │
          ─ ─ ─ ─ ┤  ~3.7V       ├ ─ ─ ─ ─  (débranchable)
                   │              │
╔══════════════════╪══════════════╪════════╗
║                  │   MAGNETIC   │        ║
║                  │   CONNECTOR  │        ║
║                  │ (femelle) ●● │        ║
║                  └──────┬───────┘        ║
║           PCB PRINCIPAL │                ║
║                         │ VBAT           ║
║              ┌──────────┼──────────┐     ║
║              │          │          │     ║
║              ▼          ▼          ▼     ║
║        ┌──────────┐ ┌──────┐ ┌────────┐ ║
║        │ AMS1117  │ │MT3608│ │Voltage │ ║
║        │  3.3V    │ │  5V  │ │Divider │ ║
║        └────┬─────┘ └──┬───┘ └───┬────┘ ║
║             │3V3       │5V       │ADC   ║
║             ▼          ▼         │      ║
║      ┌────────────┐ ┌────────┐   │      ║
║      │  ESP32-C6  │ │ULN2003A│   │      ║
║      │  WROOM-1   │→│ Driver │   │      ║
║      │            │ └───┬────┘   │      ║
║      │ GPIO2→IN1  │     │        │      ║
║      │ GPIO3→IN2  │     ▼        │      ║
║      │ GPIO10→IN3 │ ┌────────┐   │      ║
║      │ GPIO11→IN4 │ │28BYJ-48│   │      ║
║      │            │ │ Motor  │   │      ║
║      │ GPIO0←ADC──┼─┘        │   │      ║
║      │ GPIO7←Wake │          │   │      ║
║      │ GPIO16→TX──┼── J3 UART│   │      ║
║      │ GPIO17←RX──┼── (prog) │   │      ║
║      └────────────┘ └────────┘   │      ║
╚══════════════════════════════════════════╝
```

### 5.1 Liaison magnétique (inter-PCB) — 2 fils

Les deux cartes ne partagent **que** VBAT et GND. **Même numéro de pin** des deux côtés si le connecteur est miroir correct (Pin1 = +, Pin2 = GND).

| Origine (Satellite) | Destination (Principal) | Net / rôle |
|---------------------|-------------------------|------------|
| `J_MAG.Pin1` | `J1.Pin1` | **VBAT+** (3.0V–4.2V typ., batterie protégée) |
| `J_MAG.Pin2` | `J1.Pin2` | **GND** commun |

> Vérifier le **détrompeur mécanique** : inversion VBAT/GND détruit le circuit.

---

### 5.2 PCB principal — routage « qui va où » (pin à pin)

Notation : **`Signal_A` → `Signal_B`** = une piste / un même net. Les **numéros de broches** ci‑dessous pour **U2** et **J2** suivent la doc Luxia (ULN2003A DIP / 28BYJ‑48 JST‑XH). Pour **U1**, on utilise les **noms de broches du module** `ESP32-C6-WROOM-1` (GPIO, 3V3, GND, EN, etc.) — à faire correspondre au symbole KiCad / placement du module.

#### Alimentation (depuis J1)

| De | Vers | Net |
|----|------|-----|
| `J1.Pin1` | `U4.VIN`, `U5.VIN`, extrémité haute `R1`, `C4`+ | **VBAT** |
| `J1.Pin2` | `U4.GND`, `U5.GND`, `U2.GND` (pin 8), `U1.GND`, tous les GND | **GND** |
| `C4` | entre **VBAT** et **GND** | filtrage entrée |
| `C5` | entre **5V** et **GND** | filtrage sortie boost |
| `C2` | entre **VBAT** et **GND** | entrée LDO |
| `C3` | entre **3V3** et **GND** | sortie LDO |
| `C7` | entre **3V3** et **GND** | bypass ESP32 |

#### MT3608 (U4) — boost VBAT → 5V

Aligné sur **A.4.2** : `L1` entre **`U4.VIN`** et **`U4.SW`** ; `D1` **anode** → `U4.SW`, **cathode** → **5V**.

| Connexion | Net / note |
|-----------|------------|
| `U4.VIN` → `J1.Pin1` (VBAT) via `L1` → `U4.SW` | inductance en série entrée ↔ nœud switch |
| `U4.EN` → `U4.VIN` | boost toujours actif |
| `U4.GND` → GND | |
| `U4.SW` → **anode** `D1` ; **cathode** `D1` → **5V** | diode Schottky |
| `U4.FB` → diviseur : **5V** → `R_fb1` → `U4.FB` → `R_fb2` → GND | réglage ~5 V |
| `C5` | **5V** ↔ GND |

#### AMS1117 (U5) — VBAT → 3V3

| Connexion | Net |
|-----------|-----|
| `U5.VIN` → VBAT | |
| `U5.GND` → GND | |
| `U5.VOUT` → **3V3** | alimente ESP32, pull-ups, J3 pin1 |

#### ESP32-C6 (U1)

| Broche U1 (module) | Connexion |
|--------------------|-----------|
| `3V3` | Net **3V3** |
| `GND` | **GND** (tous les pads GND du module) |
| `EN` | `R4` → **3V3** ; `C1` → GND ; `SW1` → GND (reset) |
| `IO9` (GPIO9) | `R5` → **3V3** ; `SW2` → GND (boot) |
| `IO2` | `U2.IN1` (ULN2003 pin **1**) |
| `IO3` | `U2.IN2` (pin **2**) |
| `IO10` | `U2.IN3` (pin **3**) |
| `IO11` | `U2.IN4` (pin **4**) |
| `IO0` | Point milieu `R1` / `R2` ; `C6` → GND (filtre ADC) |
| `IO7` | Bouton Wake : pull-up → **3V3**, contact → GND *(réf. bouton à nommer sur schéma si absent du BOM)* |
| `IO8` | `R8` → `D4` (anode LED) → **cathode** / GND |
| `IO16` (TX) | `J3.Pin2` |
| `IO17` (RX) | `J3.Pin3` |

#### Header UART J3

| J3 | Connexion |
|----|-----------|
| Pin 1 | **3V3** |
| Pin 2 | `U1.IO16` (TX) |
| Pin 3 | `U1.IO17` (RX) |
| Pin 4 | **GND** |

#### Diviseur batterie (ADC)

| Élément | Connexion |
|---------|-----------|
| `R1` | **VBAT** → nœud **VBAT_SENSE** (milieu) |
| `R2` | **VBAT_SENSE** → GND |
| `C6` | **VBAT_SENSE** → GND |
| `U1.IO0` | **VBAT_SENSE** |

#### ULN2003A (U2) + moteur J2 (JST‑XH 5P)

| U2 (broche) | Vers |
|-------------|------|
| IN1 (pin **1**) | `U1.IO2` |
| IN2 (pin **2**) | `U1.IO3` |
| IN3 (pin **3**) | `U1.IO10` |
| IN4 (pin **4**) | `U1.IO11` |
| IN5–IN7 (pins **5–7**) | **GND** |
| GND (pin **8**) | **GND** |
| COM (pin **9**) | **5V** |
| OUT1 (pin **16**) | `J2.Pin2` (Orange, bobine A) |
| OUT2 (pin **15**) | `J2.Pin4` (Rose, bobine B) |
| OUT3 (pin **14**) | `J2.Pin3` (Jaune, bobine C) |
| OUT4 (pin **13**) | `J2.Pin5` (Bleu, bobine D) |

| J2 (JST‑XH, vue fils 28BYJ‑48) | Net |
|--------------------------------|-----|
| Pin **1** (Rouge) | **5V** (centre tap moteur) |
| Pin **2** | `U2.OUT1` |
| Pin **3** | `U2.OUT3` |
| Pin **4** | `U2.OUT2` |
| Pin **5** | `U2.OUT4` |

> Ordre des couleurs **Pin2–5** : aligné sur **A.4.6** ; le câble du moteur peut varier selon fournisseur — vérifier continuité bobines vs datasheet moteur.

---

### 5.3 PCB satellite — routage « qui va où » (résumé)

#### USB-C `J_USB` (alimentation seule)

| Broche / fonction | Connexion |
|-------------------|-----------|
| VBUS | `U3.VCC`, `C8` → GND, anodes LEDs via résistances (cf. B.1.2) |
| GND | GND |
| D+, D− | **NC** |
| CC1 | `R_CC1` → GND |
| CC2 | `R_CC2` → GND |

#### TP4056 (U3) — extraits

| Signal U3 | Vers |
|-----------|------|
| VCC | `J_USB.VBUS` |
| BAT | Net **BAT_PROT** (vers chaîne protection) |
| PROG | `R3` → GND |
| CHRG / STDBY | LEDs `D2` / `D3` via `R6` / `R7` (cf. B.1.2) |
| TEMP | GND *(ou NTC si utilisé)* |
| CE | VCC |
| GND | GND |

#### Batterie `J_BAT` et sortie `J_MAG`

| Composant | Connexion |
|-----------|-----------|
| `J_BAT` B+ / B− | **BAT_PROT** et **GND** via **U6 + Q1** (détail **B.1.3**) |
| `J_MAG.Pin1` | **BAT_PROT** (VBAT+ protégé) |
| `J_MAG.Pin2` | **GND** |

> Le câblage exact des broches **DW01A** et **FS8205A** dépend du symbole KiCad et du PCB ; suivre la netlist **B.1.3** pour OD/OC/CS et grilles MOSFET.

---

## 6. Notes de conception

### 6.1 Connecteur magnétique
- Utiliser des **pogo pins magnétiques** 2P (VBAT+ et GND suffisent)
- Espacement recommandé: 2.54mm entre pins
- Courant max: vérifier **≥ 1A par pin** (pic moteur ~300mA + marge)
- Le magnétisme assure l'alignement et la connexion automatique
- Prévoir un **détrompeur mécanique** (asymétrie) pour éviter l'inversion de polarité
- Alternative : connecteur 3P ou 4P si besoin futur (NTC, I2C fuel gauge)

### 6.2 Protection batterie (satellite)
- Le TP4056-42-ESOP8 assure la charge CC/CV (4.2V / 500mA avec R3=2kΩ)
- Le **DW01A + FS8205A** protège contre :
  - Sur-décharge (coupure à ~2.5V)
  - Sur-charge (coupure à ~4.3V)
  - Court-circuit
- Se place entre le TP4056 BAT et la cellule LiPo

### 6.3 Programmation du principal (sans USB)
- **Premier flash** : via header UART (J3) avec un adaptateur USB-UART externe
  - Maintenir SW2 (BOOT) → appuyer SW1 (RESET) → relâcher = mode flash
- **Mises à jour suivantes** : OTA via Wi-Fi (sky_ota intégré dans le firmware)
- GPIO 12/13 (USB natif) restent libres pour usage futur

### 6.4 Consommation estimée
| État | Courant (typ.) |
|------|---------------|
| ESP32-C6 idle (Wi-Fi connecté) | ~80mA |
| ESP32-C6 modem sleep | ~15mA |
| ESP32-C6 deep sleep | ~7µA |
| ULN2003A + 28BYJ-48 (actif) | ~200mA |
| **Total max (moteur actif)** | **~300mA** |

### 6.5 Autonomie estimée (batterie 2000mAh)
- Idle Wi-Fi: ~25h
- Modem sleep: ~133h (~5.5 jours)
- Deep sleep: pratiquement illimité (~mois)
- Avec 20 cycles moteur/jour (30s chaque): plusieurs jours

### 6.6 Dimensions PCB suggérées
- **Principal** : ~45mm × 35mm (2 couches)
- **Satellite** : ~25mm × 20mm (2 couches, très compact)
- Plans de masse complets sur la couche bottom des deux PCBs

---

## 7. Modifications firmware requises

Fichier `src/main.c` — remapper les GPIOs pour ESP32-C6 :

```c
// Avant (ESP32)
motor_cfg.gpio.pins[0] = GPIO_NUM_17;
motor_cfg.gpio.pins[1] = GPIO_NUM_16;
motor_cfg.gpio.pins[2] = GPIO_NUM_4;
motor_cfg.gpio.pins[3] = GPIO_NUM_0;

// Après (ESP32-C6)
motor_cfg.gpio.pins[0] = GPIO_NUM_2;
motor_cfg.gpio.pins[1] = GPIO_NUM_3;
motor_cfg.gpio.pins[2] = GPIO_NUM_10;
motor_cfg.gpio.pins[3] = GPIO_NUM_11;
```

Fichier `sky_power.h` — changer le GPIO ADC par défaut :

```c
// Avant (ESP32 — GPIO 34, ADC1)
.adc_pin = GPIO_NUM_34,

// Après (ESP32-C6 — GPIO 0, ADC1_CH0)
.adc_pin = GPIO_NUM_0,
```

Fichier `main.c` — changer le wake GPIO :

```c
// Avant (ESP32 — GPIO 35)
sleep_cfg.wake_gpio = GPIO_NUM_35;

// Après (ESP32-C6 — GPIO 7)
sleep_cfg.wake_gpio = GPIO_NUM_7;
```

Fichier `platformio.ini` — changer la carte :

```ini
[env:luxia]
board = esp32-c6-devkitc-1
```
