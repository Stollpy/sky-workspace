# Luxia — Schéma filaire complet (à reproduire dans KiCad)

> **Légende :**
>
> - `───` = fil (même net)
> - `─┬─` = jonction (T, le fil se divise)
> - `─┼─` = croisement SANS contact (fils qui se croisent sans se toucher)
> - `(net NAME)` = nom du net à utiliser comme label dans KiCad
> - Chaque ligne `Ref.Pin ─── Ref.Pin` = **un fil à tirer**

---

# ════════════════════════════════════════════════

# PCB PRINCIPAL — Luxia Main

# ════════════════════════════════════════════════

## 1. Bus VBAT (net: VBAT)

Tous ces pins sont sur le MÊME fil :

```
                         ┌─── L1.Pin_1
                         │
                         ├─── U4.IN (pin 5)
                         │
J1.Pin_1 ────────────────┼─── U4.EN (pin 4)
  (VBAT)                 │
                         ├─── U5.VIN (pin 3)
                         │
                         ├─── R1.Pin_1
                         │
                         ├─── C4.+
                         │
                         └─── C2.Pin_1
```

## 2. Boost MT3608 (U4) — VBAT → 5V

```
(VBAT) ─── L1.Pin_1 ─── L1 ─── L1.Pin_2 ───┬─── U4.SW (pin 1)
                                              │
                                         (net: SW_NODE)
                                              │
                                           D1.A (anode)
                                              │
                                             D1
                                              │
                                           D1.K (cathode)
                                              │
                                         (net: 5V)
                                              │
                         ┌────────────────────┤
                         │                    │
                    R_fb1.Pin_1          (suite bus 5V
                         │                ci-dessous)
                    R_fb1 (100kΩ)
                         │
                    R_fb1.Pin_2
                         │
                    (net: FB_NODE) ─── U4.FB (pin 3)
                         │
                    R_fb2.Pin_1
                         │
                    R_fb2 (33kΩ)
                         │
                    R_fb2.Pin_2
                         │
                       (GND)
```

> **IMPORTANT** : R_fb1.Pin_1 doit être sur le net **5V** (côté cathode de D1),
> PAS sur le SW_NODE (côté anode de D1).

U4 résumé broches :

```
U4.SW  (pin 1) ─── (SW_NODE)
U4.GND (pin 2) ─── (GND)
U4.FB  (pin 3) ─── (FB_NODE)
U4.EN  (pin 4) ─── (VBAT)
U4.IN  (pin 5) ─── (VBAT)
U4.NC  (pin 6) ─── rien
```

## 3. Bus 5V (net: 5V)

Tous ces pins sont sur le MÊME fil :

```
                         ┌─── D1.K (cathode)
                         │
                         ├─── C5.+
                         │
         (5V) ───────────┼─── U2.COM (pin 9)
                         │
                         ├─── J2.Pin_1
                         │
                         └─── R_fb1.Pin_1
```

Et les condensateurs de filtrage :

```
C5.+  ─── (5V)
C5.−  ─── (GND)
```

## 4. LDO AMS1117 (U5) — VBAT → 3V3

```
U5.VIN  (pin 3) ─── (VBAT)
U5.GND  (pin 1) ─── (GND)
U5.VOUT (pin 2) ─── (3V3)
U5.Tab          ─── (3V3)
```

Condensateurs d'entrée et sortie :

```
C2.Pin_1 ─── (VBAT)          C3.+  ─── (3V3)
C2.Pin_2 ─── (GND)           C3.−  ─── (GND)
```

## 5. Bus 3V3 (net: 3V3)

Tous ces pins sont sur le MÊME fil :

```
                         ┌─── U5.VOUT (pin 2)
                         │
                         ├─── U1.3V3
                         │
                         ├─── C3.+
                         │
        (3V3) ───────────┼─── C7.Pin_1
                         │
                         ├─── R4.Pin_1
                         │
                         ├─── R5.Pin_1
                         │
                         └─── J3.Pin_1
```

Bypass ESP32 :

```
C7.Pin_1 ─── (3V3)
C7.Pin_2 ─── (GND)
```

## 6. ESP32-C6 (U1) — Circuit Reset (EN)

```
        (3V3) ─── R4.Pin_1 ─── R4 (10kΩ) ─── R4.Pin_2 ───┬─── U1.EN (pin 3)
                                                            │
                                                       (net: EN_NODE)
                                                            │
                                                       C1.Pin_1
                                                            │
                                                       C1 (100nF)
                                                            │
                                                       C1.Pin_2 ─── (GND)
                                                            │
                                                       SW1.Pin_1
                                                            │
                                                       SW1 (bouton)
                                                            │
                                                       SW1.Pin_2 ─── (GND)
```

