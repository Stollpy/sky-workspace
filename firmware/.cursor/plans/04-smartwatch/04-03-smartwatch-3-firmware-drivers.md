# Phase 3 — Firmware : Drivers Capteurs

## Objectif

Créer les **composants Sky** (`sky_<name>`) pour chaque capteur/périphérique,
suivant la convention init → start → stop → deinit du framework Sky.

---

## 1. Composants à développer

### 1.1. `sky_hr` — Capteur HR + SpO2 (MAX30102)

| Détail | Valeur |
|--------|--------|
| Puce | MAX30102 |
| Bus | I2C, adresse 0x57 |
| Dépendance | `sky_core` |

**Fonctionnalités :**
- Lecture PPG brute (LED rouge + IR)
- Algorithme HR (peak detection sur signal IR filtré)
- Calcul SpO2 (ratio R/IR → lookup table)
- Publication événements : `SKY_EVENT_HR_UPDATE`, `SKY_EVENT_SPO2_UPDATE`
- Mode continu vs mode one-shot (économie batterie)

**Dev estimé : 2–3 semaines**

### 1.2. `sky_imu` — IMU 6 axes (MPU-6050 proto / BMI270 final)

| Détail | Valeur |
|--------|--------|
| Puce | MPU-6050 (proto) → BMI270 (PCB custom) |
| Bus | I2C, adresse 0x68 |
| Dépendance | `sky_core` |

**Fonctionnalités :**
- Lecture accéléro 3 axes + gyro 3 axes
- Détection de geste (lever poignet → allumer écran)
- Compteur de pas (step counter)
- Publication événements : `SKY_EVENT_IMU_MOTION`, `SKY_EVENT_IMU_STEP`, `SKY_EVENT_IMU_WRIST_RAISE`
- Abstraction HAL pour swap MPU-6050 ↔ BMI270 sans casser l'API

**Dev estimé : 2 semaines**

### 1.3. `sky_gps` — GNSS (u-blox MIA-M10Q)

| Détail | Valeur |
|--------|--------|
| Puce | u-blox MIA-M10Q |
| Bus | UART (GPIO 42 TX, 41 RX) |
| Dépendance | `sky_core` |

**Fonctionnalités :**
- Parse NMEA ($GPRMC, $GPGGA)
- Fournir lat/lon/alt/speed/heading
- Mode basse conso (Power Save Mode u-blox)
- Publication événements : `SKY_EVENT_GPS_FIX`, `SKY_EVENT_GPS_LOST`

**Dev estimé : 1–2 semaines**

### 1.4. `sky_audio` — Micro PDM + Speaker I2S (MAX98357A)

| Détail | Valeur |
|--------|--------|
| Micro | SPM1423HM4H-B (PDM, GPIO 44 CLK / 47 DATA) |
| Ampli | MAX98357A (I2S, GPIO 48 BCLK / 15 WCLK / 46 DOUT) |
| Dépendance | `sky_core` |

**Fonctionnalités :**
- Capture audio PDM → buffer PCM (ring buffer FreeRTOS)
- Playback PCM → I2S → speaker
- Contrôle volume logiciel
- Publication événements : `SKY_EVENT_AUDIO_CAPTURE_READY`
- API simple : `sky_audio_play(buf, len)`, `sky_audio_record_start()`, `sky_audio_record_stop()`

**Dev estimé : 2 semaines**

### 1.5. `sky_haptic` — Vibreur haptique (DRV2605 + LRA)

| Détail | Valeur |
|--------|--------|
| Puce | DRV2605 (déjà sur la T-Watch) |
| Bus | I2C, adresse 0x5A |
| Dépendance | `sky_core` |

**Fonctionnalités :**
- Patterns prédéfinis (tap, double-tap, notification, alarme)
- Custom waveform séquences
- API : `sky_haptic_play(SKY_HAPTIC_PATTERN_NOTIFICATION)`

**Dev estimé : 1 semaine**

---

## 2. Event ID Ranges (convention Sky)

| Range | Composant |
|-------|-----------|
| 0x0400–0x040F | `sky_hr` |
| 0x0410–0x041F | `sky_imu` |
| 0x0420–0x042F | `sky_gps` |
| 0x0430–0x043F | `sky_audio` |
| 0x0440–0x044F | `sky_haptic` |

---

## 3. Ordre de développement recommandé

```
sky_haptic ──► sky_imu ──► sky_audio ──► sky_gps ──► sky_hr
 (1 sem)       (2 sem)     (2 sem)      (1-2 sem)   (2-3 sem)
```

Commencer par le plus simple (haptique, déjà sur la carte) pour valider la chaîne
composant Sky → event bus → test, puis monter en complexité.

---

## 4. Structure composant type

```
components/sky_hr/
├── CMakeLists.txt
├── Kconfig
├── include/
│   └── sky_hr.h
└── src/
    ├── sky_hr.c
    └── sky_hr_algo.c     ← algorithme HR/SpO2
```
