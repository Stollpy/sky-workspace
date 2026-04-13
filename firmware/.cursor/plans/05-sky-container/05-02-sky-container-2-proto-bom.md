# 05-02 — Sky Container · BOM Proto V1 (le moins cher possible)

> **Budget estimé : ~$450–500 USD** — tout dans une malette imprimée 3D.

---

## Principe

Un **seul SBC** (Orange Pi 5 Plus 32 Go) remplace le cluster Turing Pi pour le proto.
Même SoC **RK3588**, même **32 Go RAM**, même **NPU 6 TOPS** — mais autonome, sans backplane.
Le cluster (Turing Pi 2.5 + 4× RK1) viendra en **V2** quand le concept est validé.

---

## A. Compute

| # | Composant | Qté | Prix | Lien |
|---|-----------|-----|------|------|
| A1 | **Orange Pi 5 Plus 32 Go** — RK3588, 8 cores (4× A76 + 4× A55), 32 Go LPDDR4X, NPU 6 TOPS, 2× HDMI 2.1 (8K), 2× USB 3.0, 2× USB 2.0, 2× 2.5 GbE, M.2 NVMe (PCIe 3.0 ×4), M.2 E-Key (Wi-Fi) | 1 | ~$150–180 | [orangepi.org](http://www.orangepi.org/html/hardWare/computerAndMicrocontrollers/details/Orange-Pi-5-plus-32GB.html) / AliExpress (Xunlong Store) |
| A2 | **Module Wi-Fi 6 / BT M.2 E-Key** — (AW-CB375NF ou RTL8852BE), 2.4/5 GHz, BLE | 1 | ~$15 | AliExpress (chercher « AW-CB375NF M.2 E-Key ») |
| A3 | **Heatsink + ventilateur** — kit refroidissement OPi5+ (30 mm fan + dissipateur aluminium) | 1 | ~$10 | AliExpress (kit officiel Orange Pi 5 Plus) |
| A4 | **Alimentation USB-C PD 5V/4A** (ou 12V/2A barrel jack selon variante) | 1 | ~$15 | Amazon |

**Sous-total compute : ~$190–220**

---

## B. Stockage

| # | Composant | Qté | Prix | Lien |
|---|-----------|-----|------|------|
| B1 | **NVMe M.2 2280 512 Go** — OS + app cloud + images + modèles GGUF (PCIe Gen3) | 1 | ~$45 | [Waveshare NX-NVME 2280](https://www.waveshare.com/product/nx-nvme-2280-ssd-m.2.htm) / [Newegg ORICO](https://www.newegg.com/orico-256gb-d10/p/0D9-004U-00068) |

**Sous-total stockage : ~$45**

---

## C. Affichage (intégré dans le couvercle)

| # | Composant | Qté | Prix | Lien |
|---|-----------|-----|------|------|
| C1 | **Écran 10.1" IPS tactile HDMI** — 1280×800, capacitif, USB touch | 1 | ~$130 | [APROTII 10"](https://robotpishop.com/products/10-inch-1024x600-ips-capacitive-touchscreen) / [SunFounder 10.1"](https://www.raspberrypiq.com/products/10-inch-touch-screen-raspberry-pi-10.html) |
| C2 | **Câble HDMI plat** — ~20–30 cm, passage par charnière imprimée 3D | 1 | ~$8 | Amazon / AliExpress |
| C3 | **Câble USB plat** — pour le tactile, ~20–30 cm | 1 | ~$5 | Amazon / AliExpress |

**Sous-total affichage : ~$143**

---

## D. Radio & Connectivité

| # | Composant | Qté | Prix | Lien |
|---|-----------|-----|------|------|
| D1 | **ESP32-C6-DevKitC-1** — Thread / Matter / Zigbee border router, Wi-Fi 6, BLE 5.3, 802.15.4 | 1 | ~$10 | [DFRobot](https://www.dfrobot.com/product-2692.html) |
| D2 | **Dongle USB BLE 5.0** — pont BLE ↔ Linux (montre, Luxia) | 1 | ~$15 | TP-Link UB500 (~$15) / [BleuIO](https://novelbits.io/bleuio/) (~$29) |
| D3 | **Câble USB-A court** — ESP32-C6 → USB OPi5+ | 1 | ~$5 | Amazon |

**Sous-total radio : ~$30**

---

## E. Audio (pipeline vocal STT/TTS)

| # | Composant | Qté | Prix | Lien |
|---|-----------|-----|------|------|
| E1 | **Micro USB** — petit micro USB plug-and-play, compatible Linux | 1 | ~$8 | Amazon (chercher « mini USB microphone Linux ») |
| E2 | **Haut-parleur 3 W** + **ampli MAX98357A I2S** — pour la sortie TTS | 1 | ~$10 | AliExpress / Adafruit MAX98357A breakout |

> Optionnel au début : tu peux commencer avec un **casque USB / jack** branché directement.

**Sous-total audio : ~$18**

---

## F. Enclosure — Malette imprimée 3D

| # | Composant | Qté | Prix | Notes |
|---|-----------|-----|------|-------|
| F1 | **Filament PLA/PETG** — ~200–300 g pour la malette (base + couvercle) | 1 bobine | ~$20 | PETG recommandé (plus résistant à la chaleur que le PLA) |
| F2 | **Charnière piano** ou **charnières imprimées** — liaison base ↔ couvercle | 2 | ~$3 | Quincaillerie / imprimé (type print-in-place hinge) |
| F3 | **Loquets / fermoirs** — snap-fit imprimés ou petits fermoirs métal | 2 | ~$3 | Quincaillerie |
| F4 | **Inserts filetés M3 thermiques** — pour monter l'OPi5+, l'écran, les modules | 1 lot (20 pcs) | ~$5 | Amazon (« M3 heat set inserts ») |
| F5 | **Vis M3 × 6 mm** — fixation des composants | 1 lot | ~$3 | Amazon |
| F6 | **Entretoises M2.5/M3** — surélever l'OPi5+ pour ventilation | 1 lot | ~$5 | Amazon |
| F7 | **Connecteurs bulkhead** — barrel jack (alim), RJ45 (Ethernet uplink), USB-C (accès direct) — montés sur paroi de la malette | 1 lot | ~$15 | Amazon |
| F8 | **Interrupteur rocker** — ON/OFF, montage panneau | 1 | ~$3 | Amazon / AliExpress |
| F9 | **Pieds caoutchouc** — autocollants pour le dessous | 4 | ~$2 | Amazon |

**Sous-total enclosure : ~$59**

---

## G. Charge Qi (optionnel proto)

| # | Composant | Qté | Prix | Lien |
|---|-----------|-----|------|------|
| G1 | **Module Qi TX 15 W + bobine** — intégré dans le couvercle, à côté de l'écran | 1 | ~$15 | [ShopBu](https://shopbu.com/product/10w-15w-high-power-wireless-charger-transmitter-module-type-c-coil-for-qi-standard-fast-charging-circuit-board-with-protection/) / AliExpress |

> Peut être ajouté **après** la validation du LLM + dashboard. Non bloquant.

**Sous-total Qi : ~$15**

---

## Récapitulatif

| Catégorie | Sous-total |
|-----------|-----------|
| **A. Compute** (OPi5+ 32 Go + Wi-Fi + cooling + alim) | ~$205 |
| **B. Stockage** (NVMe 512 Go) | ~$45 |
| **C. Affichage** (écran 10" + câbles) | ~$143 |
| **D. Radio** (ESP32-C6 + BLE dongle) | ~$30 |
| **E. Audio** (micro + speaker) | ~$18 |
| **F. Enclosure** (impression 3D + quincaillerie) | ~$59 |
| | |
| **TOTAL proto essentiel** (sans Qi) | **~$500** |
| **TOTAL avec Qi** | **~$515** |

---

## Layout malette imprimée 3D

```
┌─────────────────────────────────────┐
│           COUVERCLE (intérieur)     │
│                                     │
│  ┌───────────────────────────┐      │
│  │   Écran 10.1" IPS tactile │      │
│  │   (fixé par inserts M3)   │      │
│  └───────────────────────────┘      │
│                        [Qi pad]     │
│                                     │
├── charnières ───────────────────────┤
│           BASE                      │
│                                     │
│  ┌──────────────┐  ┌─────────────┐  │
│  │ Orange Pi 5+ │  │ ESP32-C6    │  │
│  │ + NVMe       │  │ BLE dongle  │  │
│  │ + Wi-Fi mod  │  │ Micro USB   │  │
│  │ + heatsink   │  │ Speaker+amp │  │
│  └──────────────┘  └─────────────┘  │
│                                     │
│  [Barrel jack] [RJ45] [USB-C] [ON] │
│          (paroi latérale)           │
│  ~~~~~~~~~ grille ventilation ~~~~~ │
└─────────────────────────────────────┘
```

### Conseils impression 3D

- **Matériau** : **PETG** (résiste à ~80 °C vs ~60 °C pour le PLA) — important vu la chaleur du RK3588
- **Épaisseur parois** : 2–3 mm min pour rigidité
- **Grilles de ventilation** : percer en bas et sur les côtés (hex pattern ou slots)
- **Charnière** : soit print-in-place (type living hinge PETG), soit charnière piano collée/vissée
- **Câbles HDMI + USB** : prévoir un **canal de passage** dans la charnière (trou oblong ~10 mm)
- **Inserts thermiques M3** : fondre à 220 °C avec le fer à souder pour un montage solide et démontable

---

## Check-list de commande

- [ ] 1× Orange Pi 5 Plus 32 Go
- [ ] 1× Module Wi-Fi M.2 E-Key (AW-CB375NF / RTL8852BE)
- [ ] 1× Kit heatsink + fan pour OPi5+
- [ ] 1× Alim USB-C PD 5V/4A
- [ ] 1× NVMe M.2 2280 512 Go
- [ ] 1× Écran 10.1" IPS tactile HDMI
- [ ] 1× Câble HDMI plat + 1× câble USB plat (~25 cm)
- [ ] 1× ESP32-C6-DevKitC-1
- [ ] 1× Dongle BLE USB (TP-Link UB500 ou BleuIO)
- [ ] 1× Câble USB-A court
- [ ] 1× Mini micro USB
- [ ] 1× Breakout MAX98357A + petit speaker 3 W
- [ ] 1× Bobine PETG (ou PLA+)
- [ ] 1× Lot inserts M3 thermiques + vis M3
- [ ] 1× Lot entretoises M2.5/M3
- [ ] 1× Lot connecteurs bulkhead (barrel jack, RJ45, USB-C)
- [ ] 1× Interrupteur rocker
- [ ] 4× Pieds caoutchouc
- [ ] _(optionnel)_ 1× Module Qi TX 15 W

---

## Comparatif Proto V1 vs Produit V2

| | **Proto V1** (ce plan) | **Produit V2** (futur) |
|---|----------------------|----------------------|
| Compute | 1× Orange Pi 5 Plus 32 Go | Turing Pi 2.5 + 4× RK1 |
| k3s | Single node | Multi-nœuds |
| LLM | Gemma 4 E4B (8B) → rapide | Mistral Small 4 (119B) distribué |
| Stockage | 1× NVMe 512 Go | 4× NVMe |
| Enclosure | Imprimée 3D | Pelican 1450 / injection custom |
| Budget | **~$500** | **~$2 000–2 400** |