> Appuyer sur SW1 → EN tombe à GND → reset ESP32.

## 7. ESP32-C6 (U1) — Circuit Boot (GPIO9)

```
        (3V3) ─── R5.Pin_1 ─── R5 (10kΩ) ─── R5.Pin_2 ───┬─── U1.IO9 (pin 15)
                                                            │
                                                       (net: BOOT_NODE)
                                                            │
                                                       SW2.Pin_1
                                                            │
                                                       SW2 (bouton)
                                                            │
                                                       SW2.Pin_2 ─── (GND)
```

> Maintenir SW2 + reset → mode flash UART.

## 8. Diviseur tension batterie (ADC)

```
        (VBAT) ─── R1.Pin_1 ─── R1 (100kΩ) ─── R1.Pin_2 ───┬─── U1.IO0 (pin 8)
                                                              │
                                                         (net: VBAT_SENSE)
                                                              │
                                                         C6.Pin_1
                                                              │
                                                         C6 (100nF)
                                                              │
                                                         C6.Pin_2 ─── (GND)
                                                              │
                                                         R2.Pin_1
                                                              │
                                                         R2 (100kΩ)
                                                              │
                                                         R2.Pin_2 ─── (GND)
```

> VBAT_SENSE = VBAT × 0.5 → lu par l'ADC de l'ESP32.

## 9. LED Status

```
U1.IO8 (pin 10) ─── R8.Pin_1 ─── R8 (470Ω) ─── R8.Pin_2 ─── D4.A (anode)
                                                                    │
                                                                   D4 (LED bleue)
                                                                    │
                                                               D4.K (cathode) ─── (GND)
```

## 10. Header UART — J3

```
J3.Pin_1 ─── (3V3)
J3.Pin_2 ─── U1.IO16 (pin 25)     (TX)
J3.Pin_3 ─── U1.IO17 (pin 24)     (RX)
J3.Pin_4 ─── (GND)
```

## 11. ULN2003A (U2) — Entrées (depuis ESP32)

```
U1.IO2  (pin 27) ─── U2.1B (pin 1)    IN1 → Stepper
U1.IO3  (pin 26) ─── U2.2B (pin 2)    IN2
U1.IO10 (pin 11) ─── U2.3B (pin 3)    IN3
U1.IO11 (pin 12) ─── U2.4B (pin 4)    IN4
```

Entrées non utilisées → GND :

```
U2.5B (pin 5) ─── (GND)
U2.6B (pin 6) ─── (GND)
U2.7B (pin 7) ─── (GND)
```

## 12. ULN2003A (U2) — Sorties → J2 (moteur 28BYJ-48)

```
U2.1C (pin 16, OUT1) ─── J2.Pin_2   (Orange, bobine A)
U2.2C (pin 15, OUT2) ─── J2.Pin_4   (Rose,   bobine B)   ← CROISEMENT !
U2.3C (pin 14, OUT3) ─── J2.Pin_3   (Jaune,  bobine C)   ← CROISEMENT !
U2.4C (pin 13, OUT4) ─── J2.Pin_5   (Bleu,   bobine D)
```

> Attention : O2 va sur Pin_4 et O3 va sur Pin_3 (pas droit).

Sorties non utilisées → laisser NC :

```
U2.5C (pin 12) ─── NC
U2.6C (pin 11) ─── NC
U2.7C (pin 10) ─── NC
```

Alimentations U2 :

```
U2.COM (pin 9) ─── (5V)
U2.GND (pin 8) ─── (GND)
```

## 13. Connecteur moteur J2

```
J2.Pin_1 ─── (5V)           Centre tap (fil rouge)
J2.Pin_2 ─── U2.1C (pin 16) Bobine A (fil orange)
J2.Pin_3 ─── U2.3C (pin 14) Bobine C (fil jaune)
J2.Pin_4 ─── U2.2C (pin 15) Bobine B (fil rose)
J2.Pin_5 ─── U2.4C (pin 13) Bobine D (fil bleu)
```

## 14. Bus GND (net: GND)

Tout ce qui va à GND (un seul net, plan de masse) :

```
J1.Pin_2, U4.GND (pin 2), U5.GND (pin 1), U1.GND (tous pads),
U2.GND (pin 8), U2.5B, U2.6B, U2.7B,
C4.−, C5.−, C2.−, C3.−, C7.Pin_2, C1.Pin_2, C6.Pin_2,
R2.Pin_2, R_fb2.Pin_2,
SW1.Pin_2, SW2.Pin_2,
D4.K, J3.Pin_4
```

