# 05-01 — Sky Container · BOM & Références d'Achat

> Budget estimé : **~1 800–2 600 USD** selon config RAM des nœuds.

---

## A. Turing Pi — Cœur du cluster

Tout sur [turingpi.com/shop](https://turingpi.com/shop/).

| # | Composant | Qté | Prix unitaire | Total | Lien |
|---|-----------|-----|--------------|-------|------|
| A1 | **Turing Pi 2.5** — backplane mini-ITX, switch GbE L2, 4 slots, 4× M.2 NVMe, BMC, HDMI 4K, ATX 24-pin | 1 | $279 | $279 | [turingpi.com](https://turingpi.com/product/turing-pi-2-5/) |
| A2 | **Turing RK1 32 Go** — nœud master (RK3588, 8 cores, NPU 6 TOPS, 32 Go RAM) — LLM inference dédié | 1 | ~$319 | $319 | [turingpi.com](https://turingpi.com/product/turing-rk1/) |
| A3 | **Turing RK1 16 Go** — nœuds worker (k3s + apps + services) | 3 | ~$199 | $597 | [turingpi.com](https://turingpi.com/product/turing-rk1/) |
| A4 | **RK1 Heatsink** — dissipateur dédié RK1 | 4 | $12 | $48 | [turingpi.com](https://turingpi.com/shop/) |
| A5 | **24-pin PSU** — alimentation ATX pour Turing Pi 2.5 | 1 | $45 | $45 | [turingpi.com](https://turingpi.com/shop/) |
| A6 | **Power Supply** — bloc secteur compatible (12 V) | 1 | $45 | $45 | [turingpi.com](https://turingpi.com/shop/) |
| A7 | **I/O Shield TP2.5** | 1 | $5 | $5 | [turingpi.com](https://turingpi.com/shop/) |

**Sous-total Turing Pi : ~$1 338**

### Variante "LLM max" (optionnelle)

Pour distribuer **Mistral Small 4 (119B)** sur le cluster entier (128 Go RAM totale) :

| # | Composant | Qté | Prix unitaire | Total |
|---|-----------|-----|--------------|-------|
| A3' | **Turing RK1 32 Go** (remplace les 3× RK1 16 Go) | 3 | ~$319 | $957 |

→ Surcoût : **+$360** ($957 − $597). Total Turing Pi passe à **~$1 698**.

---

## B. Stockage NVMe (1 par nœud)

| # | Composant | Qté | Prix unitaire | Total | Lien |
|---|-----------|-----|--------------|-------|------|
| B1 | **NVMe M.2 2280 256 Go** — OS + données par nœud (PCIe Gen3) | 4 | ~$30–45 | ~$120–180 | [Waveshare NX-NVME](https://www.waveshare.com/product/nx-nvme-2280-ssd-m.2.htm) / [Newegg ORICO 256 Go](https://www.newegg.com/orico-256gb-d10/p/0D9-004U-00068) |

> **Note** : le nœud master (LLM + OS) peut monter en **512 Go** si tu veux stocker plusieurs modèles GGUF localement (~$50–70).

**Sous-total stockage : ~$120–180**

---

## C. Affichage (couvercle de la malette)

| # | Composant | Qté | Prix unitaire | Total | Lien |
|---|-----------|-----|--------------|-------|------|
| C1 | **Écran 10.1" IPS tactile HDMI** — 1280×800, capacitif, USB touch, 12 V | 1 | ~$130–200 | ~$130–200 | [SunFounder 10.1"](https://www.raspberrypiq.com/products/10-inch-touch-screen-raspberry-pi-10.html) / [APROTII 10"](https://robotpishop.com/products/10-inch-1024x600-ips-capacitive-touchscreen) |
| C2 | **Câble HDMI plat** — micro-HDMI ou standard vers HDMI, ~30 cm, pour passage par charnière | 1 | ~$8–12 | ~$10 | Amazon / AliExpress |
| C3 | **Câble USB plat** — pour le tactile, ~30 cm | 1 | ~$5–8 | ~$7 | Amazon / AliExpress |

**Sous-total affichage : ~$150–220**

---

## D. Radio & Connectivité

| # | Composant | Qté | Prix unitaire | Total | Lien |
|---|-----------|-----|--------------|-------|------|
| D1 | **ESP32-C6-DevKitC-1** — Thread / Matter border router, Wi-Fi 6, BLE 5.3, IEEE 802.15.4 | 1 | ~$10 | $10 | [DFRobot ESP32-C6](https://www.dfrobot.com/product-2692.html) |
| D2 | **Dongle USB BLE 5.0** — pont BLE ↔ nœud master (Linux) | 1 | ~$29 | $29 | [BleuIO Standard](https://novelbits.io/bleuio/) |
| D3 | **Câble pigtail U.FL → SMA femelle bulkhead** — passage antenne vers coque | 2 | ~$7 | $14 | [Connected Things](https://connectedthings.store/gb/cables-and-accessories/ufl-to-female-sma-bulkhead-pigtail.html) / [Voltatek CA](https://voltatek.ca/communication/220-ufl-ipx-to-sma-female-pigtail-cable-for-wifi-antenna.html) |
| D4 | **Antenne SMA 2.4 GHz rubber duck** — Wi-Fi / BLE, ~2–5 dBi, omni | 2 | ~$5–16 | ~$10–32 | [L-COM HG2402RDR](https://www.l-com.com/wireless-antenna-24-ghz-2-dbi-rubber-duck-antenna-rigid-rp-sma-plug-connector) |
| D5 | **Câble USB-A court** — ESP32-C6 → port USB du Turing Pi 2.5 (nœud 1) | 1 | ~$5 | $5 | Amazon |

**Sous-total radio : ~$70–90**

---

## E. Charge sans fil (couvercle)

| # | Composant | Qté | Prix unitaire | Total | Lien |
|---|-----------|-----|--------------|-------|------|
| E1 | **Module Qi TX 15 W + bobine** — circuit transmetteur, USB-C / 12 V in, FOD, Qi certifié, ~50×50 mm bobine | 1 | ~$10–25 | ~$15 | [ShopBu — 15 W TX module](https://shopbu.com/product/10w-15w-high-power-wireless-charger-transmitter-module-type-c-coil-for-qi-standard-fast-charging-circuit-board-with-protection/) / AliExpress « Qi 15W TX module coil » |
| E2 | **Câble d'alimentation** — du PSU 12 V / 5 V vers le module Qi (si USB-C, un câble court suffit) | 1 | ~$3 | $3 | Inclus ou Amazon |

**Sous-total charge : ~$18**

---

## F. Refroidissement

| # | Composant | Qté | Prix unitaire | Total | Lien |
|---|-----------|-----|--------------|-------|------|
| F1 | **Noctua NF-A6x25 5V PWM** — ventilateur 60 mm, silencieux, 5 V (compatible 5 V du Turing Pi), PWM | 2 | ~$17 | $34 | [Noctua officiel](https://noctua.at/en/products/fan/nf-a6x25-5v-pwm) / [Newegg](https://www.newegg.com/p/1YF-000T-000Y8) |
| F2 | **Grille de ventilation** — 60 mm, métal ou plastique, avec vis | 2 | ~$3 | $6 | Amazon / AliExpress |
| F3 | **Pâte thermique** — pour les heatsinks RK1 si non fournie | 1 | ~$5 | $5 | Amazon |

**Sous-total refroidissement : ~$45**

---

## G. Enclosure (malette)

| # | Composant | Qté | Prix unitaire | Total | Lien |
|---|-----------|-----|--------------|-------|------|
| G1 | **Pelican 1450** (ou équivalent) — malette étanche, ~370 × 258 × 152 mm intérieur, mousse Pick'N'Pluck, charnière 180°, 2 latchs | 1 | ~$170–200 | ~$185 | [DataPro Pelican 1450](https://www.datapro.net/products/pelican-1450-protector-case.html) / [Nalpak](https://nalpak.com/copy-of-pelican-1400-edc-case/) |
| G2 | **Connecteurs bulkhead** — barrel jack 5.5×2.1 mm (alimentation), RJ45 (Ethernet uplink), USB-C (accès direct master) | 1 lot | ~$15–25 | ~$20 | Amazon (lot de connecteurs chassis-mount) |
| G3 | **Interrupteur rocker illuminé** — 12 V, montage panneau, LED verte | 1 | ~$5 | $5 | Amazon / AliExpress |
| G4 | **Entretoises, vis, inserts M3** — montage de la Turing Pi + écran + modules | 1 lot | ~$10 | $10 | Amazon (kit entretoises M2.5/M3) |
| G5 | **Câble RJ45 court** — du switch Turing Pi vers le connecteur bulkhead RJ45 | 1 | ~$5 | $5 | Amazon |

**Sous-total enclosure : ~$225**

---

## Récapitulatif global

| Catégorie | Sous-total |
|-----------|-----------|
| **A. Turing Pi (cluster)** | ~$1 338 |
| **B. Stockage NVMe** | ~$150 |
| **C. Affichage** | ~$185 |
| **D. Radio & connectivité** | ~$80 |
| **E. Charge Qi** | ~$18 |
| **F. Refroidissement** | ~$45 |
| **G. Enclosure (malette)** | ~$225 |
| | |
| **TOTAL config standard** (1× 32 Go + 3× 16 Go) | **~$2 041** |
| **TOTAL config LLM max** (4× 32 Go) | **~$2 401** |

> Tous les prix en **USD**. Hors shipping et taxes/douane locales.

---

## Check-list de commande rapide

### Sur [turingpi.com](https://turingpi.com/shop/) (une seule commande)

- [ ] 1× Turing Pi 2.5 ($279)
- [ ] 1× RK1 32 Go ($319) — master
- [ ] 3× RK1 16 Go ($199 chacun) — workers _OU_ 3× RK1 32 Go ($319) pour LLM distribué
- [ ] 4× RK1 Heatsink ($12)
- [ ] 1× 24-pin PSU ($45)
- [ ] 1× Power Supply ($45)
- [ ] 1× I/O Shield TP2.5 ($5)

### Ailleurs (Amazon, AliExpress, Newegg, spécialisé)

- [ ] 4× NVMe M.2 2280 256 Go
- [ ] 1× Écran 10.1" IPS tactile HDMI
- [ ] 1× Câble HDMI plat + 1× câble USB plat
- [ ] 1× ESP32-C6-DevKitC-1
- [ ] 1× Dongle BleuIO BLE 5.0
- [ ] 2× Pigtail U.FL → SMA bulkhead + 2× antenne SMA 2.4 GHz
- [ ] 1× Module Qi TX 15 W + bobine
- [ ] 2× Noctua NF-A6x25 5V PWM + grilles
- [ ] 1× Pelican 1450 (ou équivalent)
- [ ] 1× Lot connecteurs bulkhead (barrel jack, RJ45, USB-C)
- [ ] 1× Interrupteur rocker illuminé
- [ ] 1× Kit entretoises M3 + vis
- [ ] 1× Câble RJ45 court
- [ ] 1× Pâte thermique
