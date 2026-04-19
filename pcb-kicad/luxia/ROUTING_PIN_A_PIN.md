# Luxia — Guide de routage pin à pin

> **Comment lire ce fichier :**
> - Chaque composant liste **toutes ses broches** et **exactement où elles vont**.
> - Notation : `Ref.Pin_Name` → `Ref.Pin_Name` = une piste (même fil / même net).
> - **Net** = un nom de « fil virtuel » qui relie plusieurs composants (ex. `VBAT`, `GND`, `3V3`, `5V`).
> - **NC** = Non Connecté (la broche ne va nulle part).
> - Les résistances/condensateurs/inductances n'ont pas de sens : Pin_1 et Pin_2 sont interchangeables (sauf condensateurs polarisés : + et −).
> - Les LEDs et diodes ont un **sens** : A = Anode (+), K = Cathode (−). Le courant entre par A et sort par K.
> - **Vérifier systématiquement chaque CI (U1–U6, Q1) avec sa datasheet** pour confirmer les noms/numéros de broches.

---

# ════════════════════════════════════════
# PARTIE 1 — PCB PRINCIPAL (Luxia Main)
# ════════════════════════════════════════

## Résumé des nets du PCB Principal

| Net | Description | Tension |
|-----|-------------|---------|
| **VBAT** | Bus batterie depuis le satellite | 3.0V – 4.2V |
| **GND** | Masse commune (0V) | 0V |
| **5V** | Sortie du boost MT3608 | 5.0V |
| **3V3** | Sortie du LDO AMS1117 | 3.3V |
| **VBAT_SENSE** | Point milieu du diviseur R1/R2, vers ADC | ~1.5V–2.1V |
| **SW_NODE** | Nœud de commutation du boost (L1 ↔ U4.SW ↔ D1.A) | variable |
| **FB_NODE** | Nœud feedback du boost (R_fb1 ↔ R_fb2 ↔ U4.FB) | 0.6V |
| **EN_NODE** | Broche EN de l'ESP32 (R4 + C1 + SW1) | ~3.3V |
| **BOOT_NODE** | GPIO9 de l'ESP32 (R5 + SW2) | ~3.3V |

---

## J1 — Connecteur magnétique femelle (Conn_01x02)

> Reçoit l'alimentation depuis le PCB satellite.

| Broche J1 | Va vers | Net |
|-----------|---------|-----|
| `J1.Pin_1` | `U4.VIN` (pin 5), `U5.VIN` (pin 3), `L1.Pin_1`, `R1.Pin_1`, `C4.+`, `C2.Pin_1` | **VBAT** |
| `J1.Pin_2` | Toutes les masses | **GND** |

---

## U4 — MT3608 (SOT-23-6) — Boost VBAT → 5V

> Convertisseur boost. Brochage datasheet MT3608 SOT-23-6.

| Broche U4 | N° pin | Va vers | Net |
|-----------|--------|---------|-----|
| `U4.SW` | pin 1 | `L1.Pin_2`, `D1.A` | **SW_NODE** |
| `U4.GND` | pin 2 | Masse | **GND** |
| `U4.FB` | pin 3 | `R_fb1.Pin_2`, `R_fb2.Pin_1` | **FB_NODE** |
| `U4.EN` | pin 4 | `U4.VIN` (pin 5) — reliés ensemble | **VBAT** |
| `U4.VIN` | pin 5 | `J1.Pin_1`, `L1.Pin_1`, `C4.+`, `U4.EN` (pin 4) | **VBAT** |
| `U4.NC` | pin 6 | Rien | **NC** |

---

## L1 — Inductance 22µH (1210)

> Entre l'entrée VBAT et le nœud de commutation.

| Broche L1 | Va vers | Net |
|-----------|---------|-----|
| `L1.Pin_1` | `U4.VIN` (pin 5), `J1.Pin_1` | **VBAT** |
| `L1.Pin_2` | `U4.SW` (pin 1), `D1.A` | **SW_NODE** |

---

## D1 — Diode Schottky SS34 (SMA)

