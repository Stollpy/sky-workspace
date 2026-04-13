# Phase 1 — BOM & Achats Hardware pour Prototypage

## 1. Plateforme de base : LILYGO T-Watch S3 Plus

La T-Watch S3 Plus est la montre dev **la plus proche** de ton besoin final.
Elle fournit l'ossature firmware (ESP32-S3, écran, audio, GPS, haptique, PMU) sur laquelle tu codes immédiatement.

| Composant embarqué | Puce | Ce que ça couvre |
|---------------------|------|-----------------|
| MCU | ESP32-S3 (16 Mo flash, 8 Mo PSRAM) | Wi-Fi, BLE 5.0 |
| Écran | 1.54" IPS 240×240, ST7789V3, SPI | Affichage |
| Touch | FT6336U (I2C) | Interface tactile |
| Accéléromètre | BMA423 (I2C, 0x19) | Détection mouvement, pas |
| GPS/GNSS | u-blox MIA-M10Q (UART) | Position |
| Audio out | MAX98357A (I2S) | Haut-parleur |
| Micro | SPM1423HM4H-B (PDM) | Entrée voix |
| Haptique | DRV2605 + LRA (I2C, 0x5A) | Vibration |
| PMU | AXP2101 (I2C, 0x34) | Gestion batterie / rails |
| RTC | PCF8563 (I2C, 0x51) | Horloge temps réel |
| LoRa | SX1262 (SPI) | Radio longue portée (bonus) |
| IR | IR12-21C | Émetteur infrarouge (bonus) |

### Où acheter

| Boutique | Lien |
|----------|------|
| **LILYGO officiel** | https://lilygo.cc/products/t-watch-s3-plus |
| **AliExpress (LILYGO store)** | Chercher « LILYGO T-Watch S3 Plus » |
| **Amazon** | Chercher « LILYGO T-Watch S3 Plus » (dispo variable) |

> **Prix estimé** : ~60–80 € selon variante (SX1262 vs SX1280).

---

## 2. Capteurs manquants — Modules breakout I2C

La T-Watch S3 Plus **ne possède pas** : HR, SpO2, gyroscope.
Tu ajoutes ces capteurs en **breakout I2C** sur le bus partagé (GPIO 10 = SDA, GPIO 11 = SCL).

### 2.1. Capteur HR + SpO2 : MAX30102

| Paramètre | Détail |
|-----------|--------|
| Puce | **Maxim MAX30102** |
| Interface | I2C (adresse 0x57) |
| Mesure | Fréquence cardiaque (PPG) + SpO2 (rouge + IR) |
| Taille module | ~14 × 14 mm (breakout classique) |
| Tension | 1.8 V (logique) / 3.3 V ou 5 V (VIN régulé sur breakout) |

#### Où acheter

| Boutique | Lien / Recherche |
|----------|-----------------|
| **Adafruit** | https://www.adafruit.com/product/4263 (breakout MAX30101, compatible) |
| **SparkFun** | https://www.sparkfun.com/products/14045 |
| **AliExpress** | Chercher « MAX30102 breakout module » (~2–5 €) |
| **Amazon** | Chercher « MAX30102 pulse oximeter sensor module » |

### 2.2. IMU 6 axes (accéléromètre + gyroscope) : BMI270

Le BMA423 de la T-Watch ne fait que l'accéléro 3 axes.
Pour le **gyroscope**, il faut un IMU 6 axes.

| Paramètre | Détail |
|-----------|--------|
| Puce | **Bosch BMI270** (ou alternative : ICM-42688-P, LSM6DSO) |
| Interface | I2C (adresse 0x68 ou 0x69) |
| Mesure | Accéléromètre 3 axes + gyroscope 3 axes |
| Taille module | ~12 × 16 mm (breakout) |
| Conso | Ultra basse conso, conçu pour wearables |

#### Où acheter

| Boutique | Lien / Recherche |
|----------|-----------------|
| **Adafruit** | https://www.adafruit.com/product/5975 (BMI270 breakout) |
| **SparkFun** | https://www.sparkfun.com/products/22397 (BMI270) |
| **AliExpress** | Chercher « BMI270 breakout module I2C » |
| **Alternative facile** | **MPU-6050** breakout (~2 €, moins précis mais très courant pour proto) |

> Le **MPU-6050** (adresse 0x68) est le plus facile à trouver et le moins cher pour du prototypage rapide.
> Le **BMI270** est le choix final pour le PCB custom (basse conso wearable).

