---
description: Rappelle et prépare le workflow de flash de Luxia (ESP-IDF).
argument-hint: <port-usb> (ex. /dev/cu.usbserial-110)
---

Tu vas guider l'utilisateur pour flasher `sky-luxia/` sur une
ESP32-WROOM-32 (firmware produit Luxia, Matter over Wi-Fi, Window
Covering cluster 0x0102).

Contexte :
- repo cible : `sky-luxia/`
- build system : ESP-IDF 5.x + `esp_matter` + `sky-framework` v0.2.0
- target : `esp32` (WROOM-32 classique)
- modules : `luxia_main.c` + `luxia_wifi.c/h` + `luxia_matter.cpp/h`
  + `luxia_motor.c/h` + `luxia_http.c/h` + `luxia_cmd.c/h`
- commissioning Matter : passcode `20202021`, discriminateur `3840`,
  BLE PASE → CASE → Wi-Fi STA

IMPORTANT — sécurité :
- **Ne lance JAMAIS `idf.py flash` ni `idf.py monitor` sans demander
  une confirmation explicite à l'utilisateur.** Hardware réel +
  stepper motor : effet physique possible au reboot.

Étapes :

1. Si `$1` est vide, liste les ports USB détectés :
   - macOS : `ls /dev/cu.usbserial-* 2>/dev/null; ls /dev/cu.SLAB* 2>/dev/null; ls /dev/cu.wchusbserial* 2>/dev/null`
   - Si aucun, suggère de vérifier le câble.
2. Rappelle le workflow à l'utilisateur, puis attend son feu vert :
   ```bash
   cd sky-luxia
   get_idf
   export IDF_USB_PORT=$1
   idf.py set-target esp32
   idf.py build
   idf.py -p $IDF_USB_PORT flash monitor
   ```
3. `build` est OK sans confirmation explicite. `flash`/`monitor`
   restent toujours sujets à "ask".
4. Note : si tu veux forcer un re-commissioning propre, propose un
   `erase-flash` **avant** le flash (également destructif → confirmation).

Réfère-toi au `CLAUDE.md` dans `sky-luxia/` et au
`firmware/CLAUDE.md` racine pour les conventions produit (`luxia_*`
prefix, `percent_100ths` 0-10000, GPIO stepper 17/16/4/0).