> Redresse le signal boost. Le courant va de SW_NODE vers 5V quand le switch s'ouvre.

| Broche D1 | Va vers | Net |
|-----------|---------|-----|
| `D1.A` (Anode) | `U4.SW` (pin 1), `L1.Pin_2` | **SW_NODE** |
| `D1.K` (Cathode) | `C5.+`, `U2.COM` (pin 9), `J2.Pin_1`, `R_fb1.Pin_1` | **5V** |

---

## R_fb1 — Résistance 100kΩ (diviseur feedback haut)

| Broche R_fb1 | Va vers | Net |
|--------------|---------|-----|
| `R_fb1.Pin_1` | `D1.K`, `C5.+`, `U2.COM` (pin 9), `J2.Pin_1` | **5V** |
| `R_fb1.Pin_2` | `U4.FB` (pin 3), `R_fb2.Pin_1` | **FB_NODE** |

---

## R_fb2 — Résistance 33kΩ (diviseur feedback bas)

| Broche R_fb2 | Va vers | Net |
|--------------|---------|-----|
| `R_fb2.Pin_1` | `U4.FB` (pin 3), `R_fb1.Pin_2` | **FB_NODE** |
| `R_fb2.Pin_2` | Masse | **GND** |

---

## C4 — Condensateur 10µF (filtrage entrée VBAT)

| Broche C4 | Va vers | Net |
|-----------|---------|-----|
| `C4.+` | `J1.Pin_1`, `U4.VIN` (pin 5), `L1.Pin_1` | **VBAT** |
| `C4.−` | Masse | **GND** |

---

## C5 — Condensateur 22µF (filtrage sortie 5V)

| Broche C5 | Va vers | Net |
|-----------|---------|-----|
| `C5.+` | `D1.K`, `U2.COM` (pin 9), `J2.Pin_1`, `R_fb1.Pin_1` | **5V** |
| `C5.−` | Masse | **GND** |

---

## U5 — AMS1117-3.3 (SOT-223) — LDO VBAT → 3.3V

> Brochage datasheet AMS1117 SOT-223.

| Broche U5 | N° pin | Va vers | Net |
|-----------|--------|---------|-----|
| `U5.GND` | pin 1 | Masse | **GND** |
| `U5.VOUT` | pin 2 | `C3.+`, `U1.3V3`, `C7.Pin_1`, `R4.Pin_1`, `R5.Pin_1`, `J3.Pin_1` | **3V3** |
| `U5.VIN` | pin 3 | `J1.Pin_1`, `C2.Pin_1` | **VBAT** |
| `U5.Tab` | tab | `U5.VOUT` (pin 2) — même net | **3V3** |

---

## C2 — Condensateur 10µF (entrée LDO)

| Broche C2 | Va vers | Net |
|-----------|---------|-----|
| `C2.Pin_1` (ou +) | `U5.VIN` (pin 3), `J1.Pin_1` | **VBAT** |
| `C2.Pin_2` (ou −) | Masse | **GND** |

---

## C3 — Condensateur 22µF (sortie LDO)

| Broche C3 | Va vers | Net |
|-----------|---------|-----|
| `C3.+` | `U5.VOUT` (pin 2) | **3V3** |
| `C3.−` | Masse | **GND** |

---

## C7 — Condensateur 100nF (bypass ESP32)

| Broche C7 | Va vers | Net |
|-----------|---------|-----|
| `C7.Pin_1` | `U5.VOUT` (pin 2), `U1.3V3` | **3V3** |
| `C7.Pin_2` | Masse | **GND** |

---

## U1 — ESP32-C6-WROOM-1 (Module)

> Le module a beaucoup de broches. Seules les broches **utilisées** sont listées.
> Les broches GND du module (il y en a plusieurs) vont **toutes** sur le net GND.
> Vérifier les noms exacts des broches dans le symbole KiCad `RF_Module:ESP32-C6-WROOM-1`.

