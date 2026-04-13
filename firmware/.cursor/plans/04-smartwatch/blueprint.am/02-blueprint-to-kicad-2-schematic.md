# Phase 2 — Schéma KiCad Complet

## Organisation du schéma

Utilise des **feuilles hiérarchiques** (hierarchical sheets) dans KiCad pour garder le schéma lisible :

```
stollper-watch.kicad_sch (racine)
├── sheet: MCU_POWER        ← ESP32 + AXP2101 + TPS62743 + batterie + USB-C
├── sheet: SENSORS           ← MAX30102, BMI270, DRV2605L, PCF8563
├── sheet: DISPLAY_TOUCH     ← ST7789V3 + FT6336U
├── sheet: AUDIO             ← SPM1423HM4H + MAX98357A + speaker
├── sheet: CONNECTIVITY      ← SIM7080G + eSIM + MIA-M10Q + antennes
└── sheet: UI_BUTTONS        ← Boutons, LEDs status (optionnel)
```

---

## 1. GPIO Mapping Corrigé — ESP32-PICO-V3-02

Le pinout ci-dessous est le mapping **corrigé** par rapport à Blueprint.am.
Les GPIO marqués ⚠️ sont ceux qui étaient faux dans le output original.

| GPIO | Fonction | Protocole | Composant | Note |
|------|----------|-----------|-----------|------|
| **0** | ⚠️ **BOOT button** | GPIO (pull-up 10k) | Bouton BOOT | Strapping pin : LOW = download mode |
| **1** | UART0 TX | UART | **Console USB** (garder libre) | ⚠️ Blueprint l'assignait au LTE |
| **3** | UART0 RX | UART | **Console USB** (garder libre) | ⚠️ Blueprint l'assignait au LTE |
| **2** | IR Emitter (optionnel) | GPIO | IR LED | Peut être réassigné |
| **4** | SPI MISO (display) | SPI | ST7789V3 | Non connecté sur la plupart des modules LCD |
| **5** | SPI CS (display) | SPI | ST7789V3 CS | |
| **12** | I2C0 SDA | I2C | Bus partagé (AXP, RTC, IMU, HR, haptic) | Pull-up 4.7 kΩ vers 3.3V |
| **13** | I2C0 SCL | I2C | Bus partagé | Pull-up 4.7 kΩ vers 3.3V |
| **14** | Display DC | GPIO | ST7789V3 DC (Data/Command) | |
| **15** | Display Backlight EN | GPIO | Via AXP2101 ALDO2 ou direct | |
| **16** | ⚠️ **UART2 TX → SIM7080G RX** | UART | SIM7080G | Corrigé : était UART0 |
| **17** | ⚠️ **UART2 RX → SIM7080G TX** | UART | SIM7080G | Corrigé : était UART0 |
| **18** | SPI SCK (display) | SPI | ST7789V3 SCK | VSPI CLK |
| **19** | SPI MOSI (display) | SPI | ST7789V3 MOSI | VSPI MOSI |
| **21** | I2C1 SDA | I2C | FT6336U Touch (bus séparé) | Pull-up 4.7 kΩ |
| **22** | I2C1 SCL | I2C | FT6336U Touch | Pull-up 4.7 kΩ |
| **23** | Touch INT | GPIO (input, pull-up) | FT6336U INT | Interrupt tactile |
| **25** | I2S BCLK | I2S | MAX98357A BCLK | Audio speaker |
| **26** | I2S WCLK (LRCLK) | I2S | MAX98357A WCLK | |
| **27** | I2S DOUT | I2S | MAX98357A DIN | |
| **32** | PDM CLK | PDM | SPM1423HM4H CLK | Micro |
| **33** | PDM DATA | PDM | SPM1423HM4H DATA | |
| **34** | UART1 RX → GPS TX | UART | MIA-M10Q TX | Input only (GPIO34-39) |
| **35** | AXP2101 IRQ | GPIO (input) | AXP2101 IRQ | Power events, ⚠️ POWER button via AXP2101 |
| **36** | MAX30102 INT | GPIO (input) | MAX30102 INT | HR data ready |
| **39** | BMI270 INT1 | GPIO (input) | BMI270 INT1 | Wrist raise, step |

> **GPIO 6–11** : connectés au flash SPI interne sur l'ESP32-PICO-V3-02. **NE PAS UTILISER.**

