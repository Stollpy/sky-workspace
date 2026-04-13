# Phase 6 — Roadmap Complète & Timeline

## Vue d'ensemble

Durée totale estimée : **~6–8 mois** (en parallèle d'autres projets, rythme hobby/side-project).
En full-time concentré : **~4–5 mois**.

---

## Timeline détaillée

```
Mois 1          Mois 2          Mois 3          Mois 4          Mois 5          Mois 6          Mois 7+
│               │               │               │               │               │               │
├── M1 ─────────┤               │               │               │               │               │
│ HARDWARE      │               │               │               │               │               │
│ Achat +       │               │               │               │               │               │
│ Assemblage    │               │               │               │               │               │
│               ├── M2 ─────────┤               │               │               │               │
│               │ DRIVERS       │               │               │               │               │
│               │ haptique, IMU │               │               │               │               │
│               │ audio         │               │               │               │               │
│               │               ├── M3 ─────────┤               │               │               │
│               │               │ DRIVERS       │               │               │               │
│               │               │ GPS, HR/SpO2  │               │               │               │
│               │               │               ├── M4 ─────────┤               │               │
│               │               │               │ CONNECTIVITÉ  │               │               │
│               │               │               │ WiFi adapt,   │               │               │
│               │               │               │ BLE GATT,     │               │               │
│               │               │               │ BT Audio ext  │               │               │
│               │               │               │               ├── M5 ─────────┤               │
│               │               │               │               │ CONNECTIVITÉ  │               │
│               │               │               │               │ LTE/AT parser │               │
│               │               │               │               │ + UI display  │               │
│               │               │               │               │ driver + face │               │
│               │               │               │               │               ├── M6 ─────────┤
│               │               │               │               │               │ UI COMPLET    │
│               │               │               │               │               │ Tous écrans   │
│               │               │               │               │               │ Navigation    │
│               │               │               │               │               │               ├── M7+
│               │               │               │               │               │               │ PCB CUSTOM
│               │               │               │               │               │               │ Design V1
```

---

## Détail semaine par semaine

### MOIS 1 — Hardware & Setup (Semaines 1–4)

| Semaine | Tâche | Livrable |
|---------|-------|----------|
| **S1** | Commander tout le hardware (voir BOM Phase 1) | Commandes passées |
| **S1–S2** | Setup environnement ESP-IDF / PlatformIO pour la T-Watch S3 Plus | Projet compilé, flash factory OK |
| **S2** | Tester firmware factory LILYGO, valider écran / touch / audio / GPS / haptique | Validation hardware de base |
| **S3** | Réception breakouts (MAX30102, MPU-6050), câblage I2C | Breakouts câblés sur breadboard |
| **S3** | Script I2C scan : valider tous les devices sur le bus | Scan I2C montre 0x19, 0x34, 0x51, 0x57, 0x5A, 0x68 |
| **S4** | Réception module SIM7080G, câblage UART + alim séparée | AT commands OK (`AT` → `OK`) |
| **S4** | Créer le squelette du projet firmware (repo, structure composants Sky) | Repo prêt, `main.c` minimal qui boot |

---

### MOIS 2 — Drivers Capteurs I (Semaines 5–8)

| Semaine | Tâche | Livrable |
|---------|-------|----------|
| **S5** | `sky_haptic` : driver DRV2605, patterns basiques | `sky_haptic_play(NOTIFICATION)` fonctionne |
| **S6** | `sky_imu` : driver MPU-6050, lecture accel+gyro raw | Données 6 axes en log série |
| **S7** | `sky_imu` : step counter, détection wrist-raise | Événements `SKY_EVENT_IMU_STEP`, `SKY_EVENT_IMU_WRIST_RAISE` |
| **S8** | `sky_audio` : capture micro PDM → buffer PCM | Audio capturé, playback speaker validé |

---

### MOIS 3 — Drivers Capteurs II (Semaines 9–12)

| Semaine | Tâche | Livrable |
|---------|-------|----------|
| **S9** | `sky_gps` : parser NMEA, lecture position | Lat/lon affichés en log |
| **S10** | `sky_gps` : mode basse conso, événements fix/lost | GPS fonctionnel + power-aware |
| **S11** | `sky_hr` : driver MAX30102, lecture PPG raw | Signal PPG brut en log |
| **S12** | `sky_hr` : algo HR (peak detection) + SpO2 (ratio R/IR) | BPM + SpO2 % affichés en log |

---

### MOIS 4 — Connectivité I (Semaines 13–16)

| Semaine | Tâche | Livrable |
|---------|-------|----------|
| **S13** | `sky_wifi` : adapter politique power pour montre (OFF par défaut) | Wi-Fi ON/OFF à la demande, OTA fonctionnel |
| **S14** | `sky_ble` : GATT server, service HR standard (0x180D) | App mobile (nRF Connect) voit le BPM |
| **S15** | `sky_ble` : services custom (SpO2, IMU, GPS), notifications push | Données capteurs visibles en BLE |
| **S16** | `sky_ble` : pairing sécurisé + bonding | Connexion persistante téléphone ↔ montre |

---

### MOIS 5 — Connectivité II + UI début (Semaines 17–20)

| Semaine | Tâche | Livrable |
|---------|-------|----------|
| **S17** | BT Audio : intégrer module KCX_BT_EMITTER, piloter en I2S | Audio streamé vers enceinte BT |
| **S18** | `sky_lte` : AT parser, machine à états modem | Modem s'enregistre sur réseau LTE-M |
| **S19** | `sky_lte` : TCP/IP via modem, HTTP GET/POST | Requête HTTP depuis la montre via LTE |
| **S20** | `sky_display` : driver LVGL + ST7789, touch FT6336U, backlight | Écran affiche « Hello World » via LVGL |

---

### MOIS 6 — UI Complet (Semaines 21–26)

| Semaine | Tâche | Livrable |
|---------|-------|----------|
| **S21** | Watch Face : heure (RTC), date, icône batterie, icône BLE | Cadran fonctionnel |
| **S22** | Quick Panel (swipe up) : toggles WiFi/BLE/LTE, luminosité | Panel de contrôle rapide |
| **S23** | Écran HR / SpO2 : courbe PPG temps réel, BPM, % | Écran santé |
| **S24** | Écran Activité : pas, distance, calories | Écran sport |
| **S25** | Écran Notifications : liste sync BLE depuis téléphone | Notifs visibles |
| **S26** | Écran Réglages + navigation par gestes (swipe L/R/U/D) | Navigation complète |

---

### MOIS 7+ — PCB Custom V1

| Tâche | Détail |
|-------|--------|
| Choix MCU définitif | ESP32 classique (si BT Classic nécessaire) ou ESP32-S3 + co-processeur |
| Schéma électrique | KiCad, basé sur les learnings du proto |
| Design PCB | HDI 4–6 couches, composants 0201 |
| Intégration eSIM MFF2 | Puce MFF2 soudée, interfacée au module LTE |
| Antennes | FPC LTE + antenne céramique WiFi/BT, matching réseau |
| Boîtier | Design boîtier montre (3D print proto, injection finale) |
| Tests | Certification RF (si commercial), test thermique |

---

## Milestones clés

| Milestone | Critère de succès | Semaine cible |
|-----------|-------------------|---------------|
| **M0 — Hardware Ready** | T-Watch + breakouts câblés, I2C scan OK | S4 |
| **M1 — Sensors MVP** | Tous les capteurs lisent des données valides en log | S12 |
| **M2 — Connected MVP** | Montre visible en BLE, données poussées vers téléphone | S16 |
| **M3 — LTE Online** | Montre fait une requête HTTP via LTE-M | S19 |
| **M4 — UI MVP** | Watch face + 2 écrans fonctionnels avec navigation | S22 |
| **M5 — Firmware V1** | Tous les écrans, tous les capteurs, BLE + Wi-Fi + LTE | S26 |
| **M6 — PCB V1** | Premier PCB custom fabriqué et testé | M8–M10 |

---

## Risques & mitigations

| Risque | Impact | Mitigation |
|--------|--------|------------|
| GPIO insuffisants sur T-Watch pour UART LTE | Bloque le proto LTE | Utiliser un second ESP32 dédié LTE en pont UART ↔ BLE |
| MAX30102 pas assez précis au poignet | Mesures HR/SpO2 aberrantes | Tester placement mécanique, sinon passer à un AFE type AFE4404 |
| LE Audio pas mature sur ESP-IDF | Pas de streaming BT natif | Module KCX externe (solution de contournement validée) |
| Courant LTE dépasse capacité AXP2101 | Reset / brownout | Alim LTE séparée avec gros condensateur (1000 µF) |
| LVGL trop lourd en RAM | Crash ou lenteur | Utiliser LVGL en mode partiel (partial rendering), réduire les buffers |