| Broche U1 | Va vers | Net |
|-----------|---------|-----|
| `U1.3V3` | `U5.VOUT` (pin 2), `C7.Pin_1`, `C3.+`, `R4.Pin_1`, `R5.Pin_1`, `J3.Pin_1` | **3V3** |
| `U1.GND` (tous les pads) | Masse | **GND** |
| `U1.EN` | `R4.Pin_2`, `C1.Pin_1`, `SW1.Pin_1` | **EN_NODE** |
| `U1.IO0` | `R1.Pin_2`, `R2.Pin_1`, `C6.Pin_1` | **VBAT_SENSE** |
| `U1.IO2` | `U2.1B` (pin 1) | direct |
| `U1.IO3` | `U2.2B` (pin 2) | direct |
| `U1.IO7` | Bouton Wake externe ⚠️ (voir note ci-dessous) | direct |
| `U1.IO8` | `R8.Pin_1` | direct |
| `U1.IO9` | `R5.Pin_2`, `SW2.Pin_1` | **BOOT_NODE** |
| `U1.IO10` | `U2.3B` (pin 3) | direct |
| `U1.IO11` | `U2.4B` (pin 4) | direct |
| `U1.IO16` | `J3.Pin_2` | direct |
| `U1.IO17` | `J3.Pin_3` | direct |

> ⚠️ **GPIO7 (Wake Button)** : Le SCHEMATIC_REFERENCE mentionne un bouton wake avec
> pull-up 10kΩ vers 3V3 et contact vers GND, mais **aucune référence** (Rxx, SWxx)
> n'apparaît dans la BOM. **Deux options :**
> 1. Ajouter un `R9` (10kΩ, 0603) et un `SW3` (tact 6×6 SMD) à la BOM.
> 2. Omettre le bouton wake si non nécessaire pour le prototype.
>
> Si ajouté :
> - `R9.Pin_1` → **3V3**
> - `R9.Pin_2` → `U1.IO7`, `SW3.Pin_1`
> - `SW3.Pin_2` → **GND**

---

## R4 — Résistance 10kΩ (pull-up EN / Reset)

| Broche R4 | Va vers | Net |
|-----------|---------|-----|
| `R4.Pin_1` | `U5.VOUT` (pin 2), `U1.3V3` | **3V3** |
| `R4.Pin_2` | `U1.EN`, `C1.Pin_1`, `SW1.Pin_1` | **EN_NODE** |

---

## C1 — Condensateur 100nF (filtre EN)

| Broche C1 | Va vers | Net |
|-----------|---------|-----|
| `C1.Pin_1` | `U1.EN`, `R4.Pin_2`, `SW1.Pin_1` | **EN_NODE** |
| `C1.Pin_2` | Masse | **GND** |

---

## SW1 — Bouton Reset (Tactile 6×6mm SMD)

> Quand tu appuies, EN tombe à GND → l'ESP32 redémarre.

| Broche SW1 | Va vers | Net |
|------------|---------|-----|
| `SW1.Pin_1` | `U1.EN`, `R4.Pin_2`, `C1.Pin_1` | **EN_NODE** |
| `SW1.Pin_2` | Masse | **GND** |

---

## R5 — Résistance 10kΩ (pull-up BOOT / GPIO9)

| Broche R5 | Va vers | Net |
|-----------|---------|-----|
| `R5.Pin_1` | `U5.VOUT` (pin 2), `U1.3V3` | **3V3** |
| `R5.Pin_2` | `U1.IO9`, `SW2.Pin_1` | **BOOT_NODE** |

---

## SW2 — Bouton Boot (Tactile 6×6mm SMD)

> Maintenir enfoncé pendant reset → mode flash UART.

| Broche SW2 | Va vers | Net |
|------------|---------|-----|
| `SW2.Pin_1` | `U1.IO9`, `R5.Pin_2` | **BOOT_NODE** |
| `SW2.Pin_2` | Masse | **GND** |

---

## R1 — Résistance 100kΩ (diviseur tension — haut)

| Broche R1 | Va vers | Net |
|-----------|---------|-----|
| `R1.Pin_1` | `J1.Pin_1`, `U4.VIN` (pin 5), `L1.Pin_1` | **VBAT** |
| `R1.Pin_2` | `R2.Pin_1`, `C6.Pin_1`, `U1.IO0` | **VBAT_SENSE** |