> **GPIO 34–39** : input-only (pas de pull-up interne). Conviennent pour les interrupts et UART RX.

### UART1 TX pour le GPS

L'ESP32-PICO a UART1 TX sur GPIO10 par défaut, mais GPIO10 est lié au flash.
Utiliser le **remapping UART** d'ESP-IDF pour assigner UART1 TX à un GPIO libre :

```c
uart_set_pin(UART_NUM_1, 25, 34, -1, -1);  // TX=25... mais 25 est pris par I2S
```

**Solution alternative** : le GPS ne nécessite souvent que la **réception** (NMEA).
Configurer UART1 en **RX-only** sur GPIO34 et utiliser UBX pour configurer le module au boot via I2C (MIA-M10Q supporte I2C en plus d'UART).

---

## 2. Schéma Sheet : MCU_POWER

### 2.1. ESP32-PICO-V3-02

```
ESP32-PICO-V3-02
├── VDD3P3 ←── 3.3V (DC1 AXP2101)
├── VDD3P3_RTC ←── 3.3V
├── EN ←── RC reset (10kΩ pull-up + 1µF vers GND + bouton RESET optionnel)
├── GPIO0 ←── 10kΩ pull-up + bouton BOOT (vers GND)
├── CHIP_PU ←── EN (même signal)
└── Decoupling: 3× 100nF + 1× 10µF sur VDD3P3
                1× 100nF sur VDD3P3_RTC
```

### 2.2. AXP2101

```
AXP2101
├── VBUS ←── USB-C VBUS (5V) via diode Schottky ou directement
├── VBAT ←── LiPo 3.7V (+ protection)
├── I2C SDA/SCL → GPIO 12/13 (Bus 0)
├── IRQ → GPIO 35 (power key events, charge status)
├── PWRKEY ←── Bouton POWER externe (pull-up, appui = GND)
│
├── DC1 → 3.3V → ESP32 VDD3P3
├── ALDO2 → Display backlight
├── ALDO3 → 3.3V → Display VCC + Touch VCC
├── ALDO4 → Enable SIM7080G (PWRKEY)
├── BLDO1 → 3.3V → MIA-M10Q GPS VCC
├── BLDO2 → Enable DRV2605L (EN pin)
├── VBACKUP → PCF8563 VBAT (remplace CR1220)
│
└── Decoupling: chaque rail ALDO/BLDO: 1× 1µF
                VBUS: 1× 10µF
                VBAT: 1× 10µF
                DC1: 1× 22µF + 1× 100nF
```

### 2.3. TPS62743 (DC-DC Buck pour LTE)

```
TPS62743
├── VIN ←── VBAT (3.7V directement depuis batterie)
├── VOUT → 3.3V_LTE → SIM7080G VCC_MAIN
├── EN ←── AXP2101 ALDO4 (enable/disable)
├── FB ←── Diviseur résistif (voir datasheet, R1/R2 pour 3.3V)
├── SW → Inductance 2.2µH → VOUT
│
├── Cin: 10µF céramique (X5R, entrée)
├── Cout: 22µF céramique (X5R, sortie)
├── Cbulk: 470µF tantale polymère (sortie, pour pics 2A LTE)
└── L: Inductance 2.2µH, 2A saturation (ex. Murata LQH32CN2R2)
```

> **Attention** : Blueprint.am avait un "1000µF céramique" → remplacé par **470µF tantale polymère** (réaliste) + **22µF céramique** en parallèle.

### 2.4. USB-C

```
USB-C Receptacle
├── VBUS → AXP2101 VBUS (via fusible/PTC 500mA optionnel)
├── GND → GND
├── D+ → ESP32 GPIO 19 (USB D+) ou direct UART0
├── D- → ESP32 GPIO 18 (USB D-) ou direct UART0
├── CC1 → 5.1kΩ vers GND
├── CC2 → 5.1kΩ vers GND
└── SHIELD → GND via 1MΩ + 100pF
```

> CC1/CC2 avec pull-down 5.1 kΩ = signale "device" USB-C (sink 5V).

### 2.5. Batterie LiPo

```
LiPo 3.7V 300-400mAh
├── + → AXP2101 VBAT (charge path)
│    └── aussi → TPS62743 VIN (via trace Power)
├── - → GND
└── Protection: NTC thermistance 10kΩ → AXP2101 TS pin
```

---

## 3. Schéma Sheet : SENSORS

### 3.1. MAX30102 (HR + SpO2)

```
MAX30102
├── VDD → 1.8V (régulateur intégré depuis VLED)
├── VLED → 3.3V (ALDO3 ou rail commun)
├── SDA/SCL → I2C Bus 0 (GPIO 12/13)
├── INT → GPIO 36
├── GND → GND
└── Decoupling: 1× 100nF (VDD) + 1× 1µF (VLED)
```

> **Placement** : bord du PCB, face bottom, aligné avec ouverture boîtier pour contact peau.

### 3.2. BMI270 (IMU 6 axes)

```
BMI270
├── VDD → 1.8V (LDO interne) ou 3.3V avec pull-up level ok
├── VDDIO → 3.3V
├── SDA/SCL → I2C Bus 0 (GPIO 12/13)
├── SDO → GND (adresse 0x68) ou VDD (adresse 0x69)
├── CS → VDD (force mode I2C)
├── INT1 → GPIO 39
├── INT2 → non connecté (optionnel)
└── Decoupling: 1× 100nF (VDD) + 1× 100nF (VDDIO)
```

### 3.3. DRV2605L + LRA

```
DRV2605L
├── VDD → 3.3V
├── EN → AXP2101 BLDO2 (enable/disable pour économie)
├── SDA/SCL → I2C Bus 0 (GPIO 12/13)
├── OUT+ → LRA motor (+)
├── OUT- → LRA motor (-)
├── IN/TRIG → non connecté (mode I2C)
└── Decoupling: 1× 100nF + 1× 10µF
```

### 3.4. PCF8563 (RTC)

```
PCF8563
├── VDD → 3.3V
├── VBAT → AXP2101 VBACKUP (au lieu de coin cell)
├── SDA/SCL → I2C Bus 0 (GPIO 12/13)
├── INT → optionnel (alarme) — peut rester NC
├── CLKOUT → non connecté
└── Decoupling: 1× 100nF
```

---

## 4. Schéma Sheet : DISPLAY_TOUCH

### 4.1. ST7789V3 (via connecteur FPC)

```
FPC Connector 18-24 pins
├── VCC → 3.3V (AXP ALDO3)
├── GND → GND
├── CS → GPIO 5
├── MOSI → GPIO 19
├── SCK → GPIO 18
├── DC → GPIO 14
├── RST → ESP32 EN (ou GPIO dédié, peut aussi être RC reset)
├── BL → GPIO 15 (ou AXP ALDO2 ON/OFF)
└── Decoupling: 1× 100nF + 1× 10µF (VCC)
```

### 4.2. FT6336U (Touch)

```
FT6336U
├── VDD → 3.3V (AXP ALDO3, même que display)
├── SDA → GPIO 21 (I2C Bus 1)
├── SCL → GPIO 22 (I2C Bus 1)
├── INT → GPIO 23 (active low)
├── RST → non connecté sur T-Watch, mais recommandé via GPIO si dispo
└── Decoupling: 1× 100nF
```

---

## 5. Schéma Sheet : AUDIO

### 5.1. SPM1423HM4H-B (Micro PDM)

```
SPM1423HM4H-B
├── VDD → 3.3V
├── GND → GND
├── CLK → GPIO 32
├── DATA → GPIO 33
└── Decoupling: 1× 100nF
```

### 5.2. MAX98357A (Ampli I2S → Speaker)

```
MAX98357A
├── VDD → 3.3V
├── GND → GND
├── BCLK → GPIO 25
├── LRCLK → GPIO 26
├── DIN → GPIO 27
├── SD_MODE → pull-up 100kΩ à VDD (mono L+R/2) ou GPIO pour mute
├── GAIN → configure via résistances (voir datasheet)
├── OUT+ → Speaker (+)
├── OUT- → Speaker (-)
└── Decoupling: 1× 10µF + 1× 100nF
```

---

## 6. Schéma Sheet : CONNECTIVITY

### 6.1. SIM7080G (LTE-M)

```
SIM7080G
├── VCC → 3.3V_LTE (TPS62743 output)
├── GND → GND
├── UART TXD → GPIO 17 (ESP32 UART2 RX)
├── UART RXD → GPIO 16 (ESP32 UART2 TX)
├── PWRKEY → AXP2101 ALDO4 (enable) ou GPIO
├── RESET → GPIO optionnel (ou RC auto-reset)
├── SIM_DATA → eSIM MFF2 IO
├── SIM_CLK → eSIM MFF2 CLK
├── SIM_RST → eSIM MFF2 RST
├── SIM_VCC → eSIM MFF2 VCC (1.8V/3V fourni par SIM7080G)
├── ANT_MAIN → Connecteur U.FL/IPEX → FPC LTE
├── ANT_GNSS → Non utilisé (GPS séparé via MIA-M10Q)
└── Decoupling: 1× 100nF + 1× 10µF + Cbulk 470µF tantale
```

### 6.2. MFF2 eSIM

```
MFF2 eSIM
├── VCC ←── SIM7080G SIM_VCC
├── RST ←── SIM7080G SIM_RST
├── CLK ←── SIM7080G SIM_CLK
├── IO ←── SIM7080G SIM_DATA
├── GND → GND
└── Placer le plus proche possible du SIM7080G (traces courtes)
```

### 6.3. MIA-M10Q (GPS/GNSS)

```
MIA-M10Q
├── VCC → 3.3V (AXP BLDO1, enable/disable)
├── GND → GND
├── TXD → GPIO 34 (ESP32 UART1 RX, input-only = ok)
├── RXD → Non connecté ou via I2C si besoin de config UBX
├── PPS → Non connecté (optionnel)
├── V_BCKP → 3.3V ou VBACKUP pour hot-start
├── RF_IN → Connecteur U.FL/IPEX → FPC GPS
└── Decoupling: 1× 100nF + 1× 10µF
```

### 6.4. Antenne WiFi/BT (ESP32-PICO-V3-02)

L'ESP32-PICO-V3-02 a une **antenne PCB intégrée** dans le module.

> **Règle critique** : maintenir une **zone d'exclusion** (keep-out) de **15 mm minimum** autour de l'antenne côté PCB — pas de cuivre, pas de GND, pas de composants. L'antenne est sur un bord du module.

---

## 7. Récapitulatif condensateurs de découplage ajoutés

| IC | Découplage | Total caps |
|----|-----------|------------|
| ESP32-PICO-V3-02 | 3× 100nF + 1× 10µF (VDD3P3), 1× 100nF (VDD_RTC) | 5 |
| AXP2101 | 1× 10µF (VBUS), 1× 10µF (VBAT), 1× 22µF + 100nF (DC1), 6× 1µF (ALDOs/BLDOs) | 10 |
| TPS62743 | 1× 10µF (Cin), 1× 22µF (Cout), 1× 470µF tantale (Cbulk) | 3 |
| MAX30102 | 1× 100nF + 1× 1µF | 2 |
| BMI270 | 2× 100nF | 2 |
| DRV2605L | 1× 100nF + 1× 10µF | 2 |
| PCF8563 | 1× 100nF | 1 |
| FT6336U | 1× 100nF | 1 |
| SPM1423HM4H | 1× 100nF | 1 |
| MAX98357A | 1× 100nF + 1× 10µF | 2 |
| SIM7080G | 1× 100nF + 1× 10µF + 1× 470µF | 3 |
| MIA-M10Q | 1× 100nF + 1× 10µF | 2 |
| **TOTAL** | | **~34 condensateurs** |

> Blueprint.am en avait listé **0**. C'est le principal ajout.

---

## 8. Checklist Phase 2

- [ ] Schéma hiérarchique créé (5 feuilles)
- [ ] ESP32-PICO-V3-02 placé avec GPIO mapping corrigé
- [ ] AXP2101 câblé avec tous les rails (DC1, ALDO2-4, BLDO1-2, VBACKUP)
- [ ] TPS62743 câblé avec inductance 2.2µH + passifs
- [ ] USB-C avec résistances CC 5.1kΩ
- [ ] Bouton BOOT sur GPIO0 (pull-up 10kΩ)
- [ ] Bouton POWER sur AXP2101 PWRKEY
- [ ] Tous les capteurs I2C sur Bus 0 (GPIO 12/13) avec 4.7kΩ pull-ups
- [ ] Touch sur Bus 1 (GPIO 21/22) avec 4.7kΩ pull-ups
- [ ] SIM7080G sur UART2 (GPIO 16/17) — PAS UART0
- [ ] GPS sur UART1 RX (GPIO 34, input-only)
- [ ] Audio I2S (GPIO 25/26/27) + PDM (GPIO 32/33)
- [ ] ~34 condensateurs de découplage placés
- [ ] ERC (Electrical Rules Check) KiCad = 0 erreurs
