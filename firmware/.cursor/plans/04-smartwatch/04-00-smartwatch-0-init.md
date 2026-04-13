# 04 — Smartwatch Connectée · Plan Général

## Objectif

Concevoir et prototyper une **montre connectée** de la taille d'une Apple Watch (~44 × 38 × 11 mm) embarquant :

- **Écran** tactile compact (IPS/AMOLED ~1.3–1.7")
- **Wi-Fi** 802.11 b/g/n
- **BLE 5.0** + **Bluetooth Classic** (A2DP/HFP pour audio speaker/écouteurs)
- **LTE-M / NB-IoT** avec **eSIM (MFF2)**
- **Micro** MEMS + **Haut-parleur** miniature (I2S)
- **Vibration haptique** (LRA + driver DRV2605)
- **Capteur HR + SpO2** (PPG)
- **IMU 6 axes** (accéléromètre + gyroscope)
- **GPS/GNSS**
- **Batterie LiPo** ~300–500 mAh, PMU intégrée

## Stratégie

1. **Phase 1** — Acheter la plateforme de dev la plus proche (**LILYGO T-Watch S3 Plus**) + modules breakout pour les capteurs manquants
2. **Phase 2** — Assemblage proto : câbler les breakouts I2C au bus libre de la T-Watch
3. **Phase 3** — Firmware drivers capteurs (HR, SpO2, IMU, GPS…)
4. **Phase 4** — Firmware connectivité (BLE, Wi-Fi, LTE externe)
5. **Phase 5** — Firmware UI / UX sur écran
6. **Phase 6** — Conception du PCB custom (V1 hardware propre)

## Phases du plan

| Fichier | Phase |
|---------|-------|
| `04-01-smartwatch-1-hardware-bom.md` | BOM & achats hardware |
| `04-02-smartwatch-2-proto-assembly.md` | Assemblage proto |
| `04-03-smartwatch-3-firmware-drivers.md` | Firmware — drivers capteurs |
| `04-04-smartwatch-4-firmware-connectivity.md` | Firmware — connectivité |
| `04-05-smartwatch-5-firmware-ui.md` | Firmware — UI/UX écran |
| `04-06-smartwatch-6-roadmap.md` | Roadmap complète & timeline |