---

## R2 — Résistance 100kΩ (diviseur tension — bas)

| Broche R2 | Va vers | Net |
|-----------|---------|-----|
| `R2.Pin_1` | `R1.Pin_2`, `C6.Pin_1`, `U1.IO0` | **VBAT_SENSE** |
| `R2.Pin_2` | Masse | **GND** |

---

## C6 — Condensateur 100nF (filtre ADC)

| Broche C6 | Va vers | Net |
|-----------|---------|-----|
| `C6.Pin_1` | `R1.Pin_2`, `R2.Pin_1`, `U1.IO0` | **VBAT_SENSE** |
| `C6.Pin_2` | Masse | **GND** |

---

## R8 — Résistance 470Ω (LED status)

| Broche R8 | Va vers | Net |
|-----------|---------|-----|
| `R8.Pin_1` | `U1.IO8` | direct |
| `R8.Pin_2` | `D4.A` (anode LED) | direct |

---

## D4 — LED bleue (Status ESP)

> Le courant passe : IO8 → R8 → D4 (anode→cathode) → GND.

| Broche D4 | Va vers | Net |
|-----------|---------|-----|
| `D4.A` (Anode) | `R8.Pin_2` | direct |
| `D4.K` (Cathode) | Masse | **GND** |

---

## J3 — Header UART 1×4 (Programmation)

> Pour flasher l'ESP32 avec un adaptateur USB-UART externe.

| Broche J3 | Va vers | Net |
|-----------|---------|-----|
| `J3.Pin_1` | `U5.VOUT` (pin 2), `U1.3V3` | **3V3** |
| `J3.Pin_2` | `U1.IO16` | direct (TX) |
| `J3.Pin_3` | `U1.IO17` | direct (RX) |
| `J3.Pin_4` | Masse | **GND** |

---

## U2 — ULN2003A (DIP-16) — Driver moteur

> Brochage datasheet ULN2003A DIP-16.
> Entrées (1B–7B) = pins 1–7 ; GND = pin 8 ; COM = pin 9 ; Sorties (7C–1C) = pins 10–16.

| Broche U2 | N° pin | Va vers | Net |
|-----------|--------|---------|-----|
| `U2.1B` (IN1) | pin 1 | `U1.IO2` | direct |
| `U2.2B` (IN2) | pin 2 | `U1.IO3` | direct |
| `U2.3B` (IN3) | pin 3 | `U1.IO10` | direct |
| `U2.4B` (IN4) | pin 4 | `U1.IO11` | direct |
| `U2.5B` (IN5) | pin 5 | Masse (non utilisé) | **GND** |
| `U2.6B` (IN6) | pin 6 | Masse (non utilisé) | **GND** |
| `U2.7B` (IN7) | pin 7 | Masse (non utilisé) | **GND** |
| `U2.E` (GND) | pin 8 | Masse | **GND** |
| `U2.COM` | pin 9 | `D1.K`, `C5.+`, `J2.Pin_1`, `R_fb1.Pin_1` | **5V** |
| `U2.7C` (OUT7) | pin 10 | Rien | **NC** |
| `U2.6C` (OUT6) | pin 11 | Rien | **NC** |
| `U2.5C` (OUT5) | pin 12 | Rien | **NC** |
| `U2.4C` (OUT4) | pin 13 | `J2.Pin_5` (fil bleu — bobine D) | direct |
| `U2.3C` (OUT3) | pin 14 | `J2.Pin_3` (fil jaune — bobine C) | direct |
| `U2.2C` (OUT2) | pin 15 | `J2.Pin_4` (fil rose — bobine B) | direct |
| `U2.1C` (OUT1) | pin 16 | `J2.Pin_2` (fil orange — bobine A) | direct |

---

## J2 — Connecteur JST-XH 5P (Moteur 28BYJ-48)

> Ordre des pins vu de face du connecteur. Couleurs = fils du 28BYJ-48 standard.
> Vérifier les couleurs avec ton moteur spécifique !

