# Phase 5 — Firmware : UI / UX Écran

## Objectif

Créer l'interface utilisateur de la montre sur l'écran **1.54" IPS 240×240** (ST7789V3, SPI)
avec navigation tactile (FT6336U) et gestion intelligente du rétroéclairage.

---

## 1. Choix du framework graphique

| Option | Description | Avantage | Inconvénient |
|--------|-------------|----------|--------------|
| **LVGL v9** | Bibliothèque UI embarquée, très utilisée sur ESP32 | Riche (widgets, animations, thèmes), communauté active | Empreinte mémoire (~100–200 Ko RAM) |
| **Custom framebuffer** | Dessiner directement en SPI | Contrôle total, ultra léger | Beaucoup de code pour chaque écran |

**Recommandation : LVGL v9** — c'est le standard de facto pour les montres ESP32 (LILYGO fournit déjà des exemples LVGL pour la T-Watch).

---

## 2. Composant `sky_display`

| Détail | Valeur |
|--------|--------|
| Écran | ST7789V3, 240×240, SPI |
| Touch | FT6336U, I2C (Wire1, 0x38) |
| Backlight | GPIO 45 via AXP2101 (ALDO2) |
| Dépendance | `sky_core`, LVGL |

### Architecture

```
components/sky_display/
├── CMakeLists.txt
├── Kconfig                    ← résolution, orientation, timeout backlight
├── include/
│   └── sky_display.h
└── src/
    ├── sky_display.c          ← init écran + LVGL + touch + tick timer
    ├── sky_display_driver.c   ← flush callback SPI → ST7789
    └── sky_display_touch.c    ← lecture FT6336U → LVGL indev
```

**Dev estimé : 2 semaines**

---

## 3. Composant `sky_ui` — Navigation & Écrans

### 3.1. Structure de navigation

```
                    ┌─────────────┐
         ┌──────────│  Watch Face │──────────┐
         │          └──────┬──────┘          │
     swipe ←           swipe ↑           swipe →
         │                 │                 │
   ┌─────▼─────┐   ┌──────▼──────┐   ┌──────▼──────┐
   │ Notifs     │   │ Quick Panel │   │ Apps Menu   │
   │            │   │ (WiFi, BT,  │   │             │
   │            │   │  Battery)   │   │ - HR/SpO2   │
   └────────────┘   └─────────────┘   │ - GPS       │
                                      │ - Chrono    │
                                      │ - Réglages  │
                                      └─────────────┘
```

### 3.2. Écrans à développer

| Écran | Données affichées | Priorité |
|-------|-------------------|----------|
| **Watch Face** | Heure (RTC), date, batterie, BLE status | P0 |
| **Quick Panel** | Toggles Wi-Fi/BLE/LTE, luminosité, mode avion | P0 |
| **HR / SpO2** | BPM en temps réel, courbe PPG, SpO2 % | P1 |
| **Activité** | Pas (IMU), distance, calories | P1 |
| **GPS / Map** | Lat/lon, vitesse, cap | P2 |
| **Notifications** | Liste notifications BLE (sync téléphone) | P1 |
| **Chronomètre / Timer** | Chrono, compte à rebours | P2 |
| **Réglages** | Luminosité, vibration, Wi-Fi config, about | P1 |

### 3.3. Structure

```
components/sky_ui/
├── CMakeLists.txt
├── include/
│   └── sky_ui.h
└── src/
    ├── sky_ui.c              ← screen manager, navigation gestures
    ├── sky_ui_watchface.c    ← cadran principal
    ├── sky_ui_panel.c        ← quick panel (toggles)
    ├── sky_ui_hr.c           ← écran HR/SpO2
    ├── sky_ui_activity.c     ← écran activité/pas
    ├── sky_ui_gps.c          ← écran GPS
    ├── sky_ui_notifs.c       ← écran notifications
    ├── sky_ui_settings.c     ← réglages
    └── sky_ui_assets.h       ← icônes, fonts embarquées
```

**Dev estimé : 4–6 semaines**

---

## 4. Gestion du rétroéclairage & veille écran

| Comportement | Implémentation |
|--------------|----------------|
| Écran ON au lever de poignet | `sky_imu` → `SKY_EVENT_IMU_WRIST_RAISE` → allumer backlight |
| Écran ON au toucher | `sky_display_touch` → interrupt GPIO 16 → allumer |
| Timeout extinction | Configurable via NVS (défaut : 5 s) |
| Luminosité | AXP2101 ALDO2 (PWM ou ON/OFF selon hardware) |

---

## 5. Ordre de développement UI

```
sky_display (driver) ──► Watch Face ──► Quick Panel ──► HR Screen ──► Activity ──► reste
     (2 sem)              (1 sem)        (1 sem)        (1 sem)       (1 sem)     (2 sem)
```