## 15. Résumé ESP32-C6 (U1) — toutes les broches utilisées

```
U1.3V3            ─── (3V3)
U1.GND            ─── (GND)         (tous les pads GND)
U1.EN    (pin 3)  ─── (EN_NODE)     → R4 + C1 + SW1
U1.IO0   (pin 8)  ─── (VBAT_SENSE)  → R1/R2 diviseur ADC
U1.IO2   (pin 27) ─── U2.1B         → Stepper IN1
U1.IO3   (pin 26) ─── U2.2B         → Stepper IN2
U1.IO7   (pin 7)  ─── (WAKE)        → bouton wake (voir note)
U1.IO8   (pin 10) ─── R8.Pin_1      → LED status
U1.IO9   (pin 15) ─── (BOOT_NODE)   → R5 + SW2
U1.IO10  (pin 11) ─── U2.3B         → Stepper IN3
U1.IO11  (pin 12) ─── U2.4B         → Stepper IN4
U1.IO16  (pin 25) ─── J3.Pin_2      → TX (UART prog)
U1.IO17  (pin 24) ─── J3.Pin_3      → RX (UART prog)
```

> **Note GPIO7** : besoin d'un R9 (10kΩ) pull-up vers 3V3 et d'un SW3 vers GND.
> Non inclus dans la BOM actuelle — ajouter si le bouton wake est nécessaire.

---

# ════════════════════════════════════════════════

# PCB SATELLITE — Batterie

# ════════════════════════════════════════════════

## 16. USB-C (J_USB) — Entrée 5V charge

```
J_USB.VBUS ───┬─── U3.VCC  (pin 4)
              │
              ├─── U3.CE   (pin 8)
              │
              ├─── C8.Pin_1
              │
              ├─── R6.Pin_1
              │
         (net: VBUS)
              │
              └─── R7.Pin_1


J_USB.GND  ─── (GND)
J_USB.D+   ─── NC
J_USB.D−   ─── NC
J_USB.CC1  ─── R_CC1.Pin_1 ─── R_CC1 (5.1kΩ) ─── R_CC1.Pin_2 ─── (GND)
J_USB.CC2  ─── R_CC2.Pin_1 ─── R_CC2 (5.1kΩ) ─── R_CC2.Pin_2 ─── (GND)
```

Filtrage VBUS :

```
C8.Pin_1 ─── (VBUS)
C8.Pin_2 ─── (GND)
```

## 17. TP4056 (U3) — Chargeur LiPo

```
U3.TEMP  (pin 1) ─── (GND)          Pas de NTC
U3.PROG  (pin 2) ─── R3.Pin_1
U3.GND   (pin 3) ─── (GND)
U3.VCC   (pin 4) ─── (VBUS)
U3.BAT   (pin 5) ─── (BAT_PROT)     Bus batterie protégé
U3.STDBY (pin 6) ─── D3.K           LED standby (cathode)
U3.CHRG  (pin 7) ─── D2.K           LED charge (cathode)
U3.CE    (pin 8) ─── (VBUS)         Toujours actif
```

Résistance de programmation courant :

```
R3.Pin_1 ─── U3.PROG (pin 2)
R3.Pin_2 ─── (GND)
```

> I_charge = 1000/R3 = 1000/2000 = 500mA

## 18. LEDs de charge

LED rouge (charge en cours) :

```
(VBUS) ─── R6.Pin_1 ─── R6 (1kΩ) ─── R6.Pin_2 ─── D2.A ─── D2 ─── D2.K ─── U3.CHRG (pin 7)
```

LED verte (charge terminée) :

```
(VBUS) ─── R7.Pin_1 ─── R7 (1kΩ) ─── R7.Pin_2 ─── D3.A ─── D3 ─── D3.K ─── U3.STDBY (pin 6)
```

> CHRG et STDBY sont open-drain : ils tirent vers GND quand actifs.
> Le courant va : VBUS → R → LED (A→K) → pin TP4056 → GND interne.

## 19. Bus BAT_PROT (net: BAT_PROT)

Tous ces pins sont sur le MÊME fil :

```
                              ┌─── U3.BAT (pin 5)
                              │
                              ├─── U6.VCC (pin 5)
                              │
      (BAT_PROT) ─────────────┼─── J_BAT.Pin_1 (B+)
                              │
                              ├─── J_MAG.Pin_1
                              │
                              └─── C9.+
```