| Broche J2 | Couleur fil | Va vers | Net |
|-----------|-------------|---------|-----|
| `J2.Pin_1` | Rouge | `D1.K`, `C5.+`, `U2.COM` (pin 9) | **5V** (centre tap) |
| `J2.Pin_2` | Orange | `U2.1C` (pin 16 — OUT1) | direct (bobine A) |
| `J2.Pin_3` | Jaune | `U2.3C` (pin 14 — OUT3) | direct (bobine C) |
| `J2.Pin_4` | Rose | `U2.2C` (pin 15 — OUT2) | direct (bobine B) |
| `J2.Pin_5` | Bleu | `U2.4C` (pin 13 — OUT4) | direct (bobine D) |

---

# ════════════════════════════════════════
# PARTIE 2 — PCB SATELLITE (Batterie)
# ════════════════════════════════════════

## Résumé des nets du PCB Satellite

| Net | Description | Tension |
|-----|-------------|---------|
| **VBUS** | 5V depuis USB-C (uniquement quand branché) | 5V |
| **GND** | Masse commune | 0V |
| **BAT_PROT** | Bus batterie protégé (B+) | 3.0V – 4.2V |
| **BAT_MINUS** | Borne négative de la cellule LiPo (B−) | ref. basse |
| **MID_PROT** | Nœud entre les 2 MOSFETs (drain commun Q1) | ~0V |

---

## J_USB — Connecteur USB-C (Power only)

> Seuls VBUS, GND, CC1, CC2 sont connectés. Pas de data (D+/D−).

| Broche J_USB | Va vers | Net |
|--------------|---------|-----|
| `J_USB.VBUS` | `U3.VCC` (pin 4), `C8.Pin_1`, `U3.CE` (pin 8), `R6.Pin_1`, `R7.Pin_1` | **VBUS** |
| `J_USB.GND` | Masse | **GND** |
| `J_USB.D+` | Rien | **NC** |
| `J_USB.D−` | Rien | **NC** |
| `J_USB.CC1` | `R_CC1.Pin_1` | direct |
| `J_USB.CC2` | `R_CC2.Pin_1` | direct |

---

## R_CC1 — Résistance 5.1kΩ (identification USB-C sink)

| Broche R_CC1 | Va vers | Net |
|--------------|---------|-----|
| `R_CC1.Pin_1` | `J_USB.CC1` | direct |
| `R_CC1.Pin_2` | Masse | **GND** |

---

## R_CC2 — Résistance 5.1kΩ (identification USB-C sink)

| Broche R_CC2 | Va vers | Net |
|--------------|---------|-----|
| `R_CC2.Pin_1` | `J_USB.CC2` | direct |
| `R_CC2.Pin_2` | Masse | **GND** |

---

## C8 — Condensateur 100nF (filtrage VBUS)

| Broche C8 | Va vers | Net |
|-----------|---------|-----|
| `C8.Pin_1` | `J_USB.VBUS`, `U3.VCC` (pin 4) | **VBUS** |
| `C8.Pin_2` | Masse | **GND** |

---

## U3 — TP4056-42-ESOP8 — Chargeur LiPo

> Brochage datasheet TP4056 ESOP-8.
> Les sorties CHRG et STDBY sont **open-drain** : elles tirent vers GND quand actives.

| Broche U3 | N° pin | Va vers | Net |
|-----------|--------|---------|-----|
| `U3.TEMP` | pin 1 | Masse (pas de NTC) | **GND** |
| `U3.PROG` | pin 2 | `R3.Pin_1` | direct |
| `U3.GND` | pin 3 | Masse | **GND** |
| `U3.VCC` | pin 4 | `J_USB.VBUS`, `C8.Pin_1` | **VBUS** |
| `U3.BAT` | pin 5 | `U6.VCC` (pin 5), `J_BAT.Pin_1`, `J_MAG.Pin_1`, `C9.+` | **BAT_PROT** |
| `U3.STDBY` | pin 6 | `D3.K` (cathode LED verte) | direct |
| `U3.CHRG` | pin 7 | `D2.K` (cathode LED rouge) | direct |
| `U3.CE` | pin 8 | `J_USB.VBUS` (toujours actif) | **VBUS** |

