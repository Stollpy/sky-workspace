---
description: Rappelle le workflow de run du device-service (Rust) avec les env vars DS_*.
---

Tu vas rappeler à l'utilisateur comment lancer le `device-service`
en local (bridge UART ↔ Reverb WebSocket). **Ne lance PAS la commande
toi-même** — `cargo run` ouvre le port série et maintient une
connexion WebSocket, c'est un process long.

Contexte :
- repo : `sky-hub-services/device-service/`
- stack : Rust 2021, `tokio`, `serialport`, `tokio-tungstenite`,
  `hmac`/`sha2` (HMAC-SHA256 pour private channels Pusher/Reverb)
- rôle : lit les attribute reports Matter depuis l'UART du
  co-processeur (`sky-hub-antenna` ou `sky-hub-antenna-wroom`),
  pousse dans Reverb → Orbit ; dans l'autre sens, écoute les commandes
  Orbit sur `private-device-commands` → envoie sur UART.
- config : `envy` avec préfixe `DS_`, voir `src/config.rs`.

Env vars typiques (dev local sur RPi4B ou mac via câble USB) :

```bash
export DS_SERIAL_PORT=/dev/ttyAMA0          # RPi4B PL011
# export DS_SERIAL_PORT=/dev/cu.usbserial-110   # sur Mac via adaptateur
export DS_SERIAL_BAUD_RATE=115200
export DS_REVERB_URL=ws://localhost:8080/app/sky-local-key
export DS_REVERB_APP_KEY=sky-local-key
export DS_REVERB_APP_SECRET=sky-local-secret
export DS_HEALTH_INTERVAL_SECS=10
export DS_ATTRIBUTE_TTL_SECS=30
export RUST_LOG=info,device_service=debug
```

Étapes :

1. Vérifie que `sky-hub-services/device-service/Cargo.toml` existe.
2. Rappelle à l'utilisateur la commande à lancer lui-même dans un
   terminal dédié :
   ```bash
   cd sky-hub-services/device-service
   cargo build --release   # (optionnel) build préalable
   cargo run               # nécessite les DS_* en env
   ```
3. Précise les prérequis côté Orbit :
   - `sky-orbit` doit tourner (`composer run dev`) et Reverb doit être
     démarré (`php artisan reverb:start --debug`).
   - Les credentials `sky-local-key`/`sky-local-secret` doivent matcher
     la config Reverb côté Laravel (`.env` d'Orbit).
4. Si le port série est occupé, rappelle que `picocom`, `screen`,
   `minicom` et Arduino IDE peuvent le verrouiller.

Ne lance pas `cargo run` directement — c'est listé en `ask` dans
`.claude/settings.json`. Demande confirmation avant, ou laisse
l'utilisateur démarrer le service lui-même.