Filtrage :

```
C9.+  ─── (BAT_PROT)
C9.−  ─── (GND)
```

## 20. Protection DW01A (U6) + FS8205A (Q1)

```
                (BAT_PROT)
                    │
               U6.VCC (pin 5)
                    │
              ┌─── [DW01A U6] ───┐
              │                   │
         U6.OD (pin 1)      U6.OC (pin 3)
              │                   │
              │                   │
         Q1.G1 (pin 3)      Q1.G2 (pin 4)
              │                   │
              └─── [FS8205A Q1] ──┘
                    │
              Q1.D (pins 7,8)
             (net: MID_PROT)
                    │
              U6.CS (pin 2)


U6.VSS (pin 6)  ─── Q1.S1 (pins 1,2) ─── (GND)
U6.TD  (pin 4)  ─── NC

Q1.S2  (pins 5,6) ─── J_BAT.Pin_2 (B−)  ─── (net: BAT_MINUS)
```

Résumé des connexions U6 :

```
U6.OD  (pin 1) ─── Q1.G1 (pin 3)        Contrôle sur-décharge
U6.CS  (pin 2) ─── Q1.D  (pins 7,8)     Sense courant (drain commun)
U6.OC  (pin 3) ─── Q1.G2 (pin 4)        Contrôle sur-charge
U6.TD  (pin 4) ─── NC
U6.VCC (pin 5) ─── (BAT_PROT)
U6.VSS (pin 6) ─── (GND)                Via Q1.S1
```

Résumé des connexions Q1 :

```
Q1.S1 (pins 1,2) ─── (GND)              Sortie GND protégée
Q1.G1 (pin 3)    ─── U6.OD (pin 1)
Q1.G2 (pin 4)    ─── U6.OC (pin 3)
Q1.S2 (pins 5,6) ─── J_BAT.Pin_2 (B−)   Batterie négatif
Q1.D  (pins 7,8) ─── U6.CS (pin 2)      Drain commun
```

> ⚠️ Vérifier chaque pin avec les datasheets DW01A et FS8205A de ton fournisseur.

## 21. Connecteur batterie J_BAT

```
J_BAT.Pin_1 (B+) ─── (BAT_PROT)
J_BAT.Pin_2 (B−) ─── Q1.S2 (pins 5,6)   ─── (BAT_MINUS)
```

## 22. Connecteur magnétique satellite J_MAG

```
J_MAG.Pin_1 ─── (BAT_PROT)    → envoie VBAT+ au PCB principal
J_MAG.Pin_2 ─── (GND)         → GND commun
```

---

# ════════════════════════════════════════════════

# LIAISON INTER-PCB (câble magnétique)

# ════════════════════════════════════════════════

```
╔═══════════════╗                    ╔═══════════════╗
║   SATELLITE   ║                    ║   PRINCIPAL   ║
║               ║                    ║               ║
║  J_MAG.Pin_1 ─╫──── VBAT+ ────────╫─ J1.Pin_1     ║
║  J_MAG.Pin_2 ─╫──── GND ──────────╫─ J1.Pin_2     ║
║               ║                    ║               ║
╚═══════════════╝                    ╚═══════════════╝
```

---

# ════════════════════════════════════════════════

# INDEX DES NETS

# ════════════════════════════════════════════════


| Net            | Tension    | PCB       | Rôle                   |
| -------------- | ---------- | --------- | ---------------------- |
| **GND**        | 0V         | Les deux  | Masse commune          |
| **VBAT**       | 3.0–4.2V   | Principal | Entrée batterie        |
| **5V**         | 5.0V       | Principal | Sortie boost MT3608    |
| **3V3**        | 3.3V       | Principal | Sortie LDO AMS1117     |
| **SW_NODE**    | oscillant  | Principal | Nœud commutation boost |
| **FB_NODE**    | 0.6V       | Principal | Feedback boost         |
| **VBAT_SENSE** | ~1.5–2.1V  | Principal | Diviseur ADC batterie  |
| **EN_NODE**    | ~3.3V      | Principal | Reset ESP32            |
| **BOOT_NODE**  | ~3.3V      | Principal | Boot mode ESP32        |
| **VBUS**       | 5V USB     | Satellite | Entrée USB-C           |
| **BAT_PROT**   | 3.0–4.2V   | Satellite | Bus batterie protégé   |
| **BAT_MINUS**  | ref. basse | Satellite | Négatif cellule LiPo   |
| **MID_PROT**   | ~0V        | Satellite | Drain commun FS8205A   |