---

## R3 — Résistance 2kΩ (réglage courant de charge)

> I_charge = 1000 / R3 = 1000 / 2000 = 500mA.

| Broche R3 | Va vers | Net |
|-----------|---------|-----|
| `R3.Pin_1` | `U3.PROG` (pin 2) | direct |
| `R3.Pin_2` | Masse | **GND** |

---

## R6 — Résistance 1kΩ (LED charge)

| Broche R6 | Va vers | Net |
|-----------|---------|-----|
| `R6.Pin_1` | `J_USB.VBUS` | **VBUS** |
| `R6.Pin_2` | `D2.A` (anode LED rouge) | direct |

---

## D2 — LED rouge (Charge en cours)

> Allumée quand la batterie charge (U3.CHRG tire vers GND).
> Chemin du courant : VBUS → R6 → D2 (A→K) → U3.CHRG → GND interne.

| Broche D2 | Va vers | Net |
|-----------|---------|-----|
| `D2.A` (Anode) | `R6.Pin_2` | direct |
| `D2.K` (Cathode) | `U3.CHRG` (pin 7) | direct |

---

## R7 — Résistance 1kΩ (LED standby)

| Broche R7 | Va vers | Net |
|-----------|---------|-----|
| `R7.Pin_1` | `J_USB.VBUS` | **VBUS** |
| `R7.Pin_2` | `D3.A` (anode LED verte) | direct |

---

## D3 — LED verte (Charge terminée / Standby)

> Allumée quand la batterie est pleine (U3.STDBY tire vers GND).
> Chemin du courant : VBUS → R7 → D3 (A→K) → U3.STDBY → GND interne.

| Broche D3 | Va vers | Net |
|-----------|---------|-----|
| `D3.A` (Anode) | `R7.Pin_2` | direct |
| `D3.K` (Cathode) | `U3.STDBY` (pin 6) | direct |

---

## C9 — Condensateur 10µF (filtrage BAT)

| Broche C9 | Va vers | Net |
|-----------|---------|-----|
| `C9.+` | `U3.BAT` (pin 5), `U6.VCC` (pin 5), `J_BAT.Pin_1`, `J_MAG.Pin_1` | **BAT_PROT** |
| `C9.−` | Masse | **GND** |

---

## U6 — DW01A (SOT-23-6) — Protection batterie

> Brochage datasheet DW01A SOT-23-6.
> Surveille la tension batterie et contrôle les 2 MOSFETs de Q1.
>
> ⚠️ **Vérifier impérativement le brochage avec la datasheet DW01A de ton fournisseur.**

| Broche U6 | N° pin | Va vers | Net |
|-----------|--------|---------|-----|
| `U6.OD` | pin 1 | `Q1.G1` (pin 3 du FS8205A) — contrôle sur-décharge | direct |
| `U6.CS` | pin 2 | `Q1.D` (pins 7–8 du FS8205A) — sense courant | **MID_PROT** |
| `U6.OC` | pin 3 | `Q1.G2` (pin 4 du FS8205A) — contrôle sur-charge | direct |
| `U6.TD` | pin 4 | Non connecté (ou GND selon datasheet) | **NC** |
| `U6.VCC` | pin 5 | `U3.BAT` (pin 5), `J_BAT.Pin_1`, `C9.+`, `J_MAG.Pin_1` | **BAT_PROT** |
| `U6.VSS` | pin 6 | `Q1.S1` (pins 1–2 du FS8205A) | **GND** |

---

## Q1 — FS8205A (TSSOP-8) — Double MOSFET protection

> Brochage datasheet FS8205A TSSOP-8.
> Contient 2 N-MOSFETs dos-à-dos (drains communs) entre B− et GND externe.
> Quand les 2 sont passants → le courant circule librement entre batterie et sortie.
>
> ⚠️ **Vérifier impérativement le brochage avec la datasheet FS8205A de ton fournisseur.**