---

## 3. Connectivité manquante — Modules externes

### 3.1. Module LTE-M + eSIM : SIMCom SIM7080G (ou Quectel BG95-M1)

Pour le cellulaire, tu branches un module en **UART** sur l'ESP32-S3.
Pendant le prototypage, utilise un **breakout SIM7080G** avec nano-SIM d'abord, eSIM MFF2 plus tard sur le PCB custom.

| Paramètre | Détail |
|-----------|--------|
| Puce | **SIMCom SIM7080G** |
| Interface | UART (AT commands) → GPIO libres ou UART partagé |
| Bandes | LTE-M (Cat-M1) + NB-IoT, multi-bande mondial |
| Taille module | ~17 × 16 mm (puce seule) |
| Breakout | Carte eval / breakout dispo |

#### Où acheter

| Boutique | Lien / Recherche |
|----------|-----------------|
| **Waveshare** | https://www.waveshare.com/sim7080g-cat-m-nb-iot-hat.htm (HAT Raspberry Pi, utilisable en UART) |
| **AliExpress** | Chercher « SIM7080G breakout module » ou « SIM7080G EVB » |
| **DFRobot** | Chercher « SIM7080G » |
| **Alternative** | **Quectel BG95-M1** mini PCIe EVK (plus cher, plus pro) |

> **Attention** : sur la T-Watch S3 Plus, presque tous les GPIO sont utilisés.
> Tu devras trouver 2 GPIO libres pour TX/RX UART ou utiliser un **multiplexeur**.
> En proto, tu peux connecter le module LTE à un **second ESP32** dédié qui communique avec la T-Watch en BLE/UART.

### 3.2. Bluetooth Classic (A2DP) — Limitation ESP32-S3

**Problème** : l'ESP32-**S3** ne supporte **que BLE**, pas Bluetooth Classic (A2DP/HFP).
Pour streamer de l'audio vers un speaker/casque Bluetooth :

| Option | Approche |
|--------|----------|
| **Option A** (proto) | Utiliser un **module BT audio** externe type **KCX_BT_EMITTER** (~3 €) branché en I2S sur le MAX98357A |
| **Option B** (PCB custom) | Remplacer l'ESP32-S3 par un **ESP32 classique** (supporte BT Classic) ou ajouter un **co-processeur BT** |
| **Option C** | Accepter BLE Audio (LE Audio / LC3) si tes écouteurs le supportent — standard plus récent |

#### Où acheter (Option A — proto)

| Boutique | Lien / Recherche |
|----------|-----------------|
| **AliExpress** | Chercher « KCX_BT_EMITTER module » ou « Bluetooth 5.0 audio transmitter module I2S » |
| **Amazon** | Chercher « Bluetooth audio transmitter module board » |

---

## 4. Câblage & connectique proto

| Article | Quantité | Usage |
|---------|----------|-------|
| Fils Dupont femelle-femelle | 1 pack (40 pcs) | Connexions I2C, UART |
| Breadboard mini (170 pts) | 1 | Support breakouts |
| Connecteur JST-SH 1.0 mm (optionnel) | Quelques-uns | Connexion propre sur la T-Watch |
| Multimètre | 1 | Debug alimentation |

---

## 5. Récapitulatif BOM prototypage

| # | Composant | Rôle | Prix estimé | Priorité |
|---|-----------|------|-------------|----------|
| 1 | **LILYGO T-Watch S3 Plus** | Base complète (MCU, écran, GPS, audio, haptique, PMU) | ~70 € | P0 — Indispensable |
| 2 | **MAX30102 breakout** | HR + SpO2 | ~4 € | P0 — Indispensable |
| 3 | **MPU-6050 breakout** (proto) ou **BMI270** (final) | Gyroscope + accéléro 6 axes | ~3 € (MPU) / ~12 € (BMI270) | P0 — Indispensable |
| 4 | **SIM7080G breakout** | LTE-M cellulaire (proto avec nano-SIM) | ~15–25 € | P1 — Important |
| 5 | **Nano-SIM data** (forfait IoT) | Tester le cellulaire | ~5 €/mois | P1 — Avec le module LTE |
| 6 | **KCX_BT_EMITTER** ou module BT audio | Bluetooth Classic audio | ~3–5 € | P2 — Plus tard |
| 7 | Fils Dupont + breadboard mini | Câblage proto | ~5 € | P0 |

**Total estimé (P0)** : **~80–95 €**
**Total estimé (tout)** : **~110–130 €**
