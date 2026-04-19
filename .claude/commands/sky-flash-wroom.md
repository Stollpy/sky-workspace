---
description: Rappelle et prépare le workflow de flash du bridge WROOM (ESP-IDF).
argument-hint: <port-usb> (ex. /dev/cu.usbserial-110)
---

Tu vas guider l'utilisateur pour flasher `sky-hub-antenna-wroom`
sur une ESP32-WROOM-32 (Phase A du bridge Matter, variante Wi-Fi seul).

Contexte :
- repo cible : `sky-hub-antenna-wroom/`
- build system : ESP-IDF 5.x (pas PlatformIO ici)
- target : `esp32` (WROOM-32), UART bridge TX=17 RX=16
- command IDs : `0x50`/`0x51` (HTTP dev start/stop), `0x11`/`0x12`
  (WC open/close vers Luxia), `0x30` (bridge status)

IMPORTANT — sécurité :
- **Ne lance JAMAIS `idf.py flash` ni `idf.py monitor` sans demander
  une confirmation explicite à l'utilisateur.** Ces commandes
  modifient l'état du hardware et peuvent interrompre son travail.

Étapes :

1. Si `$1` est vide, liste les ports USB détectés :
   - macOS : `ls /dev/cu.usbserial-* 2>/dev/null; ls /dev/cu.SLAB* 2>/dev/null; ls /dev/cu.wchusbserial* 2>/dev/null`
   - Si aucun, suggère de vérifier le câble et demande un port.
2. Rappelle le workflow à l'utilisateur, puis attend son feu vert
   avant d'aller plus loin :
   ```bash
   cd sky-hub-antenna-wroom
   get_idf                            # active l'env ESP-IDF
   export IDF_USB_PORT=$1
   idf.py set-target esp32
   idf.py build
   idf.py -p $IDF_USB_PORT flash monitor
   ```
3. Si l'utilisateur confirme pour `build` seulement, tu peux l'exécuter
   (build est non-destructif). `flash monitor` reste toujours en "ask".
4. En cas d'erreur classique :
   - `Failed to connect` → appuyer sur BOOT pendant le reset
   - `A fatal error occurred: Could not open port` → port pris, tuer
     l'éventuel `picocom`/`screen` ouvert.

Réfère-toi au `CLAUDE.md` dans `sky-hub-antenna-wroom/` pour les
détails spécifiques (CRC serial, reconnection WebSocket).