| Broche Q1 | N° pin | Va vers | Net |
|-----------|--------|---------|-----|
| `Q1.S1` | pins 1, 2 | `U6.VSS` (pin 6), `J_MAG.Pin_2` (GND sortie) | **GND** |
| `Q1.G1` | pin 3 | `U6.OD` (pin 1) — gate décharge | direct |
| `Q1.G2` | pin 4 | `U6.OC` (pin 3) — gate charge | direct |
| `Q1.S2` | pins 5, 6 | `J_BAT.Pin_2` (B− de la cellule LiPo) | **BAT_MINUS** |
| `Q1.D` | pins 7, 8 | `U6.CS` (pin 2) — drain commun | **MID_PROT** |

---

## J_BAT — Connecteur JST-PH 2P (Batterie LiPo)

> Branche la cellule LiPo. Respecter la polarité !

| Broche J_BAT | Va vers | Net |
|--------------|---------|-----|
| `J_BAT.Pin_1` (B+) | `U3.BAT` (pin 5), `U6.VCC` (pin 5), `C9.+`, `J_MAG.Pin_1` | **BAT_PROT** |
| `J_BAT.Pin_2` (B−) | `Q1.S2` (pins 5–6 du FS8205A) | **BAT_MINUS** |

---

## J_MAG — Connecteur magnétique mâle (Conn_01x02)

> Envoie l'alimentation vers le PCB principal via les pogo pins magnétiques.

| Broche J_MAG | Va vers | Net |
|--------------|---------|-----|
| `J_MAG.Pin_1` | `U3.BAT` (pin 5), `U6.VCC` (pin 5), `J_BAT.Pin_1`, `C9.+` | **BAT_PROT** |
| `J_MAG.Pin_2` | `Q1.S1` (pins 1–2), `U6.VSS` (pin 6) | **GND** |

---

# ════════════════════════════════════════
# PARTIE 3 — LIAISON INTER-PCB
# ════════════════════════════════════════

> Les deux PCBs se connectent physiquement via les pogo pins magnétiques.
> La broche 1 du satellite (mâle) touche la broche 1 du principal (femelle).

| Satellite | Principal | Net |
|-----------|-----------|-----|
| `J_MAG.Pin_1` (VBAT+) | `J1.Pin_1` | **VBAT** / **BAT_PROT** |
| `J_MAG.Pin_2` (GND) | `J1.Pin_2` | **GND** |

---

# ════════════════════════════════════════
# PARTIE 4 — CHECKLIST
# ════════════════════════════════════════

## Vérifications avant fabrication

- [ ] Chaque net **GND** est bien relié à un plan de masse
- [ ] Le net **VBAT** sur le principal ne touche **que** J1.Pin_1 et ses descendants
- [ ] Le net **5V** part de D1.K et va vers C5, U2.COM, J2.Pin_1, R_fb1.Pin_1
- [ ] Le net **3V3** part de U5.VOUT et va vers U1.3V3, C7, C3, R4, R5, J3.Pin_1
- [ ] Les entrées non utilisées du ULN2003A (pins 5–7) sont bien à GND
- [ ] Les sorties non utilisées du ULN2003A (pins 10–12) sont bien NC
- [ ] U4.EN est bien relié à U4.VIN (pas flottant)
- [ ] Le diviseur FB (R_fb1 + R_fb2) donne ~5V (vérifier : 0.6 × (1 + 100k/33k) ≈ 5.0V)
- [ ] Le diviseur ADC (R1 + R2) donne VBAT/2 (ok pour 0–3.3V de l'ADC ESP32-C6)
- [ ] GPIO7 : décider si un bouton wake est ajouté (ajouter R9 + SW3 à la BOM si oui)
- [ ] DW01A / FS8205A : **vérifier chaque pin avec la datasheet** du fournisseur exact
- [ ] TP4056 : **vérifier l'ordre des pins** avec la datasheet du fournisseur (varie selon package)
- [ ] Polarité du connecteur magnétique : impossible d'inverser (détrompeur mécanique)
- [ ] ERC (Electrical Rules Check) dans KiCad : 0 erreur avant de passer au PCB
